// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapWindow.hpp"
#include "Renderer/TrailRenderer.hpp"
#include "Computer/GlideComputer.hpp"
#include "IGC/FlightTrack.hpp"

void
MapWindow::RenderTrail(Canvas &canvas, const PixelPoint aircraft_pos) noexcept
{
  auto min_time = std::max(Basic().time - std::chrono::minutes{10},
                           TimeStamp{});

  DrawTrail(canvas, aircraft_pos, min_time);
}

void
MapWindow::DrawTrail(Canvas &canvas, const PixelPoint aircraft_pos,
                     TimeStamp min_time,
                     bool enable_traildrift) noexcept
{
  if (glide_computer)
    trail_renderer.Draw(canvas, glide_computer->GetTraceComputer(),
                        render_projection, min_time,
                        enable_traildrift, aircraft_pos,
                        Basic(), Calculated(), GetMapSettings().trail);
}

void
MapWindow::SetFlightTrack(std::shared_ptr<const FlightTrack> track) noexcept
{
  {
#ifndef ENABLE_OPENGL
    const std::lock_guard lock{mutex};
#endif

    if (track != nullptr && flight_track_renderer == nullptr)
      flight_track_renderer = std::make_unique<FlightTrackRenderer>();

    flight_track = std::move(track);
  }
}

void
MapWindow::RenderFlightTrack(Canvas &canvas) noexcept
{
  if (flight_track != nullptr && flight_track_renderer != nullptr)
    flight_track_renderer->Draw(canvas, render_projection, *flight_track);
}
