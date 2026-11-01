// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Concepts.hpp"
#include "ui/dim/Size.hpp"

#include <cstddef>
#include <new>

#ifdef ESP_PLATFORM
#include <esp_heap_caps.h>
#endif

/**
 * Allocate pixels for an image buffer.  On the ESP32 they are in
 * PSRAM and aligned to the cache line (64 bytes), so the PPA can fill
 * them (see HostFill()).
 */
template<typename T>
T *
AllocatePixels(std::size_t n)
{
#ifdef ESP_PLATFORM
  auto *p = static_cast<T *>(heap_caps_aligned_alloc(64, n * sizeof(T),
                                                     MALLOC_CAP_SPIRAM));
  if (p == nullptr)
    throw std::bad_alloc();
  return p;
#else
  return new T[n];
#endif
}

template<typename T>
void
FreePixels(T *p) noexcept
{
#ifdef ESP_PLATFORM
  heap_caps_free(p);
#else
  delete[] p;
#endif
}

/**
 * A reference to an image buffer (or a portion of it) that we can
 * write to.  This class does not allocate or free any memory, it
 * refers to a buffer owned by somebody else.
 */
template<AnyPixelTraits PixelTraits>
struct WritableImageBuffer {
  typedef typename PixelTraits::pointer pointer;
  typedef typename PixelTraits::rpointer rpointer;

  typedef typename PixelTraits::const_pointer const_pointer;

  rpointer data;

  std::size_t pitch;

  PixelSize size;

  static constexpr WritableImageBuffer<PixelTraits> Empty() noexcept {
    return { nullptr, 0, {} };
  }

  void Allocate(PixelSize _size) noexcept {
    const std::size_t i = PixelTraits::CalcIncrement(_size.width);
    data = AllocatePixels<typename PixelTraits::color_type>(i * _size.height);
    pitch = i * sizeof(typename PixelTraits::color_type);
    size = _size;
  }

  void Free() noexcept {
    FreePixels(data);
    data = nullptr;
  }

  constexpr bool Check(unsigned x, unsigned y) const noexcept {
    return x < size.width && y < size.height;
  }

  constexpr pointer At(unsigned x, unsigned y) noexcept {
    return PixelTraits::At(data, pitch, x, y);
  }

  constexpr const_pointer At(unsigned x, unsigned y) const noexcept {
    return PixelTraits::At(data, pitch, x, y);
  }
};

/**
 * A reference to an image buffer (or a portion of it) that is
 * read-only.  This class does not allocate or free any memory, it
 * refers to a buffer owned by somebody else.
 */
template<AnyPixelTraits PixelTraits>
struct ConstImageBuffer {
  typedef typename PixelTraits::const_pointer pointer;
  typedef typename PixelTraits::const_rpointer rpointer;

  rpointer data;

  std::size_t pitch;
  PixelSize size;

  ConstImageBuffer() noexcept = default;

  constexpr ConstImageBuffer(rpointer _data, std::size_t _pitch,
                             PixelSize _size) noexcept
    :data(_data), pitch(_pitch), size(_size) {}

  constexpr ConstImageBuffer(WritableImageBuffer<PixelTraits> other) noexcept
    :data(other.data), pitch(other.pitch), size(other.size) {}

  static constexpr WritableImageBuffer<PixelTraits> Empty() noexcept {
    return { nullptr, 0, 0, 0 };
  }

  constexpr bool Check(unsigned x, unsigned y) const noexcept {
    return x < size.width && y < size.height;
  }

  constexpr pointer At(unsigned x, unsigned y) const noexcept {
    return PixelTraits::At(data, pitch, x, y);
  }
};
