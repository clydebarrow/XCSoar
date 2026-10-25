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
 * finished frame over, for the host to convert and show.
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
   * Called on the XCSoar task each time a frame is finished.  The
   * frame is width * height pixels, 32 bits each, stored as the bytes
   * blue, green, red, alpha (ARGB8888 read as a little endian
   * word), with each row starting #pitch bytes after the previous
   * one.  The pixels are only valid until this function returns.
   */
  void (*frame_ready)(const void *pixels, unsigned pitch,
                      void *ctx) noexcept;
  void *ctx;

  /**
   * The directory XCSoar keeps its data in (profiles, maps, logs),
   * e.g. "/sdcard/XCSoar".
   */
  const char *data_path;

  /**
   * The pixel density of the panel, which XCSoar sizes its fonts and
   * symbols by; 0 lets XCSoar assume 96.
   */
  unsigned dpi;

  /**
   * RunDemo() only: how often the test scene moves on.
   */
  unsigned frame_interval_ms;

  /**
   * RunDemo() only: stop after this many animation steps; 0 means
   * run forever.
   */
  unsigned max_frames;
};

/**
 * Run XCSoar on the calling thread, as its main() would.  Returns
 * when XCSoar quits.  The calling thread must be a pthread: XCSoar
 * identifies its threads with pthread_self(), which aborts in other
 * FreeRTOS tasks.
 *
 * @return false on error
 */
bool
Run(const Config &config) noexcept;

/**
 * Run a test scene instead of XCSoar: XCSoar's canvas, fonts, event
 * loop and windows, with touch input.  Returns when
 * #Config::max_frames animation steps have been drawn or on a fatal
 * error (which is logged).
 *
 * @return false on error
 */
bool
RunDemo(const Config &config) noexcept;

enum class PointerAction : uint8_t {
  DOWN,
  MOVE,
  UP,
};

/**
 * Pass a touch (or mouse) event to XCSoar, at a position in XCSoar's
 * screen coordinates.  May be called from any task while Run() is
 * running; it is ignored before XCSoar's event queue exists.
 */
void
InjectPointer(PointerAction action, int x, int y) noexcept;

} // namespace XCSoarEsp32
