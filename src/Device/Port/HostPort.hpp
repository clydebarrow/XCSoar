// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "BufferedPort.hpp"
#include "event/InjectEvent.hxx"
#include "util/StaticFifoBuffer.hxx"
#include "util/StaticString.hxx"

#include <mutex>
#include <span>

struct DeviceConfig;
class EventLoop;
class ProfileMap;

/**
 * A port that reads the data the application XCSoar runs in passes
 * to Feed(), for example traffic that an ESPHome component received.
 * The data is delivered on the event loop.  Writes are discarded.
 */
class HostPort final : public BufferedPort {
public:
  /**
   * A stream of data the application provides.
   */
  struct Stream {
    /**
     * The name Feed() and the device configuration use, which is
     * also shown as the port name, e.g. "SkyEcho".
     */
    const char *name;

    /**
     * The driver that reads it by default, e.g. "GDL90".
     */
    const char *driver;
  };

private:
  const StaticString<32> name;

  InjectEvent inject;

  std::mutex input_mutex;
  StaticFifoBuffer<std::byte, 4096> input;

public:
  HostPort(EventLoop &event_loop, const char *_name,
           PortListener *_listener, DataHandler &_handler) noexcept;
  ~HostPort() noexcept override;

  /**
   * Make a stream available.  Call before XCSoar starts.  The strings
   * must stay valid.
   */
  static void AddStream(const char *name, const char *driver) noexcept;

  [[gnu::pure]]
  static std::span<const Stream> GetStreams() noexcept;

  /**
   * Put each stream that no device reads yet into a disabled device
   * slot, with its driver, and save that slot to the profile.  Each
   * stream is offered once: the profile remembers it, so a device the
   * user removed or moved to another port stays that way.
   */
  static void ApplyStreams(ProfileMap &map,
                           std::span<DeviceConfig> devices) noexcept;

  /**
   * Pass data to the open port reading the stream #name.  It is
   * dropped if no device reads the stream or the port's buffer is
   * full.  May be called from any task.
   */
  static void Feed(const char *name, std::span<const std::byte> data) noexcept;

  /* virtual methods from class Port */
  PortState GetState() const noexcept override {
    return PortState::READY;
  }

  bool Drain() override {
    return true;
  }

  void SetBaudrate(unsigned) override {
  }

  unsigned GetBaudrate() const noexcept override {
    return 0;
  }

  std::size_t Write(std::span<const std::byte> src) override {
    return src.size();
  }

private:
  void Push(std::span<const std::byte> data) noexcept;
  void OnInject() noexcept;
};
