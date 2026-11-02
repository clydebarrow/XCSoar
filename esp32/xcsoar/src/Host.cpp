// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Host.hpp"
#include "XCSoarEsp32.hpp"
#include "Device/Port/HostPort.hpp"
#include "Monitor/HostTrafficAlarm.hpp"
#include "Audio/HostVolume.hpp"
#include "ui/canvas/memory/HostDraw.hpp"
#include "ui/canvas/VirtualCanvas.hpp"
#include "ui/display/Display.hpp"
#include "ui/event/Globals.hpp"
#include "ui/event/Queue.hpp"

#include <algorithm>
#include <iterator>

namespace XCSoarEsp32 {

unsigned
BytesPerPixel() noexcept
{
#ifdef USE_RGB565
  return 2;
#else
  return 4;
#endif
}

static void
FlipToHost(const void *pixels, unsigned pitch,
           std::span<const PixelRect> dirty, void *ctx) noexcept
{
  const auto &config = *static_cast<const Config *>(ctx);

  /* TopWindow keeps at most 8; merge any more into one */
  Rect rects[8];
  unsigned n = 0;
  if (dirty.size() <= std::size(rects)) {
    for (const auto &r : dirty)
      rects[n++] = {unsigned(r.left), unsigned(r.top),
                    r.GetWidth(), r.GetHeight()};
  } else {
    rects[n++] = {0, 0, config.width, config.height};
  }

  config.frame_ready(pixels, pitch, rects, n, config.ctx);
}

static void
WaitForHost(void *ctx) noexcept
{
  const auto &config = *static_cast<const Config *>(ctx);
  config.frame_wait(config.ctx);
}

static const Config *host_config;

void
ConnectDisplay(const Config &config) noexcept
{
  host_config = &config;
  UI::Display::SetFlipHandler(FlipToHost,
                              config.frame_wait != nullptr ? WaitForHost : nullptr,
                              const_cast<Config *>(&config));

  /* the map buffers grow to the full screen in pan mode; allocating
     them at that size from the start keeps the heap unfragmented */
  VirtualCanvas::reserve_size = {config.width, config.height};
}

void
InjectPointer(PointerAction action, int x, int y) noexcept
{
  /* set by ScreenGlobalInit while XCSoar's screen exists */
  UI::EventQueue *queue = UI::event_queue;
  if (queue == nullptr)
    return;

  static constexpr UI::Event::Type types[] = {
    UI::Event::MOUSE_DOWN,
    UI::Event::MOUSE_MOTION,
    UI::Event::MOUSE_UP,
  };

  queue->Inject(UI::Event(types[unsigned(action)], PixelPoint(x, y)));
}

void
InjectTouches(std::span<const TouchPosition> fingers) noexcept
{
  UI::EventQueue *queue = UI::event_queue;
  if (queue == nullptr)
    return;

  /* the fingers of the last call, and whether this touch has had two */
  static unsigned last_count = 0;
  static bool multi = false;
  static PixelPoint last_point{};

  const unsigned count = std::min<std::size_t>(fingers.size(), 2);
  const PixelPoint p0 = count > 0
    ? PixelPoint(fingers[0].x, fingers[0].y)
    : last_point;

  if (last_count == 0 && count > 0)
    queue->Inject(UI::Event(UI::Event::MOUSE_DOWN, p0));

  if (count == 2) {
    multi = true;
    const PixelPoint p1(fingers[1].x, fingers[1].y);
    queue->Inject(UI::Event(last_count < 2
                            ? UI::Event::POINTER_DOWN
                            : UI::Event::POINTER_MOVE,
                            p0, p1));
  } else {
    if (last_count == 2)
      queue->Inject(UI::Event(UI::Event::POINTER_UP));
    if (count == 1 && last_count == 1)
      queue->Inject(UI::Event(UI::Event::MOUSE_MOTION, p0));
    if (count == 0 && last_count > 0) {
      /* a two-finger gesture is not a click */
      queue->Inject(UI::Event(multi
                              ? UI::Event::MOUSE_CANCEL
                              : UI::Event::MOUSE_UP, p0));
      multi = false;
    }
  }

  last_point = p0;
  last_count = count;
}

void
Feed(const char *name, std::span<const std::byte> data) noexcept
{
  HostPort::Feed(name, data);
}

} // namespace XCSoarEsp32

void
HostTrafficAlarm(FlarmTraffic::AlarmType level) noexcept
{
  const auto *config = XCSoarEsp32::host_config;
  if (config != nullptr && config->traffic_alarm != nullptr)
    config->traffic_alarm(unsigned(level), config->ctx);
}

bool
HostFill(void *buffer, unsigned pitch, unsigned height, const PixelRect &rect,
         Color color) noexcept
{
  const auto *config = XCSoarEsp32::host_config;
  if (config == nullptr || config->fill == nullptr)
    return false;

  const uint32_t argb = uint32_t(color.Alpha()) << 24 |
    uint32_t(color.Red()) << 16 | uint32_t(color.Green()) << 8 |
    color.Blue();
  return config->fill(buffer, pitch, height,
                      {unsigned(rect.left), unsigned(rect.top),
                       unsigned(rect.GetWidth()), unsigned(rect.GetHeight())},
                      argb, config->ctx);
}

bool
HostBlend(void *dest, unsigned dest_pitch, unsigned dest_height,
          const PixelRect &rect,
          const void *src, unsigned src_pitch, unsigned src_height,
          PixelPoint src_position, uint8_t alpha,
          bool white_transparent) noexcept
{
  const auto *config = XCSoarEsp32::host_config;
  if (config == nullptr || config->blend == nullptr)
    return false;

  return config->blend(dest, dest_pitch, dest_height,
                       {unsigned(rect.left), unsigned(rect.top),
                        unsigned(rect.GetWidth()), unsigned(rect.GetHeight())},
                       src, src_pitch, src_height,
                       unsigned(src_position.x), unsigned(src_position.y),
                       alpha, white_transparent, config->ctx);
}

void
HostVolume(unsigned percent, bool preview) noexcept
{
  const auto *config = XCSoarEsp32::host_config;
  if (config != nullptr && config->volume != nullptr)
    config->volume(percent, preview, config->ctx);
}
