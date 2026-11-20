// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * The supply voltage that the application XCSoar runs in (the ESP32
 * host) measures, such as that of its own battery, in volts; NaN if
 * it has none.  May be called from any thread.  Implemented by the
 * host glue.
 */
double
HostSupplyVoltage() noexcept;
