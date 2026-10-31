// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Host.hpp"
#include "XCSoarEsp32.hpp"
#include "net/wifi/HostWifiBackend.hpp"

#include <algorithm>

static const XCSoarEsp32::WifiHost *wifi_host;

namespace XCSoarEsp32 {

void
ConnectWifi(const WifiHost *host) noexcept
{
  wifi_host = host;
}

} // namespace XCSoarEsp32

static XCSoarEsp32::WifiStatus
GetStatus() noexcept
{
  XCSoarEsp32::WifiStatus status{};
  wifi_host->get_status(status, wifi_host->ctx);
  return status;
}

std::unique_ptr<HostWifiBackend>
HostWifiBackend::Create() noexcept
{
  if (wifi_host == nullptr)
    return nullptr;

  return std::make_unique<HostWifiBackend>();
}

void
HostWifiBackend::Scan()
{
  wifi_host->scan(wifi_host->ctx);
}

void
HostWifiBackend::Connect(const char *ssid, const char *passphrase,
                         [[maybe_unused]] WifiSecurity security)
{
  wifi_host->connect(ssid, passphrase, wifi_host->ctx);
}

void
HostWifiBackend::Connect(const WifiConnectRequest &request)
{
  /* a known network has its name as profile id, and its own
     password */
  if (!request.profile_id.empty())
    wifi_host->connect(request.profile_id.c_str(), nullptr,
                       wifi_host->ctx);
  else
    wifi_host->connect(request.ssid.c_str(), request.secret.c_str(),
                       wifi_host->ctx);
}

std::size_t
HostWifiBackend::GetNetworks(WifiNetworkEntry *dest, std::size_t max)
{
  XCSoarEsp32::WifiNetwork networks[40];
  const unsigned n = wifi_host->list(networks,
                                     std::min<std::size_t>(max, std::size(networks)),
                                     wifi_host->ctx);
  const auto status = GetStatus();

  for (unsigned i = 0; i < n; ++i) {
    const auto &network = networks[i];
    auto &entry = dest[i];
    entry.Clear();

    if (network.skyecho)
      entry.ssid.Format("%s (SkyEcho)", network.ssid);
    else
      entry.ssid = network.ssid;

    if (network.known)
      entry.profile_id = network.ssid;

    entry.signal_level = network.rssi;
    entry.signal_unit = WifiSignalUnit::Dbm;
    entry.security = network.secured ? WifiSecurity::WPA : WifiSecurity::Open;
    entry.is_visible = network.visible;
    entry.has_stored_credentials = network.known;
    entry.can_forget = network.saved;

    if (network.connected) {
      entry.kind = WifiNetworkKind::ConnectedNetwork;
      entry.can_disconnect = true;
      entry.ip_address = status.ip;
    } else {
      entry.kind = network.known
        ? WifiNetworkKind::SavedProfile
        : WifiNetworkKind::VisibleAccessPoint;
      entry.can_connect = network.known || network.visible;
    }
  }

  return n;
}

void
HostWifiBackend::ForgetNetwork(const char *profile_id)
{
  wifi_host->forget(profile_id, wifi_host->ctx);
}

void
HostWifiBackend::Disconnect()
{
  wifi_host->disconnect(wifi_host->ctx);
}

WifiBackendStatus
HostWifiBackend::GetBackendStatus()
{
  const auto status = GetStatus();

  WifiBackendStatus result;
  result.signal_unit = WifiSignalUnit::Dbm;
  switch (status.state) {
  case XCSoarEsp32::WifiState::OFF:
    result.state = WifiConnectionState::Disconnected;
    break;

  case XCSoarEsp32::WifiState::CONNECTING:
    result.state = WifiConnectionState::Connecting;
    break;

  case XCSoarEsp32::WifiState::CONNECTED:
    result.state = WifiConnectionState::Connected;
    result.ssid = status.ssid;
    result.ip_address = status.ip;
    break;
  }

  return result;
}

bool
HostWifiBackend::IsAutomatic() noexcept
{
  return GetStatus().automatic;
}

void
HostWifiBackend::SetAutomatic() noexcept
{
  wifi_host->automatic(wifi_host->ctx);
}

bool
HostWifiBackend::IsEnabled() noexcept
{
  return GetStatus().state != XCSoarEsp32::WifiState::OFF;
}

void
HostWifiBackend::SetEnabled(bool enabled) noexcept
{
  if (enabled)
    wifi_host->automatic(wifi_host->ctx);
  else
    wifi_host->disconnect(wifi_host->ctx);
}
