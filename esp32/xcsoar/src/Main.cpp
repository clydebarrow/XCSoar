// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCSoarEsp32.hpp"
#include "Look/FontDescription.hpp"
#include "Screen/Debug.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Color.hpp"
#include "ui/canvas/Font.hpp"
#include "ui/canvas/Pen.hpp"
#include "ui/canvas/custom/TopCanvas.hpp"
#include "ui/display/Display.hpp"

#include <fmt/format.h>

#include <chrono>
#include <cmath>
#include <exception>
#include <numbers>
#include <thread>

#include <stdio.h>

using std::chrono::steady_clock;

namespace XCSoarEsp32 {

static void
FlipToHost(ConstImageBuffer<ActivePixelTraits> frame, void *ctx) noexcept
{
  const auto &config = *static_cast<const Config *>(ctx);
  config.frame_ready(frame.data, frame.pitch, config.ctx);
}

static PixelPoint
Polar(PixelPoint centre, double radius, double angle) noexcept
{
  return centre + PixelPoint(int(radius * std::sin(angle)),
                             int(-radius * std::cos(angle)));
}

/**
 * Phase 1 test scene: a map-like view with a rotating aircraft
 * symbol, a thermal marker and text, to prove the canvas, the font
 * renderer and the frame handoff.
 */
static void
DrawDemo(Canvas &canvas, unsigned frame, const Font &big_font,
         const Font &small_font, unsigned fps) noexcept
{
  const PixelRect rc = canvas.GetRect();
  const PixelPoint centre = rc.GetCenter();
  const unsigned scale = std::min(rc.GetWidth(), rc.GetHeight()) / 2;
  const double heading = frame * (std::numbers::pi / 90);

  canvas.ClearWhite();

  /* range rings */
  canvas.SelectHollowBrush();
  canvas.Select(Pen(1, COLOR_GRAY));
  for (unsigned i = 1; i <= 3; ++i)
    canvas.DrawCircle(centre, scale * i / 4);

  /* a thermal circling to the right of the aircraft */
  canvas.Select(Pen(2, COLOR_RED));
  canvas.Select(Brush(COLOR_INVERSE_RED));
  canvas.DrawCircle(Polar(centre, scale / 2., heading + 1), scale / 10);

  /* track line */
  canvas.Select(Pen(3, COLOR_BLUE));
  canvas.DrawLine(centre, Polar(centre, scale * 0.9, heading));

  /* aircraft symbol */
  const BulkPixelPoint glider[] = {
    Polar(centre, scale / 6., heading),
    Polar(centre, scale / 6., heading + 2.5),
    centre,
    Polar(centre, scale / 6., heading - 2.5),
  };
  canvas.SelectBlackPen(2);
  canvas.SelectBlackBrush();
  canvas.DrawPolygon(glider, std::size(glider));

  canvas.SetBackgroundTransparent();
  canvas.SetTextColor(COLOR_BLACK);
  canvas.Select(big_font);
  canvas.DrawText({rc.left + 10, rc.top + 10}, "XCSoar on ESP32");

  canvas.Select(small_font);
  const auto info = fmt::format("Frame {}  {} fps  Track {:03}",
                                frame, fps,
                                unsigned(frame * 2 % 360));
  canvas.DrawText({rc.left + 10, rc.bottom - 10 - int(small_font.GetHeight())},
                  info);
}

bool
Run(const Config &config) noexcept
try {
  /* the parts of ScreenGlobalInit that do not need an event queue */
  Font::Initialise();
  ScreenInitialized();

  Font big_font, small_font;
  big_font.Load(FontDescription(config.height / 12, true));
  small_font.Load(FontDescription(config.height / 20));

  UI::Display display;
  TopCanvas screen(display, PixelSize(config.width, config.height),
                   FlipToHost, const_cast<Config *>(&config));

  const auto interval = std::chrono::milliseconds(config.frame_interval_ms);
  auto next = steady_clock::now();
  auto fps_start = next;
  unsigned fps = 0, fps_frames = 0;

  for (unsigned frame = 0;
       config.max_frames == 0 || frame < config.max_frames; ++frame) {
    {
      Canvas canvas = screen.Lock();
      DrawDemo(canvas, frame, big_font, small_font, fps);
      screen.Unlock();
    }

    screen.Flip();

    ++fps_frames;
    const auto now = steady_clock::now();
    if (now - fps_start >= std::chrono::seconds(1)) {
      fps = fps_frames;
      fps_frames = 0;
      fps_start = now;
    }

    next += interval;
    if (next > now)
      std::this_thread::sleep_for(next - now);
    else
      next = now;
  }

  big_font.Destroy();
  small_font.Destroy();
  ScreenDeinitialized();
  Font::Deinitialise();
  return true;
} catch (...) {
  try {
    throw;
  } catch (const std::exception &e) {
    fprintf(stderr, "XCSoar: %s\n", e.what());
  } catch (...) {
    fprintf(stderr, "XCSoar: unknown error\n");
  }
  return false;
}

} // namespace XCSoarEsp32
