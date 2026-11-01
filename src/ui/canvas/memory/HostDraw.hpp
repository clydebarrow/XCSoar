// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Color.hpp"
#include "ui/dim/Rect.hpp"

/* Drawing with the hardware of the application XCSoar runs in (the
   ESP32 host), implemented by the host glue. */

/**
 * Fills and blends of at least this many pixels are offered to
 * HostFill() and HostBlend().
 * Below that, the CPU is faster: on the ESP32-P4, a PPA fill costs
 * about 0.8 ms whatever its size (it writes back the cache of the
 * whole buffer), the CPU 0.2 ms for 128x128 pixels and 3.8 ms for
 * 256x256.
 */
static constexpr unsigned HOST_FILL_MIN_PIXELS = 40000;

/**
 * Let the application XCSoar runs in (the ESP32 host) fill #rect of
 * a canvas buffer with hardware.  Implemented by the host glue.
 *
 * @param buffer the first row of the canvas
 * @param pitch the distance between rows in bytes
 * @param height the number of rows
 * @return false if the host did not fill it
 */
bool
HostFill(void *buffer, unsigned pitch, unsigned height, const PixelRect &rect,
         Color color) noexcept;

/**
 * Let the host blend a block of #src over #rect of #dest:
 * dest + (src - dest) * alpha / 256, as PortableAlphaPixelOperations.
 *
 * @param white_transparent leave #dest unchanged where #src is white
 * @return false if the host did not blend it
 */
bool
HostBlend(void *dest, unsigned dest_pitch, unsigned dest_height,
          const PixelRect &rect,
          const void *src, unsigned src_pitch, unsigned src_height,
          PixelPoint src_position, uint8_t alpha,
          bool white_transparent) noexcept;
