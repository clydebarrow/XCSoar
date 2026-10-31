// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Host.hpp"
#include "XCSoarEsp32.hpp"
#include "Android/DetectDeviceListener.hpp"
#include "Device/Port/HostBluetooth.hpp"
#include "Device/Port/HostPort.hpp"

#include <algorithm>
#include <array>
#include <mutex>

static_assert(XCSoarEsp32::BLE_FEATURE_SERIAL ==
              DetectDeviceListener::FEATURE_BLE_SERIAL);

static const XCSoarEsp32::BleHost *ble_host;

/**
 * The open port pickers.  Guarded by #listeners_mutex, which
 * BleDetected() holds while calling them, so a listener is not
 * called after RemoveDetectListener() returns.
 */
static std::array<DetectDeviceListener *, 2> listeners;
static std::mutex listeners_mutex;

namespace XCSoarEsp32 {

void
ConnectBluetooth(const BleHost *host) noexcept
{
  ble_host = host;
}

void
BleDetected(const char *address, const char *name,
            uint64_t features) noexcept
{
  const std::scoped_lock lock{listeners_mutex};
  for (DetectDeviceListener *listener : listeners)
    if (listener != nullptr)
      listener->OnDeviceDetected(DetectDeviceListener::Type::BLUETOOTH_LE,
                                 address, name, features);
}

void
BleConnected(const char *address, bool connected) noexcept
{
  HostPort::SetConnected(address, connected);
}

} // namespace XCSoarEsp32

namespace HostBluetooth {

bool
IsAvailable() noexcept
{
  return ble_host != nullptr;
}

void
AddDetectListener(DetectDeviceListener &listener) noexcept
{
  if (ble_host == nullptr)
    return;

  bool first;
  {
    const std::scoped_lock lock{listeners_mutex};
    first = std::all_of(listeners.begin(), listeners.end(),
                        [](const auto *l){ return l == nullptr; });
    if (auto i = std::find(listeners.begin(), listeners.end(), nullptr);
        i != listeners.end())
      *i = &listener;
  }

  if (first)
    ble_host->start_scan(ble_host->ctx);
}

void
RemoveDetectListener(DetectDeviceListener &listener) noexcept
{
  if (ble_host == nullptr)
    return;

  bool last;
  {
    const std::scoped_lock lock{listeners_mutex};
    std::replace(listeners.begin(), listeners.end(), &listener,
                 static_cast<DetectDeviceListener *>(nullptr));
    last = std::all_of(listeners.begin(), listeners.end(),
                       [](const auto *l){ return l == nullptr; });
  }

  if (last)
    ble_host->stop_scan(ble_host->ctx);
}

void
Open(const char *address) noexcept
{
  if (ble_host != nullptr)
    ble_host->open(address, ble_host->ctx);
}

void
Close(const char *address) noexcept
{
  if (ble_host != nullptr)
    ble_host->close(address, ble_host->ctx);
}

std::size_t
Write(const char *address, std::span<const std::byte> data) noexcept
{
  if (ble_host == nullptr)
    return data.size();

  return ble_host->write(address, data.data(), data.size(), ble_host->ctx);
}

} // namespace HostBluetooth
