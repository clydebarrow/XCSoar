// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
 
#pragma once

#include "Geo/SearchPointVector.hpp"

struct GeoPoint;
struct FlatGeoPoint;
class SearchPoint;

/**
 * Note that this expects the vector to be closed, that is, starting point
 * and ending point are the same
 */
[[gnu::pure]]
bool
PolygonInterior(const GeoPoint &p,
                SearchPointVector::const_iterator begin,
                SearchPointVector::const_iterator end);

/**
 * The same as the GeoPoint version, given also the point's flat
 * location in the projection of the vertices' flat locations: edges
 * whose flat latitudes are both above or both below it cannot cross
 * the point's latitude, and are skipped without the slow GeoPoint
 * comparisons.  The result is the same.
 */
[[gnu::pure]]
bool
PolygonInterior(const GeoPoint &p, const FlatGeoPoint &flat_p,
                SearchPointVector::const_iterator begin,
                SearchPointVector::const_iterator end);

[[gnu::pure]]
bool
PolygonInterior(const FlatGeoPoint &p,
                SearchPointVector::const_iterator begin,
                SearchPointVector::const_iterator end);
