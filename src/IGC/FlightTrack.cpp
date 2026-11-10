// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FlightTrack.hpp"
#include "IGCParser.hpp"
#include "IGCExtensions.hpp"
#include "IGCFix.hpp"
#include "Geo/FAISphere.hpp"
#include "io/FileLineReader.hpp"
#include "system/Path.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

static constexpr uint32_t DAY_SECONDS = 24 * 60 * 60;

void
FlightTrack::Load(Path path, double tolerance)
{
  points.clear();

  FileLineReaderA reader(path);
  IGCExtensions extensions;
  extensions.clear();

  uint32_t first = 0, day_offset = 0, previous = 0;
  bool have_first = false;

  char *line;
  while ((line = reader.ReadLine()) != nullptr) {
    if (line[0] == 'I') {
      IGCParseExtensions(line, extensions);
      continue;
    }

    if (line[0] != 'B')
      continue;

    IGCFix fix;
    if (!IGCParseFix(line, extensions, fix) || !fix.gps_valid)
      continue;

    uint32_t time = fix.time.GetSecondOfDay() + day_offset;
    if (have_first && time + DAY_SECONDS / 2 < previous) {
      /* past midnight */
      day_offset += DAY_SECONDS;
      time += DAY_SECONDS;
    }

    if (!have_first) {
      first = time;
      have_first = true;
    } else if (time <= previous)
      /* a repeated or backwards fix */
      continue;

    previous = time;

    const int altitude = fix.gps_altitude != 0
      ? fix.gps_altitude
      : fix.pressure_altitude;
    Append(fix.location, altitude, time - first);
  }

  Finish(tolerance);
}

void
FlightTrack::Append(const GeoPoint &location, int altitude,
                    uint32_t time) noexcept
{
  points.push_back({location, altitude, time});
}

namespace {

/**
 * Positions in metres on a plane touching the earth near the track,
 * which is close enough for the distances of thinning.
 */
class LocalPlane {
  GeoPoint origin;
  double x_scale;

public:
  explicit LocalPlane(GeoPoint _origin) noexcept
    :origin(_origin),
     x_scale(origin.latitude.cos() * FAISphere::REARTH) {}

  std::pair<double, double> Project(const GeoPoint &p) const noexcept {
    return {
      (p.longitude - origin.longitude).AsDelta().Radians() * x_scale,
      (p.latitude - origin.latitude).Radians() * FAISphere::REARTH,
    };
  }
};

/**
 * The distance of #p from the line segment from #a to #b.
 */
[[gnu::const]]
static double
SegmentDistance(std::pair<double, double> p, std::pair<double, double> a,
                std::pair<double, double> b) noexcept
{
  const double dx = b.first - a.first, dy = b.second - a.second;
  const double px = p.first - a.first, py = p.second - a.second;
  const double length2 = dx * dx + dy * dy;

  double t = length2 > 0 ? (px * dx + py * dy) / length2 : 0;
  t = std::clamp(t, 0., 1.);

  return std::hypot(px - t * dx, py - t * dy);
}

} // anonymous namespace

void
FlightTrack::Finish(double tolerance) noexcept
{
  bounds = GeoBounds::Invalid();
  highest = 0;
  min_altitude = max_altitude = 0;

  if (points.empty())
    return;

  if (points.size() > 2) {
    /* Douglas-Peucker, with a stack instead of recursion: keep the
       point furthest from the line between the ends of a range when
       it is further than the tolerance, then do the same for both
       halves */
    const LocalPlane plane(points.front().location);
    std::vector<std::pair<double, double>> xy;
    xy.reserve(points.size());
    for (const auto &p : points)
      xy.push_back(plane.Project(p.location));

    std::vector<bool> keep(points.size(), false);
    keep.front() = keep.back() = true;

    std::vector<std::pair<std::size_t, std::size_t>> ranges;
    ranges.emplace_back(0, points.size() - 1);
    while (!ranges.empty()) {
      const auto [a, b] = ranges.back();
      ranges.pop_back();

      double max_distance = tolerance;
      std::size_t furthest = 0;
      for (std::size_t i = a + 1; i < b; ++i) {
        const double d = SegmentDistance(xy[i], xy[a], xy[b]);
        if (d > max_distance) {
          max_distance = d;
          furthest = i;
        }
      }

      if (furthest != 0) {
        keep[furthest] = true;
        ranges.emplace_back(a, furthest);
        ranges.emplace_back(furthest, b);
      }
    }

    /* the highest point stays, whatever its position */
    const auto top = std::max_element(points.begin(), points.end(),
                                      [](const Point &x, const Point &y){
                                        return x.altitude < y.altitude;
                                      });
    keep[std::distance(points.begin(), top)] = true;

    std::size_t n = 0;
    for (std::size_t i = 0; i < points.size(); ++i)
      if (keep[i])
        points[n++] = points[i];
    points.resize(n);
    points.shrink_to_fit();
  }

  min_altitude = max_altitude = points.front().altitude;
  for (std::size_t i = 0; i < points.size(); ++i) {
    const auto &p = points[i];
    bounds.Extend(p.location);
    min_altitude = std::min(min_altitude, p.altitude);
    if (p.altitude > max_altitude) {
      max_altitude = p.altitude;
      highest = i;
    }
  }
}
