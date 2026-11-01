// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LogComputer.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "Logger/Settings.hpp"
#include "Logger/Logger.hpp"
#include "LogFile.hpp"

#include <algorithm>
#include <stdio.h>

/* traffic this close is logged without an alarm too, to find alarms
   that were missed, in meters */
static constexpr double THREAT_LOG_DISTANCE = 300;

static bool
IsLoggedThreat(const FlarmTraffic &traffic) noexcept
{
  return traffic.valid &&
    (traffic.HasAlarm() || double(traffic.distance) <= THREAT_LOG_DISTANCE);
}

void
LogComputer::LogThreats(const MoreData &basic,
                        const DerivedInfo &calculated) noexcept
{
  if (logger == nullptr || !basic.time_available)
    return;

  const BrokenTime &time = basic.date_time_utc;
  char line[96];

  const auto &traffic = basic.flarm.traffic.list;
  if (std::none_of(traffic.begin(), traffic.end(), IsLoggedThreat)) {
    if (threat_logged) {
      threat_logged = false;
      snprintf(line, sizeof(line), "CLR,%02u%02u%02u",
               time.hour, time.minute, time.second);
      logger->LogComment(line);
    }
    return;
  }

  if (!threat_clock.CheckAdvance(basic.time, std::chrono::seconds(1)))
    return;

  threat_logged = true;

  snprintf(line, sizeof(line), "OWN,%02u%02u%02u,%.0f,%.1f,%.1f,%.1f",
           time.hour, time.minute, time.second,
           basic.track_available ? basic.track.Degrees() : 0.,
           basic.ground_speed_available ? basic.ground_speed : 0.,
           basic.gps_vario_available ? basic.gps_vario : 0.,
           calculated.turn_rate_smoothed.Degrees());
  logger->LogComment(line);

  const FlarmStatus &status = basic.flarm.status;
  for (const auto &t : traffic) {
    if (!IsLoggedThreat(t))
      continue;

    int n = snprintf(line, sizeof(line),
                     "TGT,%02u%02u%02u,%06X,%u,%.0f,%.0f,%.0f,%.0f,%.1f,%.1f,%.1f",
                     time.hour, time.minute, time.second,
                     unsigned(t.id.Value()), unsigned(t.alarm_level),
                     t.relative_north, t.relative_east,
                     double(t.relative_altitude), Angle(t.track).Degrees(),
                     double(t.speed), t.climb_rate, t.turn_rate);
    if (status.available && status.target_id == t.id &&
        status.has_proximity_time && n > 0 && size_t(n) < sizeof(line))
      snprintf(line + n, sizeof(line) - n, ",%.1f", status.proximity_time);
    logger->LogComment(line);
  }
}

void
LogComputer::Reset() noexcept
{
  last_location = GeoPoint::Invalid();
  fast_log_num = 0;
}

void
LogComputer::StartTask(const NMEAInfo &basic) noexcept
try {
  if (logger != NULL)
    logger->LogStartEvent(basic);
} catch (...) {
  LogError(std::current_exception(), "Logger I/O error");
}

bool
LogComputer::Run(const MoreData &basic, const DerivedInfo &calculated,
                 const LoggerSettings &settings_logger) noexcept
try {
  const bool location_jump = basic.location_available &&
    last_location.IsValid() &&
    basic.location.DistanceS(last_location) > 200;

  last_location = basic.GetLocationOrInvalid();

  if (location_jump)
    // prevent bad fixes from being logged
    return false;

  // log points more often in circling mode
  std::chrono::steady_clock::duration period;
  if (fast_log_num) {
    period = std::chrono::seconds(1);
    fast_log_num--;
  } else
    period = calculated.circling
      ? std::chrono::seconds(settings_logger.time_step_circling)
      : std::chrono::seconds(settings_logger.time_step_cruise);

  if (log_clock.CheckAdvance(basic.time, period) && logger != nullptr)
      logger->LogPoint(basic);

  LogThreats(basic, calculated);

  return true;
} catch (...) {
  LogError(std::current_exception(), "Logger I/O error");
  return false;
}
