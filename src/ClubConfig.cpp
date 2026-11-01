// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ClubConfig.hpp"
#include "LocalPath.hpp"
#include "LogFile.hpp"
#include "io/FileReader.hxx"
#include "json/Parse.hxx"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"

#include <boost/json/value.hpp>

static ClubConfig club_config;

static std::string
GetString(const boost::json::object &o, std::string_view key) noexcept
{
  if (const auto *v = o.if_contains(key))
    if (const auto *s = v->if_string())
      return std::string{*s};
  return {};
}

void
LoadClubConfig() noexcept
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

const ClubConfig &
GetClubConfig() noexcept
{
  return club_config;
}
