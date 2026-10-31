// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Threat.hpp"

#include <algorithm>
#include <cstddef>

/* how far ahead to look for the closest point of approach and close
   proximity */
static constexpr float LOOKAHEAD_S = 120;

/* turning paths are followed in steps of this long, with straight
   lines between them */
static constexpr float PATH_STEP_S = 2;
static constexpr std::size_t PATH_STEPS = std::size_t(LOOKAHEAD_S / PATH_STEP_S);

/* 1000 ft of height separation counts the same as 1 NM horizontally */
static constexpr float VERTICAL_WEIGHT = 1852.0f / 304.8f;

/* a target moving directly away is scored as this many times further
   away */
static constexpr float OPENING_FACTOR = 2;

/* below this relative ground speed squared, (0.5 m/s)^2, a target
   counts as not moving horizontally */
static constexpr float MIN_SPEED_SQUARED = 0.25f;

namespace {

/**
 * One aircraft flying at a constant speed and turn rate, moved one
 * step at a time.
 */
class Turn {
  /** velocity */
  float north, east;

  float chord_cos, chord_sin, cos_turn, sin_turn;

public:
  Turn(float _north, float _east, float rate, float step) noexcept
    :north(_north), east(_east)
  {
    const float half = rate * step / 2;
    /* the chord of an arc is shorter than the arc by sin(half) / half,
       and points half way round it */
    const float chord = half == 0 ? step : step * std::sin(half) / half;
    chord_cos = chord * std::cos(half);
    chord_sin = chord * std::sin(half);
    cos_turn = std::cos(2 * half);
    sin_turn = std::sin(2 * half);
  }

  /**
   * Move one step, adding the distance moved to #n and #e.
   */
  void Step(float &n, float &e) noexcept {
    n += chord_cos * north - chord_sin * east;
    e += chord_cos * east + chord_sin * north;
    const float next_north = cos_turn * north - sin_turn * east;
    east = cos_turn * east + sin_turn * north;
    north = next_north;
  }
};

/**
 * The target's horizontal position relative to the ownship over the
 * look-ahead, as points joined by straight lines.  When neither
 * aircraft turns the relative path is straight, so a single line is
 * exact.
 */
struct RelativePath {
  float north[PATH_STEPS + 1];
  float east[PATH_STEPS + 1];
  std::size_t steps;
  float step_time;

  explicit RelativePath(const ThreatGeometry &g) noexcept {
    north[0] = g.north;
    east[0] = g.east;
    if (g.turn_rate == 0 && g.own_turn_rate == 0) {
      steps = 1;
      step_time = LOOKAHEAD_S;
      north[1] = g.north + g.velocity_north * LOOKAHEAD_S;
      east[1] = g.east + g.velocity_east * LOOKAHEAD_S;
      return;
    }

    steps = PATH_STEPS;
    step_time = PATH_STEP_S;
    Turn own(g.own_velocity_north, g.own_velocity_east, g.own_turn_rate,
             PATH_STEP_S);
    Turn target(g.velocity_north + g.own_velocity_north,
                g.velocity_east + g.own_velocity_east, g.turn_rate,
                PATH_STEP_S);
    for (std::size_t i = 0; i != PATH_STEPS; i++) {
      float own_north = 0, own_east = 0;
      north[i + 1] = north[i];
      east[i + 1] = east[i];
      own.Step(own_north, own_east);
      target.Step(north[i + 1], east[i + 1]);
      north[i + 1] -= own_north;
      east[i + 1] -= own_east;
    }
  }
};

} // anonymous namespace

/**
 * The time of the closest approach within the look-ahead: when the
 * target is nearest horizontally, or when it passes the ownship's
 * height if it has no horizontal movement.  Sets #distance to the
 * horizontal separation then, and #range_rate, which is positive when
 * the separation, with height stretched by the vertical weight, is
 * growing.
 */
static float
ApproachTime(const ThreatGeometry &g, const RelativePath &path,
             float &distance, float &range_rate) noexcept
{
  range_rate = g.east * g.velocity_east + g.north * g.velocity_north +
    g.altitude * g.vertical_speed * VERTICAL_WEIGHT * VERTICAL_WEIGHT;

  if (g.velocity_east * g.velocity_east +
      g.velocity_north * g.velocity_north < MIN_SPEED_SQUARED) {
    distance = std::sqrt(g.east * g.east + g.north * g.north);
    if (g.vertical_speed == 0)
      return 0;
    return std::clamp(-g.altitude / g.vertical_speed, 0.0f, LOOKAHEAD_S);
  }

  float best_time = 0;
  float best = g.east * g.east + g.north * g.north;
  for (std::size_t i = 0; i != path.steps; i++) {
    const float north = path.north[i];
    const float east = path.east[i];
    const float d_north = path.north[i + 1] - north;
    const float d_east = path.east[i + 1] - east;
    const float length_squared = d_north * d_north + d_east * d_east;
    if (length_squared <= 0)
      continue;

    const float fraction =
      std::clamp(-(north * d_north + east * d_east) / length_squared,
                 0.0f, 1.0f);
    const float n = north + d_north * fraction;
    const float e = east + d_east * fraction;
    const float separation = n * n + e * e;
    if (separation < best) {
      best = separation;
      best_time = (float(i) + fraction) * path.step_time;
    }
  }

  distance = std::sqrt(best);
  return best_time;
}

static float
Score(const ThreatGeometry &g, float time, float distance,
      float range_rate) noexcept
{
  const float z = (g.altitude + g.vertical_speed * time) * VERTICAL_WEIGHT;
  const float separation = std::sqrt(distance * distance + z * z);
  if (time > 0 || range_rate <= 0)
    return separation;

  /* scale the penalty by how directly the target is moving away, so
     the score does not jump when a passing target stops closing */
  const float pz = g.altitude * VERTICAL_WEIGHT;
  const float vz = g.vertical_speed * VERTICAL_WEIGHT;
  const float range = std::sqrt(g.east * g.east + g.north * g.north + pz * pz);
  const float speed = std::sqrt(g.velocity_east * g.velocity_east +
                                g.velocity_north * g.velocity_north +
                                vz * vz);
  return separation *
    (1 + (OPENING_FACTOR - 1) * range_rate / (range * speed));
}

float
ThreatScore(const ThreatGeometry &geometry) noexcept
{
  const RelativePath path(geometry);
  float distance, range_rate;
  const float time = ApproachTime(geometry, path, distance, range_rate);
  return Score(geometry, time, distance, range_rate);
}

/**
 * The time the target is first inside close proximity, or negative
 * if not within the look-ahead.
 */
static float
ProximityTime(const ThreatGeometry &g, const RelativePath &path,
              const ThreatSettings &settings) noexcept
{
  /* vertically: |z + vz t| < H */
  const float alarm_altitude = g.gliders
    ? settings.glider_alarm_altitude
    : settings.alarm_altitude;
  float v_start, v_end;
  if (g.vertical_speed == 0) {
    if (std::fabs(g.altitude) >= alarm_altitude)
      return -1;
    v_start = 0;
    v_end = INFINITY;
  } else {
    const float t1 = (-alarm_altitude - g.altitude) / g.vertical_speed;
    const float t2 = (alarm_altitude - g.altitude) / g.vertical_speed;
    v_start = std::min(t1, t2);
    v_end = std::max(t1, t2);
  }

  /* horizontally, along each line of the path: |p + d u| < R between
     the roots of a u^2 + b u + c = 0 */
  const float radius_squared = settings.alarm_distance * settings.alarm_distance;
  for (std::size_t i = 0; i != path.steps; i++) {
    const float north = path.north[i];
    const float east = path.east[i];
    const float d_north = path.north[i + 1] - north;
    const float d_east = path.east[i + 1] - east;
    const float a = d_north * d_north + d_east * d_east;
    const float b = 2 * (north * d_north + east * d_east);
    const float c = north * north + east * east - radius_squared;
    float h_start, h_end;
    if (a <= 0) {
      if (c > 0)
        continue;
      h_start = 0;
      h_end = 1;
    } else {
      const float discriminant = b * b - 4 * a * c;
      if (discriminant < 0)
        continue;
      const float root = std::sqrt(discriminant);
      h_start = std::max((-b - root) / (2 * a), 0.0f);
      h_end = std::min((-b + root) / (2 * a), 1.0f);
    }

    const float offset = float(i);
    const float start = std::max((offset + h_start) * path.step_time, v_start);
    const float end = std::min((offset + h_end) * path.step_time, v_end);
    if (start <= end && h_start <= h_end)
      return std::max(start, 0.0f);
  }

  return -1;
}

ThreatAssessment
AssessThreat(const ThreatGeometry &geometry,
             const ThreatSettings &settings) noexcept
{
  ThreatAssessment assessment{};
  const RelativePath path(geometry);
  float distance, range_rate;
  const float time = ApproachTime(geometry, path, distance, range_rate);
  assessment.score = Score(geometry, time, distance, range_rate);
  assessment.approach_time = time;
  assessment.approach_distance = distance;
  assessment.approach_altitude =
    geometry.altitude + geometry.vertical_speed * time;
  assessment.proximity_time = ProximityTime(geometry, path, settings);

  /* a target already in close proximity but moving away has passed,
     so it no longer raises the alarm */
  assessment.alarm = assessment.proximity_time >= 0 &&
    assessment.proximity_time <= settings.alarm_time &&
    !(assessment.proximity_time == 0 && range_rate > 0);

  const float range = std::sqrt(geometry.east * geometry.east +
                                geometry.north * geometry.north);
  assessment.ignored = !assessment.alarm &&
    (range > settings.max_distance ||
     std::fabs(geometry.altitude) > settings.max_altitude_difference);
  return assessment;
}
