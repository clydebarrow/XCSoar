// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct Plane;
struct ComputerSettings;
class GlidePolar;
class ProfileMap;
namespace PolarStore { struct Item; }

namespace PlaneGlue {

void
FromProfile(Plane &plane, const ProfileMap &profile) noexcept;

void
Synchronize(const Plane &plane, ComputerSettings &settings,
            GlidePolar &gp) noexcept;

/**
 * Give the plane a built-in polar, with its masses, wing area,
 * maximum speed and handicap where the polar has them.
 */
void
ApplyPolar(Plane &plane, const PolarStore::Item &item) noexcept;

} // namespace PlaneGlue
