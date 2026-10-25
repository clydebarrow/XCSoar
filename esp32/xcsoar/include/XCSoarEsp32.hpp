// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

/**
 * Entry points for running XCSoar inside an ESP-IDF application
 * (e.g. an ESPHome component).  The host application owns the
 * display; XCSoar renders into a memory canvas and hands each
 * finished frame over as RGB565.
 */
namespace XCSoarEsp32 {

/**
 * A file embedded in the firmware.
 */
struct RomFile {
  /**
   * Path relative to the mount point, e.g.
   * "fonts/DejaVuSansCondensed.ttf".
   */
  const char *name;

  std::span<const std::byte> data;
};

/**
 * Where MountRomFs() makes the embedded files visible.
 */
constexpr const char *ROM_MOUNT_POINT = "/rom";

/**
 * Make the given files readable through the C library (open(),
 * fopen(), stat()) below #ROM_MOUNT_POINT.  The array and the data
 * it points to must stay valid for the life of the program.  Call
 * once, before Run().
 *
 * @return false on error (already logged)
 */
bool
MountRomFs(std::span<const RomFile> files) noexcept;

struct Config {
  unsigned width, height;

  /**
   * The RGB565 frame (native byte order, no row padding), owned by
   * the caller; width * height pixels.
   */
  uint16_t *frame;

  /**
   * Called on the XCSoar task each time a new frame has been
   * written to #frame.  It must not return until the caller has
   * finished reading #frame.
   */
  void (*frame_ready)(void *ctx) noexcept;
  void *ctx;

  unsigned frame_interval_ms;

  /**
   * Stop after this many frames; 0 means run forever.
   */
  unsigned max_frames;
};

/**
 * Run XCSoar on the calling thread.  Returns when #Config::max_frames
 * have been drawn or on a fatal error (which is logged).
 *
 * @return false on error
 */
bool
Run(const Config &config) noexcept;

} // namespace XCSoarEsp32
