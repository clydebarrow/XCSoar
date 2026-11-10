// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FlightTrackRenderer.hpp"
#include "IGC/FlightTrack.hpp"
#include "Projection/WindowProjection.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Ramp.hpp"
#include "ui/dim/BulkPoint.hpp"
#include "Screen/Layout.hpp"
#include "util/Macros.hpp"

#include <algorithm>
#include <vector>

/* the altitude colors of the trail */
static constexpr ColorRampEntry altitude_colors[] = {
  {  0, { 0xff, 0x00, 0x00 }},
  { 50, { 0xff, 0xff, 0x00 }},
  {100, { 0x00, 0xff, 0x00 }},
  {150, { 0x00, 0xff, 0xff }},
  {200, { 0x00, 0x00, 0xff }},
};

static constexpr ColorRamp altitude_ramp = {
  false, ARRAY_SIZE(altitude_colors), altitude_colors, nullptr,
};

FlightTrackRenderer::FlightTrackRenderer() noexcept
{
  const unsigned width = Layout::ScalePenWidth(3);
  for (unsigned i = 0; i < NUM_COLORS; ++i)
    pens[i].Create(width,
                   Color(ColorRampLookup(i * 200 / (NUM_COLORS - 1),
                                         &altitude_ramp)));

  outline_pen.Create(Layout::ScalePenWidth(2), COLOR_BLACK);
  takeoff_brush.Create(COLOR_GREEN);
  landing_brush.Create(COLOR_RED);
  highest_brush.Create(COLOR_BLUE);
}

/**
 * Is the line from #a to #b certainly outside #rc, because both ends
 * are beyond the same edge?
 */
[[gnu::const]]
static bool
IsOutside(PixelPoint a, PixelPoint b, const PixelRect &rc) noexcept
{
  return (a.x < rc.left && b.x < rc.left) ||
    (a.x >= rc.right && b.x >= rc.right) ||
    (a.y < rc.top && b.y < rc.top) ||
    (a.y >= rc.bottom && b.y >= rc.bottom);
}

void
FlightTrackRenderer::Draw(Canvas &canvas, const WindowProjection &projection,
                          const FlightTrack &track) const noexcept
{
  const auto &points = track.GetPoints();
  if (points.empty() || !projection.GetScreenBounds().Overlaps(track.GetBounds()))
    return;

  const int min_altitude = track.GetMinAltitude();
  const int range = std::max(track.GetMaxAltitude() - min_altitude, 1);
  auto color_index = [&](int altitude) noexcept {
    return unsigned((altitude - min_altitude) * int(NUM_COLORS - 1) / range);
  };

  /* a margin, so that the line is not cut where it is thick */
  PixelRect clip = canvas.GetRect();
  clip.Grow(Layout::Scale(8));

  /* consecutive segments of one color are drawn as one polyline */
  std::vector<BulkPixelPoint> run;
  unsigned run_color = 0;
  auto flush = [&]() {
    if (run.size() >= 2) {
      canvas.Select(pens[run_color]);
      canvas.DrawPolyline(run.data(), run.size());
    }
    run.clear();
  };

  PixelPoint previous = projection.GeoToScreen(points.front().location);
  for (std::size_t i = 1; i < points.size(); ++i) {
    const PixelPoint current = projection.GeoToScreen(points[i].location);
    if (IsOutside(previous, current, clip)) {
      flush();
      previous = current;
      continue;
    }

    const unsigned color = color_index(points[i].altitude);
    if (run.empty() || color != run_color) {
      flush();
      run_color = color;
      run.emplace_back(previous);
    }

    if (current != previous)
      run.emplace_back(current);
    previous = current;
  }

  flush();

  /* the markers */
  const unsigned radius = Layout::Scale(6);
  canvas.Select(outline_pen);

  auto marker = [&](const GeoPoint &location, const Brush &brush) {
    if (auto p = projection.GeoToScreenIfVisible(location)) {
      canvas.Select(brush);
      canvas.DrawCircle(*p, radius);
    }
  };

  marker(track.GetTakeoff().location, takeoff_brush);
  marker(track.GetLanding().location, landing_brush);
  marker(track.GetHighest().location, highest_brush);
}
