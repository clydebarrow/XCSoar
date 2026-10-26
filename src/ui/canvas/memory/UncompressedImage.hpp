// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Concepts.hpp"
#include "Buffer.hpp"
#include "ui/canvas/custom/UncompressedImage.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>

struct RGBPixelReader {
  const uint8_t *p;

  template<AnyPixelTraits PixelTraits>
  typename PixelTraits::color_type Read(PixelTraits) noexcept {
    const uint8_t r = *p++, g = *p++, b = *p++;
    return typename PixelTraits::color_type(r, g, b);
  }
};

/**
 * Read RGBA pixels (4 bytes each), discarding the alpha channel.
 * The memory canvas has no alpha blending, so alpha is dropped
 * and transparent areas composite against an implicit white
 * background.
 */
struct RGBAPixelReader {
  const uint8_t *p;

  template<AnyPixelTraits PixelTraits>
  typename PixelTraits::color_type Read(PixelTraits) noexcept {
    const uint8_t r = *p++, g = *p++, b = *p++;
    const uint8_t a = *p++;

    /* Pre-multiply against white so that transparent areas
       render as white instead of black. */
    const uint8_t rb = r + (255 - a) * (255 - r) / 255;
    const uint8_t gb = g + (255 - a) * (255 - g) / 255;
    const uint8_t bb = b + (255 - a) * (255 - b) / 255;
    return typename PixelTraits::color_type(rb, gb, bb);
  }
};

struct GrayPixelReader {
  const uint8_t *p;

  template<AnyPixelTraits PixelTraits>
  typename PixelTraits::color_type Read(PixelTraits) noexcept {
    const uint8_t l = *p++;
    return typename PixelTraits::color_type(l, l, l);
  }
};

template<AnyPixelTraits PixelTraits, typename Reader>
static inline void
ConvertLine(typename PixelTraits::rpointer dest, Reader src,
            unsigned n) noexcept
{
  for (unsigned i = 0; i < n; ++i, dest = PixelTraits::Next(dest, 1))
    PixelTraits::WritePixel(dest, src.Read(PixelTraits()));
}

template<AnyPixelTraits PixelTraits, typename Format>
static inline void
ConvertImage(WritableImageBuffer<PixelTraits> buffer,
             const uint8_t *src, int src_pitch, bool flipped) noexcept
{
  typename PixelTraits::rpointer dest = buffer.data;

  if (flipped) {
    src += src_pitch * (buffer.size.height - 1);
    src_pitch = -src_pitch;
  }

  for (unsigned i = 0; i < buffer.size.height; ++i,
         dest = PixelTraits::NextRow(dest, buffer.pitch, 1),
         src += src_pitch)
    ConvertLine<PixelTraits>(dest, Format{src}, buffer.size.width);
}

/**
 * Convert an #UncompressedImage to a SDL_Surface.
 *
 * @return the new SDL_Surface object or nullptr on error
 */
template<AnyPixelTraits PixelTraits>
static inline void
ImportSurface(WritableImageBuffer<PixelTraits> &buffer,
              const UncompressedImage &uncompressed) noexcept
{
  buffer.Allocate(uncompressed.GetSize());

  switch (uncompressed.GetFormat()) {
  case UncompressedImage::Format::INVALID:
    assert(false);
    gcc_unreachable();

  case UncompressedImage::Format::RGB:
    ConvertImage<PixelTraits,
                 RGBPixelReader>(buffer,
                                 (const uint8_t *)uncompressed.GetData(),
                                 uncompressed.GetPitch(),
                                 uncompressed.IsFlipped());
    break;

  case UncompressedImage::Format::RGBA:
    ConvertImage<PixelTraits,
                 RGBAPixelReader>(buffer,
                                  (const uint8_t *)uncompressed.GetData(),
                                  uncompressed.GetPitch(),
                                  uncompressed.IsFlipped());
    break;

  case UncompressedImage::Format::GRAY:
    ConvertImage<PixelTraits,
                 GrayPixelReader>(buffer,
                                  (const uint8_t *)uncompressed.GetData(),
                                  uncompressed.GetPitch(),
                                  uncompressed.IsFlipped());
    break;
  }
}

/**
 * Copy the alpha channel of an RGBA image, one byte per pixel with no
 * padding, top row first.  Returns nullptr if the image has no
 * transparent pixels.
 */
static inline std::unique_ptr<uint8_t[]>
ImportAlpha(const UncompressedImage &uncompressed) noexcept
{
  if (uncompressed.GetFormat() != UncompressedImage::Format::RGBA)
    return nullptr;

  const unsigned width = uncompressed.GetWidth();
  const unsigned height = uncompressed.GetHeight();
  const auto *src = (const uint8_t *)uncompressed.GetData();
  const std::size_t pitch = uncompressed.GetPitch();

  std::unique_ptr<uint8_t[]> alpha;
  for (unsigned y = 0; y < height; ++y) {
    const uint8_t *row = src + y * pitch;
    const unsigned dest_y = uncompressed.IsFlipped() ? height - 1 - y : y;
    for (unsigned x = 0; x < width; ++x) {
      const uint8_t a = row[x * 4 + 3];
      if (a != 0xff && alpha == nullptr) {
        /* everything before this pixel was opaque */
        alpha = std::make_unique<uint8_t[]>(std::size_t(width) * height);
        std::fill_n(alpha.get(), std::size_t(width) * height, 0xff);
      }
      if (alpha != nullptr)
        alpha[std::size_t(dest_y) * width + x] = a;
    }
  }

  return alpha;
}
