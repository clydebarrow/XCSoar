// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"
#include "time/GPSClock.hpp"

#include <cassert>

struct NMEAInfo;
struct MoreData;
struct DerivedInfo;
struct LoggerSettings;
class Logger;

class LogComputer {
  GeoPoint last_location;

  GPSClock log_clock;

  /** number of points to log at high rate */
  unsigned fast_log_num;

  Logger *logger = nullptr;

  /** when the traffic was last written by LogThreats() */
  GPSClock threat_clock;

  /** LogThreats() has written traffic that has not been cleared */
  bool threat_logged = false;

public:
  void SetLogger(Logger *_logger) noexcept {
    assert(logger == nullptr);
    assert(_logger != nullptr);

    logger = _logger;
  }

  void Reset() noexcept;
  void StartTask(const NMEAInfo &basic) noexcept;
  bool Run(const MoreData &basic, const DerivedInfo &calculated,
           const LoggerSettings &settings_logger) noexcept;

  void SetFastLogging() noexcept {
    fast_log_num = 5;
  }

private:
  /**
   * Write the traffic that has a collision alarm, or is close without
   * one, to the IGC file as L records, once a second, so encounters
   * can be analysed after the flight.  Distances in meters (north,
   * east and height relative to the ownship), speeds in m/s, angles in
   * degrees, turn rates in degrees per second:
   *
   *   LXCSOWN,hhmmss,track,ground speed,climb,turn rate
   *   LXCSTGT,hhmmss,id,alarm level,north,east,height,track,speed,
   *     climb,turn rate[,seconds to close proximity]
   *   LXCSCLR,hhmmss  (no such traffic any more)
   */
  void LogThreats(const MoreData &basic,
                  const DerivedInfo &calculated) noexcept;
};
