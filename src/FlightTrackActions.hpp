// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Path;

/**
 * Show the track of the flight logged in the IGC file #path on the
 * map, zoomed to fit, in pan mode.  Shows a message on failure.
 *
 * @return true if the track is shown
 */
bool
ShowFlightTrack(Path path) noexcept;

/**
 * Remove the track shown by ShowFlightTrack() from the map.
 */
void
HideFlightTrack() noexcept;

[[gnu::pure]]
bool
IsFlightTrackShown() noexcept;
