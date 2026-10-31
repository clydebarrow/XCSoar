// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ThreatComputer.hpp"
#include "FLARM/Data.hpp"
#include "NMEA/MoreData.hpp"
#include "Math/Util.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>

using namespace std::chrono;

static constexpr auto ASSESS_INTERVAL = seconds(1);

/* ownship turn rate estimation, in degrees and seconds */
static constexpr float TURN_SMOOTHING_S = 2;
static constexpr float MAX_TURN_GAP_S = 5;
static constexpr float MIN_TURN_SPEED = 5;
static constexpr float MAX_TURN_RATE = 30;

/* anything slower is taken as flying straight */
static constexpr float MIN_TURN_RATE = 1.5f;

/* PFLAU AlarmType of a collision alarm with an aircraft */
static constexpr uint8_t ALARM_TYPE_AIRCRAFT = 2;

static float
StraightIfSlow(float degrees_per_second) noexcept
{
  return std::fabs(degrees_per_second) < MIN_TURN_RATE
    ? 0.0f
    : float(Angle::Degrees(degrees_per_second).Radians());
}

static bool
IsGlider(FlarmTraffic::AircraftType type) noexcept
{
  return type == FlarmTraffic::AircraftType::GLIDER ||
    type == FlarmTraffic::AircraftType::HANG_GLIDER;
}

/**
 * Is this a target that no FLARM has assessed, and recent enough to
 * assess?
 */
static bool
IsAssessable(const FlarmTraffic &traffic) noexcept
{
  return traffic.valid && traffic.absolute_location &&
    traffic.location_available && traffic.altitude_available &&
    !FlarmTraffic::IsInjectedSource(traffic.source);
}

void
ThreatComputer::UpdateOwnTurnRate(const MoreData &basic) noexcept
{
  const bool track_available = basic.track_available &&
    basic.ground_speed_available && basic.ground_speed >= MIN_TURN_SPEED;

  if (!track_available) {
    own_turn_rate = 0;
  } else if (last_track_available && last_time.IsDefined()) {
    const float dt = FloatDuration{basic.clock - last_time}.count();
    if (dt > MAX_TURN_GAP_S) {
      own_turn_rate = 0;
    } else if (dt > 0) {
      const float change = (basic.track - last_track).AsDelta().Degrees();
      const float rate = std::clamp(change / dt, -MAX_TURN_RATE,
                                    MAX_TURN_RATE);
      own_turn_rate += (rate - own_turn_rate) * dt / (TURN_SMOOTHING_S + dt);
    }
  }

  last_track = basic.track;
  last_track_available = track_available;
}

void
ThreatComputer::Assess(const FlarmData &flarm, const MoreData &basic) noexcept
{
  alarms.clear();
  has_status = false;

  const bool own_moving = basic.track_available &&
    basic.ground_speed_available;
  const float own_speed = own_moving ? float(basic.ground_speed) : 0.0f;
  const Angle own_track = own_moving ? basic.track : Angle::Zero();
  const float own_north = own_speed * float(own_track.cos());
  const float own_east = own_speed * float(own_track.sin());
  const float own_vs = basic.gps_vario_available
    ? float(basic.gps_vario)
    : 0.0f;

  const FlarmTraffic *best = nullptr;
  float best_time = 0;
  auto best_level = FlarmTraffic::AlarmType::NONE;

  for (const auto &traffic : flarm.traffic.list) {
    if (!IsAssessable(traffic))
      continue;

    ThreatGeometry g{};
    g.north = float(traffic.relative_north);
    g.east = float(traffic.relative_east);
    g.altitude = float(double(traffic.relative_altitude));

    /* the speed is zero when not known */
    const Angle track = traffic.track;
    const float target_north = float(double(traffic.speed) * track.cos());
    const float target_east = float(double(traffic.speed) * track.sin());

    g.own_velocity_north = own_north;
    g.own_velocity_east = own_east;
    g.velocity_north = target_north - own_north;
    g.velocity_east = target_east - own_east;
    g.vertical_speed = float(traffic.climb_rate) - own_vs;
    g.turn_rate = StraightIfSlow(float(traffic.turn_rate));
    g.own_turn_rate = StraightIfSlow(own_turn_rate);
    /* XCSoar is mostly flown in gliders */
    g.gliders = IsGlider(traffic.type);

    const ThreatAssessment assessment = AssessThreat(g, settings);
    if (!assessment.alarm)
      continue;

    const auto level = assessment.proximity_time <= URGENT_TIME
      ? FlarmTraffic::AlarmType::URGENT
      : assessment.proximity_time <= IMPORTANT_TIME
      ? FlarmTraffic::AlarmType::IMPORTANT
      : FlarmTraffic::AlarmType::LOW;

    if (!alarms.full())
      alarms.push_back({traffic.id, level});

    if (best == nullptr || assessment.proximity_time < best_time) {
      best = &traffic;
      best_time = assessment.proximity_time;
      best_level = level;
    }
  }

  if (best == nullptr)
    return;

  /* the alarm as a FLARM reports it in $PFLAU */
  status.Clear();
  status.rx = flarm.traffic.list.size();
  status.gps = FlarmStatus::GPSStatus::GPS_3D;
  status.alarm_level = best_level;

  const Angle bearing = Angle::FromXY(best->relative_north,
                                      best->relative_east);
  status.relative_bearing = iround((bearing - own_track).AsDelta().Degrees());
  status.has_bearing = true;
  status.alarm_type = ALARM_TYPE_AIRCRAFT;
  status.relative_vertical = iround(double(best->relative_altitude));
  status.has_vertical = true;
  status.relative_distance = uround(double(best->distance));
  status.target_id = best->id;
  status.has_extended = true;
  has_status = true;
}

void
ThreatComputer::Apply(FlarmData &flarm, TimeStamp clock) const noexcept
{
  for (const auto &alarm : alarms) {
    FlarmTraffic *traffic = flarm.traffic.FindTraffic(alarm.id);
    if (traffic != nullptr && alarm.level > traffic->alarm_level)
      traffic->alarm_level = alarm.level;
  }

  if (!has_status)
    return;

  /* a FLARM's own alarm comes first, unless this one is more urgent */
  if (flarm.status.available &&
      flarm.status.alarm_level != FlarmTraffic::AlarmType::INFO_ALERT &&
      flarm.status.alarm_level >= status.alarm_level)
    return;

  const unsigned short rx = flarm.status.available
    ? flarm.status.rx
    : status.rx;
  flarm.status = status;
  flarm.status.rx = rx;
  flarm.status.available.Update(clock);
}

void
ThreatComputer::Process(FlarmData &flarm, const MoreData &basic) noexcept
{
  if (!basic.location_available) {
    alarms.clear();
    has_status = false;
    return;
  }

  if (!last_time.IsDefined() || basic.clock < last_time ||
      basic.clock - last_time >= ASSESS_INTERVAL) {
    UpdateOwnTurnRate(basic);
    Assess(flarm, basic);
    last_time = basic.clock;
  }

  Apply(flarm, basic.clock);
}
