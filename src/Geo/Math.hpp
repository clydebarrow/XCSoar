// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/*! @file
 * @brief Library for calculating Earth dimensions
 *
 * This library provides general functions for calculating dimensions
 * on the Earth with GPS coordinates.
 */

#pragma once

#include <span>

struct GeoPoint;
class Angle;

/**
 * Calculates projected distance from P3 along line P1-P2.
 */
[[gnu::pure]]
double
ProjectedDistance(const GeoPoint &loc1, const GeoPoint &loc2,
                  const GeoPoint &loc3) noexcept;

void
DistanceBearing(const GeoPoint &loc1, const GeoPoint &loc2,
                double *distance, Angle *bearing) noexcept;

/**
 * Calculates the distance between two locations
 * @param loc1 Location 1
 * @param loc2 Location 2
 * @return The distance
 */
[[gnu::pure]]
double
Distance(const GeoPoint &loc1, const GeoPoint &loc2) noexcept;

/**
 * Calculates the bearing between two locations
 * @param loc1 Location 1
 * @param loc2 Location 2
 * @return The bearing
 */
[[gnu::pure]]
Angle
Bearing(const GeoPoint &loc1, const GeoPoint &loc2) noexcept;

/**
 * Finds the point along a distance dthis (m) between p1 and p2, which are
 * separated by dtotal.
 *
 * This is a slow function.  Adapted from The Aviation Formulary 1.42.
 */
[[gnu::pure]]
GeoPoint
IntermediatePoint(const GeoPoint &loc1, const GeoPoint &loc2,
                  double dthis) noexcept;

/**
 * Find the nearest great-circle middle point between the two.
 */
[[gnu::pure]]
GeoPoint
Middle(const GeoPoint &a, const GeoPoint &b) noexcept;

/** 
 * Calculate and add distances between point 1 and 2, and point 2 and 3.
 * Warning: this method uses spherical calculations only! That's ok since
 *          it's only used to calculate alternate airports, which is no
 *          distance-critical task.
 * 
 * @param loc1 Location 1
 * @param loc2 Location 2
 * @param loc3 Location 3
 * 
 * @return Distance 12 plus 23 (m)
 */
[[gnu::pure]]
double
DoubleDistance(const GeoPoint &loc1, const GeoPoint &loc2,
               const GeoPoint &loc3) noexcept;

/**
 * Calculates the location (loc_out) you would have, after being at
 * a certain start location (loc) with a certain Bearing and going straight
 * forward for a certain Distance.
 * @param loc Current location
 * @param Bearing Current bearing
 * @param Distance Distance to predict
 * @param loc_out Future location
 */
[[gnu::pure]]
GeoPoint FindLatitudeLongitude(const GeoPoint &loc,
                               Angle bearing, double distance) noexcept;

/**
 * Fill #points with the points on a circle around #center, at the
 * bearings start + step, start + 2 * step, and so on.
 *
 * Every tenth point comes from FindLatitudeLongitude(); the points
 * in between come from a local approximation that is corrected
 * towards those, which is much faster.  The error is below one meter
 * for radii up to 100 km, except near the poles.
 */
void
FindArcPoints(const GeoPoint &center, double radius,
              Angle start, Angle step, std::span<GeoPoint> points) noexcept;
