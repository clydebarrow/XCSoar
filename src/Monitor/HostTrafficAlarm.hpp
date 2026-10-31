// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FLARM/Traffic.hpp"

/**
 * Tell the application XCSoar runs in (the ESP32 host) about a new or
 * escalated traffic alarm, so it can sound it.  Implemented by the
 * host glue.
 */
void
HostTrafficAlarm(FlarmTraffic::AlarmType level) noexcept;
