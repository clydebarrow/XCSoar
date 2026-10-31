// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>
#include <span>

class DetectDeviceListener;

/**
 * Bluetooth LE serial devices through the application XCSoar runs in
 * (the ESP32 host).  Implemented by the host glue; every function does
 * nothing (or returns false) when the host has no Bluetooth.
 */
namespace HostBluetooth {

[[gnu::pure]]
bool
IsAvailable() noexcept;

/**
 * Scan for devices while at least one listener is registered.  The
 * listener is called from any thread.
 */
void
AddDetectListener(DetectDeviceListener &listener) noexcept;

void
RemoveDetectListener(DetectDeviceListener &listener) noexcept;

/**
 * Connect to the device and keep it connected until Close().  Its
 * data arrives through HostPort::Feed() with the address as the name,
 * and its connection state through HostPort::SetConnected().
 */
void
Open(const char *address) noexcept;

void
Close(const char *address) noexcept;

/**
 * @return the number of bytes taken; 0 when the host's buffer is full
 */
std::size_t
Write(const char *address, std::span<const std::byte> data) noexcept;

} // namespace HostBluetooth
