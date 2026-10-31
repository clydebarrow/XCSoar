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

/**
 * The size of one pixel of the frames passed to Config::frame_ready:
 * 2 for RGB565 in native (little endian) byte order, or 4 for the
 * bytes blue, green, red, alpha (ARGB8888 read as a little endian
 * word).
 */
[[gnu::const]]
unsigned
BytesPerPixel() noexcept;

/**
 * A rectangle in the frame, in pixels.
 */
struct Rect {
  unsigned x, y, width, height;
};

/**
 * A stream of data the host passes to XCSoar with Feed(), for
 * example GDL90 traffic it received.  XCSoar reads it as a device.
 */
struct Stream {
  /**
   * The name Feed() uses, which is also shown as the device's port,
   * e.g. "SkyEcho".
   */
  const char *name;

  /**
   * The XCSoar driver that reads it, e.g. "GDL90" or "FLARM".  When
   * no device reads the stream, XCSoar sets one up with this driver
   * in a free device slot.
   */
  const char *driver;
};

/**
 * A Wi-Fi network the host knows of: saved, configured or seen in
 * the latest scan.
 */
struct WifiNetwork {
  char ssid[33];

  /**
   * The signal in dBm; 0 when not seen in the latest scan.
   */
  int8_t rssi;

  /**
   * Seen in the latest scan.
   */
  bool visible;

  /**
   * Saved or configured, so it can be joined without a password.
   */
  bool known;

  /**
   * Saved at run time, so it can be forgotten.
   */
  bool saved;

  /**
   * Needs a password.
   */
  bool secured;

  /**
   * SkyEcho (GDL90) data has arrived over it.
   */
  bool skyecho;

  /**
   * The network in use.
   */
  bool connected;
};

enum class WifiState : uint8_t {
  OFF,
  CONNECTING,
  CONNECTED,
};

struct WifiStatus {
  WifiState state;

  /**
   * The host chooses the network (SkyEcho first), rather than the
   * user.
   */
  bool automatic;

  char ssid[33];
  char ip[46];
};

/**
 * Wi-Fi control by the host.  The functions may be called from any
 * thread and return at once: a request is carried out later, and its
 * result shows in what #list and #get_status return.
 */
struct WifiHost {
  /**
   * Copy up to #max networks into #dest and return how many.
   */
  unsigned (*list)(WifiNetwork *dest, unsigned max, void *ctx) noexcept;

  void (*get_status)(WifiStatus &status, void *ctx) noexcept;

  /**
   * Scan for networks, without leaving the network in use.
   */
  void (*scan)(void *ctx) noexcept;

  /**
   * Join #ssid and stay on it until #automatic, #disconnect or a
   * restart.  A password (empty for an open network) saves the
   * network first; nullptr joins a known network.
   */
  void (*connect)(const char *ssid, const char *password,
                  void *ctx) noexcept;

  /**
   * Remove a saved network.
   */
  void (*forget)(const char *ssid, void *ctx) noexcept;

  /**
   * Turn Wi-Fi off until #connect, #automatic or a restart.
   */
  void (*disconnect)(void *ctx) noexcept;

  /**
   * Let the host choose the network again, turning Wi-Fi on.
   */
  void (*automatic)(void *ctx) noexcept;

  void *ctx;
};

/**
 * BleDetected() feature bit: the device has a serial service (HM-10,
 * Nordic UART or ISSC).  The value of
 * DetectDeviceListener::FEATURE_BLE_SERIAL.
 */
constexpr uint64_t BLE_FEATURE_SERIAL = 0x1;

/**
 * Bluetooth LE serial devices through the host.  XCSoar keeps which
 * devices to use; the host connects to them.  The functions may be
 * called from any thread and must not block.  An address is written
 * "AA:BB:CC:DD:EE:FF".
 */
struct BleHost {
  /**
   * Report the devices in range with BleDetected() until #stop_scan.
   * Each device needs to be reported once per scan.
   */
  void (*start_scan)(void *ctx) noexcept;

  void (*stop_scan)(void *ctx) noexcept;

  /**
   * Connect to the device, and connect again after it is lost, until
   * #close.  Report the connection with BleConnected() and pass
   * received data to Feed() with the address as the name.
   */
  void (*open)(const char *address, void *ctx) noexcept;

  void (*close)(const char *address, void *ctx) noexcept;

  /**
   * Send data to the device.  Returns the number of bytes taken; 0
   * when the host's buffer is full.  Data for a device that is not
   * connected is taken and dropped.
   */
  std::size_t (*write)(const char *address, const std::byte *data,
                       std::size_t size, void *ctx) noexcept;

  void *ctx;
};

struct Config {
  unsigned width, height;

  /**
   * Called on the XCSoar task each time a frame is finished.  The
   * frame is width * height pixels in the format BytesPerPixel()
   * describes, with each row starting #pitch bytes after the previous
   * one.  The pixels are only valid until this function returns.
   * Only the #dirty_count areas in #dirty have changed since the
   * previous frame.
   */
  void (*frame_ready)(const void *pixels, unsigned pitch,
                      const Rect *dirty, unsigned dirty_count,
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
   * The streams the host will pass to Feed().  The array and the
   * strings must stay valid while XCSoar runs.
   */
  std::span<const Stream> streams;

  /**
   * Wi-Fi control for XCSoar's network settings; nullptr if the host
   * has none.  Must stay valid while XCSoar runs.
   */
  const WifiHost *wifi;

  /**
   * Called with the FLARM alarm level (1 low, 2 important, 3 urgent)
   * on a new traffic alarm and each time it escalates, when XCSoar
   * would play its alarm, so the host can sound it.  Called on an
   * XCSoar thread with #ctx; must not block.  nullptr if the host
   * does not want it.
   */
  void (*traffic_alarm)(unsigned level, void *ctx) noexcept;

  /**
   * Bluetooth LE devices; nullptr if the host has none.  Must stay
   * valid while XCSoar runs.
   */
  const BleHost *ble;

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

/**
 * Pass data to the device reading the stream #name (see
 * Config::streams).  May be called from any task; the data is copied.
 * It is dropped while no device reads the stream, or when XCSoar
 * falls behind.
 */
void
Feed(const char *name, std::span<const std::byte> data) noexcept;

/**
 * Report a device found by BleHost::start_scan.  #name may be nullptr
 * when the device sent none; #features is a set of BLE_FEATURE_* bits.
 * May be called from any task.
 */
void
BleDetected(const char *address, const char *name,
            uint64_t features) noexcept;

/**
 * Report that a device passed to BleHost::open is now connected (and
 * its serial service ready), or lost.  May be called from any task.
 */
void
BleConnected(const char *address, bool connected) noexcept;

} // namespace XCSoarEsp32
