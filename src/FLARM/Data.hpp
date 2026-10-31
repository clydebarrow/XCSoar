// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FLARM/Error.hpp"
#include "FLARM/Version.hpp"
#include "FLARM/Hardware.hpp"
#include "FLARM/State.hpp"
#include "FLARM/Progress.hpp"
#include "FLARM/Status.hpp"
#include "FLARM/List.hpp"

#include <type_traits>

/**
 * A container for all data received by a FLARM.
 */
struct FlarmData {
  FlarmError error;

  FlarmVersion version;

  FlarmHardware hardware;

  FlarmState state;

  FlarmProgress progress;

  FlarmStatus status;

  TrafficList traffic;

  /**
   * A traffic receiver without $PFLAU (an ADS-B receiver through
   * GDL90) reports that it works.
   */
  Validity receiver;

  constexpr bool IsDetected() const noexcept {
    return status.available || !traffic.IsEmpty();
  }

  /**
   * Is a traffic receiver working, whether or not there is traffic?
   */
  constexpr bool IsReceiving() const noexcept {
    return status.available || receiver;
  }

  constexpr void Clear() noexcept {
    error.Clear();
    version.Clear();
    hardware.Clear();
    state.Clear();
    progress.Clear();
    status.Clear();
    traffic.Clear();
    receiver.Clear();
  }

  constexpr void Complement(const FlarmData &add) noexcept {
    error.Complement(add.error);
    version.Complement(add.version);
    hardware.Complement(add.hardware);
    state.Complement(add.state);
    progress.Complement(add.progress);
    status.Complement(add.status);
    traffic.Complement(add.traffic);
    receiver.Complement(add.receiver);
  }

  constexpr void Expire(TimeStamp clock) noexcept {
    error.Expire(clock);
    version.Expire(clock);
    hardware.Expire(clock);
    state.Expire(clock);
    progress.Expire(clock);
    status.Expire(clock);
    traffic.Expire(clock);
    receiver.Expire(clock, std::chrono::seconds(10));
  }
};

static_assert(std::is_trivial<FlarmData>::value, "type is not trivial");
