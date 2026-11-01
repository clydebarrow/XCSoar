// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Color.hpp"
#include "ui/dim/Rect.hpp"

/**
 * Fills of at least this many pixels are offered to HostFill().
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
