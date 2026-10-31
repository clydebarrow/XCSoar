// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FLARM/Id.hpp"
#include "FLARM/Status.hpp"
#include "FLARM/Traffic.hpp"
#include "FLARM/Threat.hpp"
#include "Math/Angle.hpp"
#include "time/Stamp.hpp"
#include "util/StaticArray.hxx"

struct FlarmData;
struct MoreData;

/**
 * Raises collision alarms for traffic that came without one: targets
 * received as positions (ADS-B, e.g. through GDL90) rather than from
 * a FLARM, which assesses its own targets.  Online traffic is left
 * alone, since its positions are too old.
 *
 * Each target is assessed with AssessThreat() and given the FLARM
 * alarm level for the time to close proximity, and the most urgent
 * one becomes the alarm in FlarmStatus (as a FLARM's $PFLAU), unless
 * a FLARM reports a higher one.  This gives the same warnings as a
 * FLARM: the traffic gauge and map colors, and the spoken alert.
 */
class ThreatComputer {
  /** FLARM alarm level by time to close proximity, in seconds */
  static constexpr float URGENT_TIME = 8;
  static constexpr float IMPORTANT_TIME = 12;
  static constexpr float LOW_TIME = 18;

  struct TargetAlarm {
    FlarmId id;
    FlarmTraffic::AlarmType level;
  };

  ThreatSettings settings;

  /** when the targets were last assessed */
  TimeStamp last_time = TimeStamp::Undefined();

  /** the ownship track at #last_time, for its turn rate */
  Angle last_track;
  bool last_track_available = false;

  /** the ownship turn rate, in degrees per second, smoothed */
  float own_turn_rate = 0;

  /** the alarms found at the latest assessment */
  StaticArray<TargetAlarm, 16> alarms;

  /** the most urgent of #alarms, as a FLARM would report it */
  FlarmStatus status;
  bool has_status = false;

public:
  ThreatComputer() noexcept {
    settings.alarm_time = LOW_TIME;
  }

  /**
   * Assess the traffic at most once a second, and apply the latest
   * alarms to #flarm.  Call after FlarmComputer::Process(), which
   * fills in the targets' relative positions.
   */
  void Process(FlarmData &flarm, const MoreData &basic) noexcept;

private:
  void Assess(const FlarmData &flarm, const MoreData &basic) noexcept;
  void UpdateOwnTurnRate(const MoreData &basic) noexcept;
  void Apply(FlarmData &flarm, TimeStamp clock) const noexcept;
};
