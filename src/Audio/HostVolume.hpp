// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Tell the application XCSoar runs in (the ESP32 host) the master
 * volume, 0 to 100 percent, for the sounds it plays.  Implemented by
 * the host glue.
 *
 * @param preview the user is adjusting the volume, and the host
 * should play a short tone at it
 */
void
HostVolume(unsigned percent, bool preview=false) noexcept;
