// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WifiBackend.hpp"

/**
 * Wi-Fi controlled by the application XCSoar runs in (the ESP32
 * build, see esp32/xcsoar/include/XCSoarEsp32.hpp).  The host keeps
 * the saved networks and chooses which to join; joining one in the
 * list keeps it until automatic choice is turned on again.
 */
class HostWifiBackend : public WifiBackend {
public:
  /**
   * @return nullptr if the host has no Wi-Fi control
   */
  static std::unique_ptr<HostWifiBackend> Create() noexcept;

  void EnsureConnected() override {}
  void Scan() override;
  void Connect(const char *ssid, const char *passphrase,
               WifiSecurity security) override;
  void Connect(const WifiConnectRequest &request) override;
  std::size_t GetNetworks(WifiNetworkEntry *dest,
                          std::size_t max) override;
  void ForgetNetwork(const char *profile_id) override;
  void Disconnect() override;
  WifiBackendStatus GetBackendStatus() override;

  /**
   * The host chooses the network, SkyEcho networks first.
   */
  bool IsAutomatic() noexcept;
  void SetAutomatic() noexcept;

  bool IsEnabled() noexcept;
  /**
   * Turn Wi-Fi on (choosing the network automatically) or off.
   */
  void SetEnabled(bool enabled) noexcept;
};
