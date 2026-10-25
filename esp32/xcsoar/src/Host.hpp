// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

namespace XCSoarEsp32 {

struct Config;

/**
 * Send the frames of the XCSoar #UI::Display created after this call
 * to #Config::frame_ready.  The config must stay valid while XCSoar
 * runs.
 */
void
ConnectDisplay(const Config &config) noexcept;

} // namespace XCSoarEsp32
