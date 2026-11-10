// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "IGC/FlightTrack.hpp"
#include "Geo/FAISphere.hpp"
#include "system/Path.hpp"
#include "TestUtil.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

static GeoPoint
Offset(GeoPoint origin, double east, double north) noexcept
{
  return {
    origin.longitude + FAISphere::EarthDistanceToAngle(east) /
      origin.latitude.cos(),
    origin.latitude + FAISphere::EarthDistanceToAngle(north),
  };
}

static const GeoPoint origin{Angle::Degrees(152.45), Angle::Degrees(-31.25)};

static void
TestStraightLine()
{
  FlightTrack track;
  for (unsigned i = 0; i < 100; ++i)
    track.Append(Offset(origin, i * 30., 0), 500, i);
  track.Finish();

  /* the ends only; all altitudes are the same, so the highest point
     is the first */
  ok1(track.GetPoints().size() == 2);
  ok1(track.GetTakeoff().time == 0);
  ok1(track.GetLanding().time == 99);
  ok1(track.GetMinAltitude() == 500);
  ok1(track.GetMaxAltitude() == 500);
}

static void
TestCorner()
{
  FlightTrack track;
  for (unsigned i = 0; i <= 50; ++i)
    track.Append(Offset(origin, i * 20., 0), 400 + i, i);
  for (unsigned i = 1; i <= 50; ++i)
    track.Append(Offset(origin, 1000, i * 20.), 450 - i, 50 + i);
  track.Finish();

  /* start, the corner and the end; the corner is also the highest */
  ok1(track.GetPoints().size() == 3);
  ok1(track.GetHighest().time == 50);
  ok1(track.GetMaxAltitude() == 450);
  ok1(track.GetMinAltitude() == 400);

  const GeoBounds &bounds = track.GetBounds();
  ok1(bounds.IsValid());
  ok1(bounds.GetGeoWidth() > 990 && bounds.GetGeoWidth() < 1010);
  ok1(bounds.GetGeoHeight() > 990 && bounds.GetGeoHeight() < 1010);
}

static void
TestHighestKept()
{
  /* a small bump in the middle of a line, closer to it than the
     tolerance, but the highest point */
  FlightTrack track;
  for (unsigned i = 0; i < 21; ++i)
    track.Append(Offset(origin, i * 50., i == 10 ? 5. : 0.),
                 i == 10 ? 900 : 300, i);
  track.Finish();

  ok1(track.GetPoints().size() == 3);
  ok1(track.GetHighest().altitude == 900);
  ok1(track.GetHighest().time == 10);
}

/**
 * The distance of #p from the nearest segment of the thinned track,
 * on a plane touching the earth at #p.
 */
static double
DistanceToTrack(const FlightTrack &track, GeoPoint p)
{
  const double x_scale = p.latitude.cos() * FAISphere::REARTH;
  auto xy = [&](GeoPoint g){
    return std::pair{
      (g.longitude - p.longitude).AsDelta().Radians() * x_scale,
      (g.latitude - p.latitude).Radians() * double(FAISphere::REARTH),
    };
  };

  const auto &points = track.GetPoints();
  double best = p.Distance(points.front().location);
  for (std::size_t i = 1; i < points.size(); ++i) {
    const auto [ax, ay] = xy(points[i - 1].location);
    const auto [bx, by] = xy(points[i].location);
    const double dx = bx - ax, dy = by - ay;
    const double length2 = dx * dx + dy * dy;
    double t = length2 > 0 ? (-ax * dx - ay * dy) / length2 : 0;
    t = std::clamp(t, 0., 1.);
    best = std::min(best, std::hypot(ax + t * dx, ay + t * dy));
  }

  return best;
}

static void
TestLoad(Path path)
{
  FlightTrack all, thinned;
  all.Load(path, 0);
  thinned.Load(path);

  ok1(!thinned.empty());
  ok1(thinned.GetPoints().size() < all.GetPoints().size());
  ok1(thinned.GetTakeoff().time == 0);
  ok1(thinned.GetLanding().time == all.GetLanding().time);
  ok1(thinned.GetMaxAltitude() == all.GetMaxAltitude());
  ok1(thinned.GetMinAltitude() >= all.GetMinAltitude());

  /* every fix is within the tolerance of the thinned track */
  bool within = true;
  for (const auto &p : all.GetPoints())
    if (DistanceToTrack(thinned, p.location) >
        FlightTrack::DEFAULT_TOLERANCE + 1)
      within = false;
  ok1(within);
}

static void
TestMidnight()
{
  FlightTrack track;
  track.Load(Path("test/data/flight_times_midnight.igc"));
  ok1(!track.empty());
  /* times keep increasing past midnight */
  bool increasing = true;
  const auto &points = track.GetPoints();
  for (std::size_t i = 1; i < points.size(); ++i)
    if (points[i].time <= points[i - 1].time)
      increasing = false;
  ok1(increasing);
}

int
main()
{
  plan_tests(5 + 7 + 3 + 7 + 7 + 2);

  TestStraightLine();
  TestCorner();
  TestHighestKept();
  TestLoad(Path("test/data/0asljd01.igc"));
  TestLoad(Path("test/data/flight_times.igc"));
  TestMidnight();

  return exit_status();
}
