// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Math.hpp"
#include "SimplifiedMath.hpp"
#include "FAISphere.hpp"
#include "WGS84.hpp"
#include "GeoPoint.hpp"
#include "Math/Util.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

using namespace WGS84;

[[gnu::const]]
static inline Angle
EarthASin(const double a) noexcept
{
  return Angle::asin(a);
}

[[gnu::const]]
static inline Angle
EarthDistance(const double a) noexcept
{
  if (a <= 0)
    return Angle::Zero();

  return Angle::acos(1 - 2 * a);
}

static constexpr double
CalcUSquare(double cos_sq_alpha) noexcept
{
  constexpr double EQUATOR_RADIUS_SQ =
    WGS84::EQUATOR_RADIUS * WGS84::EQUATOR_RADIUS;
  constexpr double POLE_RADIUS_SQ =
    WGS84::POLE_RADIUS * WGS84::POLE_RADIUS;
  constexpr double factor((EQUATOR_RADIUS_SQ - POLE_RADIUS_SQ)
                          / POLE_RADIUS_SQ);

  return cos_sq_alpha * factor;
}

static constexpr double
CalcA(double u_sq) noexcept
{
  const double A_16k = 16384
    + u_sq * (4096 + u_sq *
              (-768 + u_sq * (320 - 175 * u_sq)));
  return A_16k / 16384;
}

static constexpr double
CalcB(double u_sq) noexcept
{
  const auto B_1k = u_sq * (256 + u_sq * (-128 + u_sq * (74 - 47 * u_sq)));
  return B_1k / 1024;
}

static constexpr double
CalcC(double cos_sq_alpha) noexcept
{
  return FLATTENING / 16 * cos_sq_alpha *
    (4 + FLATTENING * (4 - 3 * cos_sq_alpha));
}

[[gnu::pure]]
static GeoPoint
IntermediatePoint(const GeoPoint &loc1, const GeoPoint &loc2,
                  Angle dthis, Angle dtotal) noexcept
{
  assert(loc1.IsValid());
  assert(loc2.IsValid());

  if (loc1.longitude == loc2.longitude &&
      loc1.latitude == loc2.latitude)
    return loc1;

  if (!dtotal.IsPositive())
    return loc1;

  // dthis can be larger than dtotal or even smaller than 0, which extrapolates
  // the gc between loc1 and loc2 to loc3
  //assert(dthis <= dtotal && !dthis.IsNegative());

  const auto A = (dtotal - dthis).sin();
  const auto B = dthis.sin();

  const auto sc1 = loc1.latitude.SinCos();
  const auto sin_loc1_lat = sc1.first, cos_loc1_lat = sc1.second;

  const auto sc2 = loc2.latitude.SinCos();
  const auto sin_loc2_lat = sc2.first, cos_loc2_lat = sc2.second;

  const auto sc3 = loc1.longitude.SinCos();
  const auto sin_loc1_lon = sc3.first, cos_loc1_lon = sc3.second;

  const auto sc4 = loc2.longitude.SinCos();
  const auto sin_loc2_lon = sc4.first, cos_loc2_lon = sc4.second;

  const auto a_cos_loc1_lat = A * cos_loc1_lat;
  const auto b_cos_loc2_lat = B * cos_loc2_lat;

  const auto x = a_cos_loc1_lat * cos_loc1_lon + b_cos_loc2_lat * cos_loc2_lon;
  const auto y = a_cos_loc1_lat * sin_loc1_lon
    + b_cos_loc2_lat * sin_loc2_lon;
  const auto z = A  * sin_loc1_lat + B * sin_loc2_lat;

  GeoPoint loc3;
  loc3.latitude = Angle::FromXY(hypot(x, y), z);
  loc3.longitude = Angle::FromXY(x, y);
  loc3.Normalize(); // ensure longitude is within -180:180

  return loc3;
}

GeoPoint
IntermediatePoint(const GeoPoint &loc1, const GeoPoint &loc2,
                  const double dthis) noexcept
{
  const auto dtotal = ::Distance(loc1, loc2);

  if (dthis >= dtotal)
    return loc2;

  return IntermediatePoint(loc1, loc2,
                           FAISphere::EarthDistanceToAngle(dthis),
                           FAISphere::EarthDistanceToAngle(dtotal));
}

GeoPoint
Middle(const GeoPoint &a, const GeoPoint &b) noexcept
{
  // TODO: optimize this naive approach
  const auto distance = Distance(a, b);
  return IntermediatePoint(a, b, distance / 2);
}

void
DistanceBearing(const GeoPoint &loc1, const GeoPoint &loc2,
                double *distance, Angle *bearing) noexcept
{
  const auto lon21 = loc2.longitude - loc1.longitude;

  auto u1 = atan((1 - FLATTENING) * loc1.latitude.tan()),
    u2 = atan((1 - FLATTENING) * loc2.latitude.tan());

  auto sinu1 = sin(u1), cosu1 = cos(u1);

  auto sinu2 = sin(u2), cosu2 = cos(u2);

  auto lambda = lon21.Radians(), lambda_p = Angle::FullCircle().Radians();

  unsigned iterLimit = 20;
  double cos_sq_alpha = 0, sin_sigma = 0, cos_sigma = 0, cos_2_sigma_m = 0,
    sigma = 0;

  while (fabs(lambda - lambda_p) > 1e-7 && --iterLimit) {
    auto sin_lambda = sin(lambda), cos_lambda = cos(lambda);

    sin_sigma = hypot(cosu2 * sin_lambda,
                      cosu1 * sinu2 - sinu1 * cosu2 * cos_lambda);

    if (sin_sigma == 0) {
      // coincident points...
      if (distance != nullptr) *distance = 0;
      if (bearing != nullptr) *bearing = Angle::Zero();
      return;
    }

    cos_sigma = sinu1 * sinu2 + cosu1 * cosu2 * cos_lambda;
    sigma = atan2(sin_sigma, cos_sigma);

    auto inner_alpha = cosu1 * cosu2 * sin_lambda / sin_sigma;
    cos_sq_alpha = 1 - Square(inner_alpha);

    if (fabs(loc1.latitude.Radians()) < 1e-7 &&
        fabs(loc2.latitude.Radians()) < 1e-7) {
      // both points are on equator.
      cos_2_sigma_m = -1;
      lambda_p = lambda;
      lambda = lon21.Radians() + FLATTENING * inner_alpha * sigma;
    } else {
      cos_2_sigma_m = cos_sigma - 2 * sinu1 * sinu2 / cos_sq_alpha;

      auto c = FLATTENING / 16 * cos_sq_alpha *
                (4 + FLATTENING * (4 - 3 * cos_sq_alpha));

      lambda_p = lambda;
      lambda = lon21.Radians() + (1 - c) * FLATTENING * inner_alpha *
        (sigma + c * sin_sigma * (cos_2_sigma_m + c * cos_sigma *
                                  (-1 + 2 * Square(cos_2_sigma_m))));
    }
  }

  if (iterLimit == 0) {
    assert(false || !"WGS84 convergence failed");
    return;  // formula failed to converge
  }

  if (distance != nullptr) {
    const auto u_sq = CalcUSquare(cos_sq_alpha);
    const auto A = CalcA(u_sq);
    const auto B = CalcB(u_sq);

    auto delta_sigma = B * sin_sigma * (
      cos_2_sigma_m +
      B / 4 * (cos_sigma * (-1 + 2 * Square(cos_2_sigma_m)) -
               B / 6 * cos_2_sigma_m *
               (-3 + 4 * Square(sin_sigma)) *
               (-3 + 4 * Square(cos_2_sigma_m))));

    *distance = POLE_RADIUS * A * (sigma - delta_sigma);
  }

  if (bearing != nullptr)
    *bearing = Angle::Radians(atan2(cosu2 * sin(lambda),
      cosu1 * sinu2 - sinu1 * cosu2 * cos(lambda))).AsBearing();
}

double
ProjectedDistance(const GeoPoint &loc1, const GeoPoint &loc2,
                  const GeoPoint &loc3) noexcept
{
  Angle dist_AD, crs_AD;
  DistanceBearingS(loc1, loc3, &dist_AD, &crs_AD);
  if (!dist_AD.IsPositive())
    /* workaround: new sine implementation may return small non-zero
       values for sin(0) */
    return 0;

  Angle dist_AB, crs_AB;
  DistanceBearingS(loc1, loc2, &dist_AB, &crs_AB);
  if (!dist_AB.IsPositive())
    /* workaround: new sine implementation may return small non-zero
       values for sin(0) */
    return 0;

  // The "along track distance", along_track_distance, the distance from A along the
  // course towards B to the point abeam D

  const auto sindist_AD = dist_AD.sin();
  const auto cross_track_distance =
    EarthASin(sindist_AD * (crs_AD - crs_AB).sin());

  const auto sc = cross_track_distance.SinCos();
  const auto sinXTD = sc.first, cosXTD = sc.second;

  // along track distance
  const Angle along_track_distance =
    EarthASin(Cathetus(sindist_AD, sinXTD) / cosXTD);

  auto projected = IntermediatePoint(loc1, loc2, along_track_distance, dist_AB);

  return Distance(loc1, projected);
}

double
DoubleDistance(const GeoPoint &loc1, const GeoPoint &loc2,
               const GeoPoint &loc3) noexcept
{
  assert(loc1.IsValid());
  assert(loc2.IsValid());
  assert(loc3.IsValid());

  const auto cos_loc1_lat = loc1.latitude.cos();
  const auto cos_loc2_lat = loc2.latitude.cos();
  const auto cos_loc3_lat = loc3.latitude.cos();

  const auto s21 = (loc2.latitude - loc1.latitude).accurate_half_sin();
  const auto sl21 = (loc2.longitude - loc1.longitude).accurate_half_sin();
  const auto s32 = (loc3.latitude - loc2.latitude).accurate_half_sin();
  const auto sl32 = (loc3.longitude - loc2.longitude).accurate_half_sin();

  const auto a12 = Square(s21) + cos_loc1_lat * cos_loc2_lat * Square(sl21);
  const auto a23 = Square(s32) + cos_loc2_lat * cos_loc3_lat * Square(sl32);

  return FAISphere::REARTH *
    (EarthDistance(a12) + EarthDistance(a23)).Radians();
}

GeoPoint
FindLatitudeLongitude(const GeoPoint &loc, const Angle bearing,
                      double distance) noexcept
{
  assert(loc.IsValid());
  assert(distance >= 0);

  if (distance <= 0)
    return loc;

  GeoPoint loc_out;

  const auto lon1 = loc.longitude.Radians();
  const auto lat1 = loc.latitude.Radians();

  //const auto alpha1 = bearing.Radians();
  const auto sin_alpha1 = bearing.SinCos().first;
  const auto cos_alpha1 = bearing.SinCos().second;

  const auto tan_u1 = (1 - FLATTENING) * tan(lat1);
  const auto cos_u1 = 1 / hypot(1, tan_u1);
  const auto sin_u1 = tan_u1 * cos_u1;

  const auto sigma1 = atan2(tan_u1, cos_alpha1);

  const auto sin_alpha = cos_u1 * sin_alpha1;
  const auto cos_sq_alpha = 1. - Square(sin_alpha);

  const auto u_sq = CalcUSquare(cos_sq_alpha);
  const auto A = CalcA(u_sq);
  const auto B = CalcB(u_sq);

  auto sigma = distance / (POLE_RADIUS * A);
  auto sigmaP = M_2PI;

  double sin_sigma, cos_sigma, cos_2_sigma_m;

  do {
    cos_2_sigma_m = cos(2 * sigma1 + sigma);
    sin_sigma = sin(sigma);
    cos_sigma = cos(sigma);

    auto delta_sigma = B * sin_sigma *
      (cos_2_sigma_m + B / 4 *
       (cos_sigma *
        (-1 + 2 * Square(cos_2_sigma_m)) - B / 6 * cos_2_sigma_m *
        (-3 + 4 * Square(sin_sigma)) * (-3 + 4 * Square(cos_2_sigma_m))));

    sigmaP = sigma;
    sigma = distance / (POLE_RADIUS * A) + delta_sigma;
  } while (fabs(sigma - sigmaP) > 1e-7);

  const auto tmp = sin_u1 * sin_sigma - cos_u1 * cos_sigma * cos_alpha1;
  const auto lat2 = atan2(sin_u1 * cos_sigma + cos_u1 * sin_sigma * cos_alpha1,
                          (1 - FLATTENING) * hypot(sin_alpha, tmp));

  const auto lambda = atan2(sin_sigma * sin_alpha1, cos_u1 * cos_sigma -
    sin_u1 * sin_sigma * cos_alpha1);

  const auto C = CalcC(cos_sq_alpha);

  const auto L = lambda - (1 - C) * FLATTENING * sin_alpha *
    (sigma + C * sin_sigma *
     (cos_2_sigma_m + C * cos_sigma * (-1 + 2 * Square(cos_2_sigma_m))));

  loc_out.longitude = Angle::Radians(lon1 + L);
  loc_out.latitude = Angle::Radians(lat2);

  loc_out.Normalize(); // ensure longitude is within -180:180

  return loc_out;
}

double
Distance(const GeoPoint &loc1, const GeoPoint &loc2) noexcept
{
  double distance;
  DistanceBearing(loc1, loc2, &distance, nullptr);
  return distance;
}

Angle
Bearing(const GeoPoint &loc1, const GeoPoint &loc2) noexcept
{
  Angle bearing;
  DistanceBearing(loc1, loc2, nullptr, &bearing);
  return bearing;
}

namespace {

/**
 * Offsets in radians from a point, in float: the ESP32-P4 and similar
 * chips have hardware single precision only, and a float offset of up
 * to a few degrees is precise to a few centimeters.
 */
struct GeoOffset {
  float longitude, latitude;
};

/**
 * Vincenty's direct formula, like FindLatitudeLongitude(), for many
 * bearings from one point, in float.  It is about 1 m off.
 */
class FloatVincenty {
  float lat1, tan_u1, cos_u1, sin_u1;

public:
  explicit FloatVincenty(Angle latitude) noexcept
    :lat1(float(latitude.Radians())),
     tan_u1(float(1 - FLATTENING) * std::tan(lat1)),
     cos_u1(1 / std::hypot(1.f, tan_u1)),
     sin_u1(tan_u1 * cos_u1) {}

  GeoOffset operator()(float sin_alpha1, float cos_alpha1,
                       float distance) const noexcept {
    constexpr float f = FLATTENING;
    constexpr float u_factor =
      (EQUATOR_RADIUS * EQUATOR_RADIUS - POLE_RADIUS * POLE_RADIUS)
      / (POLE_RADIUS * POLE_RADIUS);

    const float sigma1 = std::atan2(tan_u1, cos_alpha1);
    const float sin_alpha = cos_u1 * sin_alpha1;
    const float cos_sq_alpha = 1 - Square(sin_alpha);
    const float u_sq = cos_sq_alpha * u_factor;
    const float A = 1 + u_sq / 16384 *
      (4096 + u_sq * (-768 + u_sq * (320 - 175 * u_sq)));
    const float B = u_sq / 1024 *
      (256 + u_sq * (-128 + u_sq * (74 - 47 * u_sq)));

    const float sigma0 = distance / (float(POLE_RADIUS) * A);
    float sigma = sigma0, sigma_p, sin_sigma, cos_sigma, cos_2_sigma_m;
    unsigned iterations = 0;
    do {
      cos_2_sigma_m = std::cos(2 * sigma1 + sigma);
      sin_sigma = std::sin(sigma);
      cos_sigma = std::cos(sigma);

      const float delta_sigma = B * sin_sigma *
        (cos_2_sigma_m + B / 4 *
         (cos_sigma * (-1 + 2 * Square(cos_2_sigma_m)) -
          B / 6 * cos_2_sigma_m * (-3 + 4 * Square(sin_sigma)) *
          (-3 + 4 * Square(cos_2_sigma_m))));

      sigma_p = sigma;
      sigma = sigma0 + delta_sigma;
    } while (std::fabs(sigma - sigma_p) > 1e-7f && ++iterations < 10);

    const float tmp = sin_u1 * sin_sigma - cos_u1 * cos_sigma * cos_alpha1;
    const float lat2 =
      std::atan2(sin_u1 * cos_sigma + cos_u1 * sin_sigma * cos_alpha1,
                 (1 - f) * std::hypot(sin_alpha, tmp));
    const float lambda =
      std::atan2(sin_sigma * sin_alpha1,
                 cos_u1 * cos_sigma - sin_u1 * sin_sigma * cos_alpha1);

    const float C = f / 16 * cos_sq_alpha * (4 + f * (4 - 3 * cos_sq_alpha));
    const float L = lambda - (1 - C) * f * sin_alpha *
      (sigma + C * sin_sigma *
       (cos_2_sigma_m + C * cos_sigma * (-1 + 2 * Square(cos_2_sigma_m))));

    return {L, lat2 - lat1};
  }
};

} // namespace

void
FindArcPoints(const GeoPoint &center, const double radius,
              const Angle start, const Angle step,
              const std::span<GeoPoint> points) noexcept
{
  static constexpr unsigned EXACT_STEPS = 10;

  const FloatVincenty vincenty{center.latitude};
  const float distance = radius;

  /* offsets on the plane that touches the ellipsoid at the center,
     plus the second-order terms of a sphere */
  constexpr float e2 = FLATTENING * (2 - FLATTENING);
  const auto [sin_lat, cos_lat] = center.latitude.SinCos();
  const float w = std::sqrt(1 - e2 * Square(float(sin_lat)));
  const float north = distance * w * w * w / (float(EQUATOR_RADIUS) * (1 - e2));
  const float east = distance * w / (float(EQUATOR_RADIUS) * float(cos_lat));
  const float tan_lat = sin_lat / cos_lat;
  const float d = distance * w / float(EQUATOR_RADIUS);

  const auto approximate = [&](float sin_b, float cos_b) noexcept {
    return GeoOffset{
      east * sin_b * (1 + d * cos_b * tan_lat),
      north * cos_b - 0.5f * Square(d) * Square(sin_b) * tan_lat,
    };
  };

  const auto to_point = [&](GeoOffset o) noexcept {
    GeoPoint p(center.longitude + Angle::Radians(o.longitude),
               center.latitude + Angle::Radians(o.latitude));
    p.Normalize();
    return p;
  };

  /* the bearing at step #i, and the approximation's error there */
  const auto exact = [&](unsigned i, float &sin_b, float &cos_b,
                         GeoOffset &error) noexcept {
    const auto [s, c] = (start + step * double(i)).SinCos();
    sin_b = s;
    cos_b = c;
    const GeoOffset e = vincenty(sin_b, cos_b, distance);
    const GeoOffset a = approximate(sin_b, cos_b);
    error = {e.longitude - a.longitude, e.latitude - a.latitude};
    return e;
  };

  const auto [step_sin_d, step_cos_d] = step.SinCos();
  const float step_sin = step_sin_d, step_cos = step_cos_d;
  const unsigned count = points.size();

  float sin_b, cos_b;
  GeoOffset error0;
  exact(0, sin_b, cos_b, error0);

  for (unsigned first = 0; first < count;) {
    const unsigned last = std::min(first + EXACT_STEPS, count);
    float last_sin, last_cos;
    GeoOffset error1;
    points[last - 1] = to_point(exact(last, last_sin, last_cos, error1));

    for (unsigned i = first + 1; i < last; ++i) {
      /* rotate by one step */
      const float next_sin = sin_b * step_cos + cos_b * step_sin;
      cos_b = cos_b * step_cos - sin_b * step_sin;
      sin_b = next_sin;

      const float t = float(i - first) / float(last - first);
      const GeoOffset a = approximate(sin_b, cos_b);
      points[i - 1] = to_point({
        a.longitude + error0.longitude + (error1.longitude - error0.longitude) * t,
        a.latitude + error0.latitude + (error1.latitude - error0.latitude) * t,
      });
    }

    sin_b = last_sin;
    cos_b = last_cos;
    error0 = error1;
    first = last;
  }
}
