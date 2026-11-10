// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoBounds.hpp"
#include "Geo/GeoPoint.hpp"

#include <cstdint>
#include <vector>

class Path;

/**
 * The track of a finished flight, read from its IGC file, to be drawn
 * on the map.  Points that lie almost on a straight line between
 * their neighbors are left out.
 */
class FlightTrack {
public:
  struct Point {
    GeoPoint location;

    /** GPS altitude, or pressure altitude without it [m] */
    int altitude;

    /** Seconds since the first fix */
    uint32_t time;
  };

  /**
   * The default distance a left-out point may be from the line that
   * replaces it [m].
   */
  static constexpr double DEFAULT_TOLERANCE = 20;

private:
  std::vector<Point> points;

  GeoBounds bounds = GeoBounds::Invalid();

  int min_altitude = 0, max_altitude = 0;

  /** Index of the highest point in #points */
  std::size_t highest = 0;

public:
  /**
   * Read the valid fixes of an IGC file and thin them out.
   *
   * Throws on I/O error.
   */
  void Load(Path path, double tolerance = DEFAULT_TOLERANCE);

  /**
   * Add a fix after the last one; call Finish() after the last.
   */
  void Append(const GeoPoint &location, int altitude,
              uint32_t time) noexcept;

  /**
   * Thin out the points and work out the bounds and the altitude
   * range.
   */
  void Finish(double tolerance = DEFAULT_TOLERANCE) noexcept;

  bool empty() const noexcept {
    return points.empty();
  }

  const std::vector<Point> &GetPoints() const noexcept {
    return points;
  }

  const GeoBounds &GetBounds() const noexcept {
    return bounds;
  }

  int GetMinAltitude() const noexcept {
    return min_altitude;
  }

  int GetMaxAltitude() const noexcept {
    return max_altitude;
  }

  /* the following may only be called when the track is not empty */

  const Point &GetTakeoff() const noexcept {
    return points.front();
  }

  const Point &GetLanding() const noexcept {
    return points.back();
  }

  const Point &GetHighest() const noexcept {
    return points[highest];
  }
};
