// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TrafficSettings.hpp"

void
TrafficSettings::SetDefaults() noexcept
{
  enable_gauge = true;
  auto_close_dialog = false;
  auto_zoom = true;
  north_up = false;
  radar_zoom = 4;
  gauge_location = GaugeLocation::BOTTOM_RIGHT_AVOID_IB;
  no_position_target_distance_ring = true;
  spoken_alerts = false;
#ifdef ESP_PLATFORM
  /* a dedicated glide computer, often without a FLARM display */
  alert_popup = AlertPopup::IMPORTANT;
#else
  alert_popup = AlertPopup::OFF;
#endif
}
