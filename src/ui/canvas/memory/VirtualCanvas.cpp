// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/VirtualCanvas.hpp"

#include <cassert>

VirtualCanvas::VirtualCanvas(PixelSize new_size) noexcept
{
  Create(new_size);
}

VirtualCanvas::VirtualCanvas([[maybe_unused]] const Canvas &canvas,
                             PixelSize new_size) noexcept
{
  Create(new_size);
}

static constexpr std::size_t
CountPixels(PixelSize size) noexcept
{
  return std::size_t(ActivePixelTraits::CalcIncrement(size.width)) *
    size.height;
}

void
VirtualCanvas::Create(PixelSize new_size) noexcept
{
  const std::size_t n = CountPixels(new_size);
  if (buffer.data == nullptr || n > capacity) {
    Destroy();

    capacity = n;
    if (const std::size_t reserve = CountPixels(reserve_size);
        n < reserve && n * 2 >= reserve)
      capacity = reserve;

    buffer.data = new ActivePixelTraits::color_type[capacity];
  }

  buffer.pitch = ActivePixelTraits::CalcIncrement(new_size.width) *
    sizeof(ActivePixelTraits::color_type);
  buffer.size = new_size;
}

void
VirtualCanvas::Create([[maybe_unused]] const Canvas &canvas,
                      PixelSize new_size) noexcept
{
#if defined(ENABLE_OPENGL)
  assert(canvas.IsDefined());
#endif

  Create(new_size);
}

void
VirtualCanvas::Destroy() noexcept
{
  buffer.Free();
  capacity = 0;
}
