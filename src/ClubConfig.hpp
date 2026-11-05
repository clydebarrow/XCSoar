// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <string>
#include <string_view>

struct BrokenDateTime;

/**
 * Flights waiting to be sent to #ClubConfig::flight_upload_url, one
 * JSON object per line, in the data directory.  The host that sends
 * them renames the file first, so lines appended while it sends go
 * into a new file.
 */
static constexpr const char *CLUB_FLIGHTS_PENDING = "flights-pending.jsonl";

/**
 * Settings of a club and of the glider the unit is fitted to, read
 * from "club.json" in the data directory.  They belong to the unit,
 * not to a pilot profile, so they are known before a profile is
 * chosen.
 *
 * {
 *   "organisation": "Warwick Gliding Club",
 *   "logo": "club-logo.png",
 *   "glider": { "type": "PW6", "registration": "WGQ" },
 *   "flight_upload_url": "https://example.org/flights",
 *   "flight_upload_key": "a secret shared with the server"
 * }
 *
 * The host that sends the flights reads "flight_upload_key" itself:
 * each request carries "X-Signature: sha256=" and the HMAC-SHA256 of
 * its body with that key, in hex.
 */
struct ClubConfig {
  std::string organisation;

  /** an image file, relative to the data directory unless absolute */
  std::string logo;

  std::string glider_type, glider_registration;

  std::string flight_upload_url;

  bool IsDefined() const noexcept {
    return !organisation.empty() || !logo.empty() ||
      !glider_type.empty() || !glider_registration.empty();
  }
};

/**
 * Read "club.json"; a missing file leaves the settings empty, and an
 * invalid one is logged.
 */
void
LoadClubConfig() noexcept;

[[gnu::pure]]
const ClubConfig &
GetClubConfig() noexcept;

/**
 * Change the glider in "club.json", keeping its other settings, so
 * that the unit can be moved to another glider.  Creates the file if
 * there is none.  Throws on error.
 */
void
SaveClubGlider(std::string_view type, std::string_view registration);

/**
 * Queue a flight for #ClubConfig::flight_upload_url, if there is one:
 * the takeoff and landing (UTC), the duration in seconds, the pilot,
 * the glider and the club.
 */
void
SaveClubFlight(const BrokenDateTime &takeoff, const BrokenDateTime &landing,
               const char *pilot) noexcept;
