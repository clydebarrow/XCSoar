// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <string>

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
 *   "flight_upload_url": "https://example.org/flights"
 * }
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
