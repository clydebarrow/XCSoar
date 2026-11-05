// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Tell the application XCSoar runs in (the ESP32 host) the type and
 * registration of the glider in "club.json", empty if there is none:
 * when the file is read and when the glider is changed.  The host can
 * then, for example, send the glider's address on its own FLARM radio.
 * Implemented by the host glue.
 */
void
HostClubGlider(const char *type, const char *registration) noexcept;
