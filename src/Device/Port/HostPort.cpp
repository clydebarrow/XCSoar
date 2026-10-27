// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "HostPort.hpp"
#include "Device/Config.hpp"
#include "Profile/DeviceConfig.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Map.hpp"
#include "event/Call.hxx"
#include "util/BindMethod.hxx"
#include "util/StringAPI.hxx"

#include <algorithm>
#include <array>
#include <string>

static constexpr std::size_t MAX_STREAMS = 4;

static std::array<HostPort::Stream, MAX_STREAMS> streams;
static std::size_t n_streams;

/**
 * The open ports, which Feed() looks through.  Guarded by
 * #open_ports_mutex.
 */
static std::array<HostPort *, 8> open_ports;
static std::mutex open_ports_mutex;

HostPort::HostPort(EventLoop &event_loop, const char *_name,
                   PortListener *_listener, DataHandler &_handler) noexcept
  :BufferedPort(_listener, _handler),
   name(_name),
   inject(event_loop, BIND_THIS_METHOD(OnInject))
{
  const std::scoped_lock lock{open_ports_mutex};
  if (auto i = std::find(open_ports.begin(), open_ports.end(), nullptr);
      i != open_ports.end())
    *i = this;
}

HostPort::~HostPort() noexcept
{
  {
    const std::scoped_lock lock{open_ports_mutex};
    std::replace(open_ports.begin(), open_ports.end(),
                 static_cast<HostPort *>(this), static_cast<HostPort *>(nullptr));
  }

  /* OnInject() may be running on the event loop */
  BlockingCall(inject.GetEventLoop(), [this](){
    inject.Cancel();
  });
}

void
HostPort::AddStream(const char *name, const char *driver) noexcept
{
  if (n_streams < streams.size())
    streams[n_streams++] = {name, driver};
}

std::span<const HostPort::Stream>
HostPort::GetStreams() noexcept
{
  return {streams.data(), n_streams};
}

/**
 * Is #name one of the "|" separated names in #list?
 */
[[gnu::pure]]
static bool
ListContains(std::string_view list, std::string_view name) noexcept
{
  while (!list.empty()) {
    const auto end = list.find('|');
    if (list.substr(0, end) == name)
      return true;
    if (end == list.npos)
      break;
    list.remove_prefix(end + 1);
  }

  return false;
}

void
HostPort::ApplyStreams(ProfileMap &map,
                       std::span<DeviceConfig> devices) noexcept
{
  std::string offered = map.Get(ProfileKeys::HostStreams, "");
  bool changed = false;

  for (const auto &stream : GetStreams()) {
    if (ListContains(offered, stream.name))
      continue;

    const auto used = std::any_of(devices.begin(), devices.end(),
                                  [&stream](const DeviceConfig &config){
      return config.port_type == DeviceConfig::PortType::HOST &&
        config.path.equals(stream.name);
    });
    if (!used) {
      const auto free = std::find_if(devices.begin(), devices.end(),
                                     [](const DeviceConfig &config){
        return config.port_type == DeviceConfig::PortType::DISABLED;
      });
      if (free == devices.end())
        /* try again at the next start */
        continue;

      free->Clear();
      free->port_type = DeviceConfig::PortType::HOST;
      free->path = stream.name;
      free->driver_name = stream.driver;
      Profile::SetDeviceConfig(map, std::distance(devices.begin(), free),
                               *free);
    }

    if (!offered.empty())
      offered += '|';
    offered += stream.name;
    changed = true;
  }

  if (changed)
    map.Set(ProfileKeys::HostStreams, offered.c_str());
}

void
HostPort::Feed(const char *name, std::span<const std::byte> data) noexcept
{
  const std::scoped_lock lock{open_ports_mutex};
  for (HostPort *port : open_ports)
    if (port != nullptr && port->name.equals(name))
      port->Push(data);
}

void
HostPort::Push(std::span<const std::byte> data) noexcept
{
  {
    const std::scoped_lock lock{input_mutex};
    if (input.GetCapacity() - input.GetAvailable() < data.size())
      /* the event loop is behind; a stream protocol recovers from a
         lost chunk */
      return;

    input.MoveFrom(data);
  }

  inject.Schedule();
}

void
HostPort::OnInject() noexcept
{
  std::array<std::byte, 1024> chunk;

  while (true) {
    std::size_t n;
    {
      const std::scoped_lock lock{input_mutex};
      auto r = input.Read();
      n = std::min(r.size(), chunk.size());
      if (n == 0)
        return;

      std::copy_n(r.begin(), n, chunk.begin());
      input.Consume(n);
    }

    DataReceived({chunk.data(), n});
  }
}
