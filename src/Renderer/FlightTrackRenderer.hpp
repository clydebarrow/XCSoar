// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Pen.hpp"
#include "ui/canvas/Brush.hpp"

#include <array>

class Canvas;
class WindowProjection;
class FlightTrack;

/**
 * Draws the track of a finished flight on the map: a line colored by
 * altitude, from red low to blue high, with markers at takeoff,
 * landing and the highest point.
 */
class FlightTrackRenderer {
  static constexpr unsigned NUM_COLORS = 8;

  std::array<Pen, NUM_COLORS> pens;

  Pen outline_pen;
  Brush takeoff_brush, landing_brush, highest_brush;

public:
  FlightTrackRenderer() noexcept;

  void Draw(Canvas &canvas, const WindowProjection &projection,
            const FlightTrack &track) const noexcept;
};
