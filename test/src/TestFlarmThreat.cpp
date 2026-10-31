// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FLARM/Threat.hpp"
#include "FLARM/ThreatComputer.hpp"
#include "FLARM/Data.hpp"
#include "NMEA/MoreData.hpp"
#include "TestUtil.hpp"

#include <cmath>

static constexpr float NM = 1852;
static constexpr float FT = 0.3048f;
static constexpr float DEG = 3.14159265f / 180;

static ThreatGeometry
At(float north, float east, float altitude) noexcept
{
  ThreatGeometry g{};
  g.north = north;
  g.east = east;
  g.altitude = altitude;
  return g;
}

/**
 * A target #distance to the north at #altitude, flying straight at
 * the stationary ownship at #speed.
 */
static ThreatGeometry
ClosingNorth(float distance, float altitude, float speed) noexcept
{
  ThreatGeometry g = At(distance, 0, altitude);
  g.velocity_north = -speed;
  return g;
}

/**
 * A target 1 NM north flying east at 50 m/s while the ownship is
 * stationary.
 */
static ThreatGeometry
Crossing(float turn_rate) noexcept
{
  ThreatGeometry g = At(NM, 0, 0);
  g.velocity_east = 50;
  g.turn_rate = turn_rate;
  return g;
}

static bool
Near(float a, float b, float tolerance) noexcept
{
  return std::fabs(a - b) <= tolerance;
}

static void
TestScore() noexcept
{
  ok1(Near(ThreatScore(At(NM, 0, 0)), NM, 0.01f));
  /* 1000 ft counts the same as 1 NM */
  ok1(Near(ThreatScore(At(0, 0, 1000 * FT)), NM, 1));

  /* 3 NM ahead, passing 1 NM to the east, closing at 100 m/s */
  ThreatGeometry closing = At(3 * NM, NM, 0);
  closing.velocity_north = -100;
  ok1(Near(ThreatScore(closing), NM, 1));

  /* moving directly away scores twice its distance */
  ThreatGeometry away = At(NM, 0, 0);
  away.velocity_north = 50;
  ok1(Near(ThreatScore(away), 2 * NM, 0.1f));

  /* closing outranks a nearer opening one */
  ok1(ThreatScore(ClosingNorth(1.5f * NM, 0, 50)) < ThreatScore(away));
}

static void
TestAlarm() noexcept
{
  const ThreatSettings settings{};

  /* close proximity starts 0.5 NM out: at 150 m/s, 30 s from
     0.5 NM + 4500 m */
  const auto early = AssessThreat(ClosingNorth(926 + 4600, 0, 150), settings);
  ok1(Near(early.proximity_time, 4600.0f / 150, 0.1f));
  ok1(!early.alarm);
  const auto late = AssessThreat(ClosingNorth(926 + 4400, 0, 150), settings);
  ok1(Near(late.proximity_time, 4400.0f / 150, 0.1f));
  ok1(late.alarm);

  /* passing wide, or straight at the ownship 1000 ft above */
  ThreatGeometry wide = At(2 * NM, NM, 0);
  wide.velocity_north = -100;
  ok1(AssessThreat(wide, settings).proximity_time < 0);
  ok1(AssessThreat(ClosingNorth(NM, 1000 * FT, 100), settings).proximity_time < 0);

  /* overhead, 1000 ft above, coming down at 10 m/s */
  ThreatGeometry descending = At(0, 0, 1000 * FT);
  descending.vertical_speed = -10;
  const auto down = AssessThreat(descending, settings);
  ok1(Near(down.proximity_time, 500 * FT / 10, 0.01f));
  ok1(down.alarm);

  /* already close: now; already passed: no alarm */
  ok1(AssessThreat(At(300, 0, 100 * FT), settings).alarm);
  ThreatGeometry passed = At(-300, 0, 100 * FT);
  passed.velocity_north = -100;
  ok1(!AssessThreat(passed, settings).alarm);

  /* gliders use a smaller alarm height */
  ThreatGeometry glider = ClosingNorth(926 + 600, 300 * FT, 30);
  ok1(AssessThreat(glider, settings).alarm);
  glider.gliders = true;
  ok1(!AssessThreat(glider, settings).alarm);
}

static void
TestTurns() noexcept
{
  const ThreatSettings settings{};

  /* turning right at 3 degrees per second, its circle passes through
     the ownship */
  const auto straight = AssessThreat(Crossing(0), settings);
  const auto turning = AssessThreat(Crossing(3 * DEG), settings);
  ok1(Near(straight.approach_distance, NM, 1));
  ok1(turning.approach_distance < 100);
  ok1(Near(turning.approach_time, 60, 2));
  ok1(turning.proximity_time > 0);

  /* turning away keeps it clear */
  ok1(AssessThreat(Crossing(-3 * DEG), settings).proximity_time < 0);

  /* the ownship flies west and turns right, towards a stationary
     target to the north */
  ThreatGeometry own_turn = At(NM, 0, 0);
  own_turn.own_velocity_east = -50;
  own_turn.velocity_east = 50;
  own_turn.own_turn_rate = 3 * DEG;
  ok1(AssessThreat(own_turn, settings).approach_distance < 100);
}

static constexpr TimeStamp
At(double seconds) noexcept
{
  return TimeStamp{FloatDuration{seconds}};
}

static void
SetOwnship(MoreData &basic, TimeStamp clock) noexcept
{
  basic = MoreData{};
  basic.clock = clock;
  basic.location = GeoPoint(Angle::Degrees(152), Angle::Degrees(-28));
  basic.location_available.Update(clock);
  basic.track = Angle::Zero();
  basic.track_available.Update(clock);
  basic.ground_speed = 50;
  basic.ground_speed_available.Update(clock);
}

/**
 * A target as the GDL90 driver and FlarmComputer leave it: an
 * absolute position #north of the ownship, flying south at #speed.
 */
static void
AddTarget(FlarmData &flarm, TimeStamp clock, uint32_t id, double north,
          double altitude, double speed,
          FlarmTraffic::SourceType source = FlarmTraffic::SourceType::ADSB) noexcept
{
  FlarmTraffic &t = *flarm.traffic.AllocateTraffic();
  t.Clear();
  t.id = FlarmId::FromValue(id);
  t.valid.Update(clock);
  t.absolute_location = true;
  t.location_available = true;
  t.altitude_available = true;
  t.relative_north = north;
  t.relative_east = 0;
  t.relative_altitude = altitude;
  t.distance = north;
  t.track = Angle::HalfCircle();
  t.speed = speed;
  t.track_received = t.speed_received = true;
  t.source = source;
  t.alarm_level = FlarmTraffic::AlarmType::NONE;
  flarm.traffic.modified.Update(clock);
}

static void
TestComputer() noexcept
{
  ThreatComputer computer;
  MoreData basic;
  FlarmData flarm;

  /* head on, closing at 100 m/s, 926 m + 1000 m away: close
     proximity in 10 s, an important alarm */
  SetOwnship(basic, At(10));
  flarm.Clear();
  AddTarget(flarm, basic.clock, 0x7C0001, 1926, 0, 50);
  /* the same distance, but 2000 ft above: no alarm */
  AddTarget(flarm, basic.clock, 0x7C0002, 1926, 2000 * FT, 50);
  /* online traffic is too old to warn about */
  AddTarget(flarm, basic.clock, 0x7C0003, 1926, 0, 50,
            FlarmTraffic::SourceType::SKYLINES);
  computer.Process(flarm, basic);

  ok1(flarm.traffic.FindTraffic(FlarmId::FromValue(0x7C0001))->alarm_level ==
      FlarmTraffic::AlarmType::IMPORTANT);
  ok1(flarm.traffic.FindTraffic(FlarmId::FromValue(0x7C0002))->alarm_level ==
      FlarmTraffic::AlarmType::NONE);
  ok1(flarm.traffic.FindTraffic(FlarmId::FromValue(0x7C0003))->alarm_level ==
      FlarmTraffic::AlarmType::NONE);
  ok1(flarm.status.available);
  ok1(flarm.status.alarm_level == FlarmTraffic::AlarmType::IMPORTANT);
  ok1(flarm.status.alarm_type == 2);
  ok1(flarm.status.has_bearing && flarm.status.relative_bearing == 0);
  ok1(flarm.status.relative_distance == 1926);
  ok1(flarm.status.target_id == FlarmId::FromValue(0x7C0001));

  /* the merged data is rebuilt before each call: the alarm is applied
     again from the latest assessment */
  flarm.Clear();
  AddTarget(flarm, basic.clock, 0x7C0001, 1926, 0, 50);
  computer.Process(flarm, basic);
  ok1(flarm.status.alarm_level == FlarmTraffic::AlarmType::IMPORTANT);

  /* a FLARM's own higher alarm is kept */
  flarm.Clear();
  AddTarget(flarm, basic.clock, 0x7C0001, 1926, 0, 50);
  flarm.status.available.Update(basic.clock);
  flarm.status.alarm_level = FlarmTraffic::AlarmType::URGENT;
  flarm.status.target_id = FlarmId::FromValue(0x123456);
  computer.Process(flarm, basic);
  ok1(flarm.status.target_id == FlarmId::FromValue(0x123456));

  /* a second later, 5 s from close proximity: urgent */
  SetOwnship(basic, At(11));
  flarm.Clear();
  AddTarget(flarm, basic.clock, 0x7C0001, 1426, 0, 50);
  computer.Process(flarm, basic);
  ok1(flarm.status.alarm_level == FlarmTraffic::AlarmType::URGENT);

  /* once it has gone, nothing is left */
  SetOwnship(basic, At(12));
  flarm.Clear();
  computer.Process(flarm, basic);
  ok1(!flarm.status.available);
}

int
main()
{
  plan_tests(36);

  TestScore();
  TestAlarm();
  TestTurns();
  TestComputer();

  return exit_status();
}
