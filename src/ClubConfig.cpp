// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ClubConfig.hpp"
#include "LocalPath.hpp"
#include "LogFile.hpp"
#include "io/BufferedOutputStream.hxx"
#include "io/FileOutputStream.hxx"
#include "io/FileReader.hxx"
#include "json/Parse.hxx"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"
#include "time/BrokenDateTime.hpp"

#ifdef ESP_PLATFORM
#include "HostClubGlider.hpp"
#endif

#include <boost/json/serialize.hpp>
#include <boost/json/value.hpp>

#include <chrono>
#include <fmt/format.h>

static ClubConfig club_config;

static void
NotifyHost() noexcept
{
#ifdef ESP_PLATFORM
  HostClubGlider(club_config.glider_type.c_str(),
                 club_config.glider_registration.c_str());
#endif
}

static std::string
GetString(const boost::json::object &o, std::string_view key) noexcept
{
  if (const auto *v = o.if_contains(key))
    if (const auto *s = v->if_string())
      return std::string{*s};
  return {};
}

static void
ReadClubConfig() noexcept
try {
  club_config = {};

  const auto path = LocalPath("club.json");
  /* checking first avoids an exception, which is slow on some
     platforms */
  if (!File::Exists(path))
    return;

  FileReader reader{path};
  const auto value = Json::Parse(reader);
  const auto *o = value.if_object();
  if (o == nullptr) {
    LogFormat("club.json: not a JSON object");
    return;
  }

  club_config.organisation = GetString(*o, "organisation");
  club_config.logo = GetString(*o, "logo");
  club_config.flight_upload_url = GetString(*o, "flight_upload_url");

  if (const auto *g = o->if_contains("glider"))
    if (const auto *glider = g->if_object()) {
      club_config.glider_type = GetString(*glider, "type");
      club_config.glider_registration = GetString(*glider, "registration");
    }
} catch (...) {
  club_config = {};
  LogError(std::current_exception(), "Failed to read club.json");
}

void
LoadClubConfig() noexcept
{
  ReadClubConfig();
  NotifyHost();
}

void
SaveClubGlider(std::string_view type, std::string_view registration)
{
  const auto path = LocalPath("club.json");

  boost::json::object root;
  if (File::Exists(path)) {
    FileReader reader{path};
    auto value = Json::Parse(reader);
    if (auto *o = value.if_object())
      root = std::move(*o);
  }

  boost::json::object glider;
  if (auto *g = root.if_contains("glider"))
    if (auto *o = g->if_object())
      glider = std::move(*o);
  glider["type"] = type;
  glider["registration"] = registration;
  root["glider"] = std::move(glider);

  /* written to a new file that then replaces the old one, so a power
     loss leaves one or the other */
  FileOutputStream file(path);
  BufferedOutputStream writer(file);
  writer.Write(boost::json::serialize(root));
  writer.Write('\n');
  writer.Flush();
  file.Commit();

  club_config.glider_type = type;
  club_config.glider_registration = registration;
  LogFormat("club.json: glider set to %s %s",
            club_config.glider_type.c_str(),
            club_config.glider_registration.c_str());
  NotifyHost();
}

const ClubConfig &
GetClubConfig() noexcept
{
  return club_config;
}

static std::string
FormatUTC(const BrokenDateTime &t) noexcept
{
  return fmt::format("{:04}-{:02}-{:02}T{:02}:{:02}:{:02}Z",
                     t.year, t.month, t.day, t.hour, t.minute, t.second);
}

void
SaveClubFlight(const BrokenDateTime &takeoff, const BrokenDateTime &landing,
               const char *pilot) noexcept
try {
  if (club_config.flight_upload_url.empty())
    return;

  const auto duration = std::chrono::duration_cast<std::chrono::seconds>(landing - takeoff);

  boost::json::object flight;
  flight["organisation"] = club_config.organisation;
  flight["glider_type"] = club_config.glider_type;
  flight["registration"] = club_config.glider_registration;
  flight["pilot"] = pilot;
  flight["takeoff"] = FormatUTC(takeoff);
  flight["landing"] = FormatUTC(landing);
  flight["duration"] = duration.count();

  FileOutputStream file(LocalPath(CLUB_FLIGHTS_PENDING),
                        FileOutputStream::Mode::APPEND_OR_CREATE);
  BufferedOutputStream writer(file);
  writer.Write(boost::json::serialize(flight));
  writer.Write('\n');
  writer.Flush();
  file.Commit();

  LogFormat("Club flight queued: %s to %s",
            FormatUTC(takeoff).c_str(), FormatUTC(landing).c_str());
} catch (...) {
  LogError(std::current_exception(), "Failed to queue the club flight");
}
