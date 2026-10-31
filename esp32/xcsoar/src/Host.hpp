// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

namespace XCSoarEsp32 {

struct Config;
struct WifiHost;
struct BleHost;

/**
 * Send the frames of the XCSoar #UI::Display created after this call
 * to #Config::frame_ready.  The config must stay valid while XCSoar
 * runs.
 */
void
ConnectDisplay(const Config &config) noexcept;

/**
 * Let XCSoar's network settings use the host's Wi-Fi control;
 * nullptr if there is none.
 */
void
ConnectWifi(const WifiHost *host) noexcept;

/**
 * Let XCSoar's Bluetooth LE ports use the host's Bluetooth; nullptr if
 * there is none.
 */
void
ConnectBluetooth(const BleHost *host) noexcept;

} // namespace XCSoarEsp32
