// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/BrokenDateTime.hpp"
#include "time/Stamp.hpp"
#include "system/Path.hpp"

#include <chrono>

struct MoreData;
struct DerivedInfo;

/**
 * This class logs start and landing into a file, to be used as a
 * flying log book.
 *
 * Before first using it, this object must be initialised explicitly
 * by calling Reset().
 *
 * Depends on #FlyingComputer.
 */
class FlightLogger {
  AllocatedPath path;

  TimeStamp last_time;
  bool seen_on_ground, seen_flying;

  /**
   * Set to the most recently observed start time.  It gets cleared
   * after a landing has been logged.
   */
  BrokenDateTime start_time;

  BrokenDateTime landing_time;

  /**
   * The confirmed start of the current flight, kept until its landing
   * is logged.
   */
  BrokenDateTime takeoff_time;

public:
  FlightLogger() {
    Reset();
  }

  virtual ~FlightLogger() noexcept = default;

  /**
   * Call this before Tick().
   */
  void SetPath(Path _path) {
    path = _path;
  }

  void Reset();

  /**
   * Call this periodically.
   */
  void Tick(const MoreData &basic, const DerivedInfo &calculated);

protected:
  /**
   * Called after the landing of a flight whose start was logged.
   */
  virtual void OnFlight([[maybe_unused]] const BrokenDateTime &takeoff,
                        [[maybe_unused]] const BrokenDateTime &landing) noexcept {}

private:
  void LogEvent(const BrokenDateTime &date_time, const char *type);

  void TickInternal(const MoreData &basic, const DerivedInfo &calculated);
};
