// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cmath>

/**
 * Where a target is and how it is moving, relative to the ownship.
 * Distances in meters, speeds in meters per second, turn rates in
 * radians per second, positive to the right.
 */
struct ThreatGeometry {
  /** target position north and east of the ownship */
  float north, east;

  /** target altitude above the ownship */
  float altitude;

  /** target velocity relative to the ownship */
  float velocity_north, velocity_east;

  /** target vertical speed relative to the ownship */
  float vertical_speed;

  /** ownship velocity over the ground, needed to follow its turn */
  float own_velocity_north, own_velocity_east;

  float turn_rate, own_turn_rate;

  /** both the ownship and the target are gliders or hang gliders */
  bool gliders;
};

/**
 * Thresholds for the alarm and for ignoring targets.  Distances in
 * meters, times in seconds.
 */
struct ThreatSettings {
  /**
   * Close proximity: within this distance horizontally and this
   * height vertically.
   */
  float alarm_distance = 926;   // 0.5 NM
  float alarm_altitude = 152.4; // 500 ft

  /**
   * The height used instead when both aircraft are gliders, which
   * often fly close together and climb and descend slowly.
   */
  float glider_alarm_altitude = 61; // 200 ft

  /**
   * Raise the alarm this long before a target is predicted to reach
   * close proximity.
   */
  float alarm_time = 30;

  /**
   * Targets further away than these are ignored, unless they raise
   * the alarm.
   */
  float max_distance = INFINITY;
  float max_altitude_difference = INFINITY;
};

/**
 * What is known about one target after assessing it.
 */
struct ThreatAssessment {
  /** outside the distance or altitude filter */
  bool ignored;

  /**
   * predicted to reach close proximity within the alarm time, and
   * not moving away
   */
  bool alarm;

  /** how much attention it needs: lower is more significant */
  float score;

  /** seconds until the closest approach, zero when not closing */
  float approach_time;

  /** horizontal separation at the closest approach */
  float approach_distance;

  /**
   * target height above the ownship at the closest approach,
   * allowing for climb
   */
  float approach_altitude;

  /** seconds until close proximity, negative if not predicted */
  float proximity_time;
};

/**
 * Score how much attention a target needs: lower is more significant.
 *
 * The score is the separation at the closest point of approach within
 * a short look-ahead, with height separation weighted so that 1000 ft
 * counts the same as 1 NM horizontally.  Both aircraft are assumed to
 * keep their speed, vertical speed and turn rate.  The closest
 * approach is when the target is nearest horizontally, with the
 * height difference then predicted from the vertical speeds; a target
 * with no horizontal movement relative to the ownship is closest when
 * it passes the ownship's height.  A target that is moving away is
 * scored as further, up to twice as far when it is heading directly
 * away and will get no closer.
 */
[[gnu::pure]]
float
ThreatScore(const ThreatGeometry &geometry) noexcept;

/**
 * Assess one target: its score, its closest approach and whether it
 * raises the alarm.
 */
[[gnu::pure]]
ThreatAssessment
AssessThreat(const ThreatGeometry &geometry,
             const ThreatSettings &settings) noexcept;
