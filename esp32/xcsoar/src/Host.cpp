// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Host.hpp"
#include "XCSoarEsp32.hpp"
#include "ui/canvas/VirtualCanvas.hpp"
#include "ui/display/Display.hpp"
#include "ui/event/Globals.hpp"
#include "ui/event/Queue.hpp"

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

void
ConnectDisplay(const Config &config) noexcept
{
  UI::Display::SetFlipHandler(FlipToHost, const_cast<Config *>(&config));

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

} // namespace XCSoarEsp32
