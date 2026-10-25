// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCSoarEsp32.hpp"
#include "Look/FontDescription.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Color.hpp"
#include "ui/canvas/Font.hpp"
#include "ui/canvas/Pen.hpp"
#include "ui/event/Globals.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include "ui/event/poll/Queue.hpp"
#include "ui/window/Init.hpp"
#include "ui/window/SingleWindow.hpp"

#include <fmt/format.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <exception>
#include <numbers>

#include <stdio.h>

namespace XCSoarEsp32 {

static PixelPoint
Polar(PixelPoint centre, double radius, double angle) noexcept
{
  return centre + PixelPoint(int(radius * std::sin(angle)),
                             int(-radius * std::cos(angle)));
}

/**
 * The test scene: a map-like view with a rotating aircraft symbol, a
 * thermal marker and text, to prove the canvas, the font renderer
 * and the frame handoff.
 */
static void
DrawDemo(Canvas &canvas, unsigned frame, const Font &big_font,
         const Font &small_font, unsigned fps, unsigned taps) noexcept
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
  const auto info = fmt::format("Frame {}  {} fps  Track {:03}  Taps {}",
                                frame, fps,
                                unsigned(frame * 2 % 360), taps);
  canvas.DrawText({rc.left + 10, rc.bottom - 10 - int(small_font.GetHeight())},
                  info);
}

/**
 * The test window: the animated scene, the path of the current touch
 * and a button that pauses the animation, to prove the event loop,
 * its timers and touch input.
 */
class DemoWindow final : public UI::SingleWindow {
  const Config &config;
  const Font &big_font, &small_font;

  UI::PeriodicTimer animation_timer{[this]{ OnAnimationTimer(); }};
  unsigned steps = 0, frame = 0;
  bool paused = false;

  /* frame rate, counted over the last second */
  std::chrono::steady_clock::time_point fps_start;
  unsigned fps = 0, fps_frames = 0;

  /* the latest points of the touch in progress */
  std::array<BulkPixelPoint, 64> trail;
  unsigned trail_length = 0;
  bool touching = false;
  unsigned taps = 0;

public:
  DemoWindow(UI::Display &display, const Config &_config,
             const Font &_big_font, const Font &_small_font) noexcept
    :UI::SingleWindow(display), config(_config),
     big_font(_big_font), small_font(_small_font) {}

  void Start() noexcept {
    fps_start = std::chrono::steady_clock::now();
    animation_timer.Schedule(
      std::chrono::milliseconds(config.frame_interval_ms));
  }

private:
  [[gnu::pure]]
  PixelRect GetPauseButtonRect() const noexcept {
    const PixelRect rc = GetClientRect();
    const int size = int(big_font.GetHeight()) * 2;
    return {
      rc.right - 10 - size * 2, rc.top + 10,
      rc.right - 10, rc.top + 10 + size,
    };
  }

  void AddTrailPoint(PixelPoint p) noexcept {
    if (trail_length == trail.size()) {
      std::move(trail.begin() + 1, trail.end(), trail.begin());
      --trail_length;
    }

    trail[trail_length++] = p;
  }

  void OnAnimationTimer() noexcept {
    if (config.max_frames > 0 && ++steps >= config.max_frames) {
      UI::event_queue->Quit();
      return;
    }

    if (!paused)
      ++frame;

    Invalidate();
  }

  void DrawPauseButton(Canvas &canvas) noexcept {
    const PixelRect rc = GetPauseButtonRect();
    canvas.DrawFilledRectangle(rc, paused ? COLOR_ORANGE : COLOR_GREEN);
    canvas.DrawOutlineRectangle(rc, COLOR_BLACK);

    canvas.Select(small_font);
    canvas.SetTextColor(COLOR_BLACK);
    const std::string_view label = paused ? "Run" : "Pause";
    const PixelSize size = small_font.TextSize(label);
    canvas.DrawText(rc.GetCenter() -
                    PixelPoint(size.width / 2, size.height / 2),
                    label);
  }

protected:
  void OnPaint(Canvas &canvas) noexcept override {
    DrawDemo(canvas, frame, big_font, small_font, fps, taps);
    DrawPauseButton(canvas);

    if (trail_length > 1) {
      canvas.Select(Pen(5, COLOR_MAGENTA));
      canvas.DrawPolyline(trail.data(), trail_length);
    }

    if (touching) {
      canvas.Select(Pen(3, COLOR_MAGENTA));
      canvas.SelectHollowBrush();
      canvas.DrawCircle(trail[trail_length - 1], 40);
    }

    UI::SingleWindow::OnPaint(canvas);

    ++fps_frames;
    const auto now = std::chrono::steady_clock::now();
    if (now - fps_start >= std::chrono::seconds(1)) {
      fps = fps_frames;
      fps_frames = 0;
      fps_start = now;
    }
  }

  bool OnMouseDown(PixelPoint p) noexcept override {
    ++taps;
    touching = true;
    trail_length = 0;
    AddTrailPoint(p);

    if (GetPauseButtonRect().Contains(p))
      paused = !paused;

    Invalidate();
    return true;
  }

  bool OnMouseMove(PixelPoint p, unsigned) noexcept override {
    if (!touching)
      return false;

    AddTrailPoint(p);
    Invalidate();
    return true;
  }

  bool OnMouseUp(PixelPoint p) noexcept override {
    if (!touching)
      return false;

    touching = false;
    AddTrailPoint(p);
    Invalidate();
    return true;
  }
};

static void
FlipToHost(const void *pixels, unsigned pitch, void *ctx) noexcept
{
  const auto &config = *static_cast<const Config *>(ctx);
  config.frame_ready(pixels, pitch, config.ctx);
}

/**
 * The queue that InjectPointer() writes to, while Run() is running.
 */
static std::atomic<UI::EventQueue *> injection_queue{nullptr};

void
InjectPointer(PointerAction action, int x, int y) noexcept
{
  UI::EventQueue *queue = injection_queue.load();
  if (queue == nullptr)
    return;

  static constexpr UI::Event::Type types[] = {
    UI::Event::MOUSE_DOWN,
    UI::Event::MOUSE_MOTION,
    UI::Event::MOUSE_UP,
  };

  queue->Inject(UI::Event(types[unsigned(action)], PixelPoint(x, y)));
}

bool
Run(const Config &config) noexcept
try {
  ScreenGlobalInit screen_init;
  screen_init.GetDisplay().SetFlipHandler(FlipToHost,
                                          const_cast<Config *>(&config));

  Font big_font, small_font;
  big_font.Load(FontDescription(config.height / 12, true));
  small_font.Load(FontDescription(config.height / 20));

  {
    DemoWindow window(screen_init.GetDisplay(), config, big_font, small_font);
    window.Create("XCSoar", PixelSize(config.width, config.height));
    window.Start();

    injection_queue = &screen_init.GetEventQueue();
    window.RunEventLoop();
    injection_queue = nullptr;
  }

  big_font.Destroy();
  small_font.Destroy();
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
