// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FLARM/Traffic.hpp"
#include "FLARM/SpokenAlert.hpp"
#include "FLARM/Id.hpp"
#include "time/Stamp.hpp"

#include <chrono>
#include <optional>

class TrafficAlertWidget;

/**
 * Monitors the FLARM collision alarm and generates haptic feedback
 * when the alarm level rises.  FLARM devices beep on their own, but
 * the sound is easily missed in a noisy cockpit.  Optionally, the
 * alarm is spoken, and shown on a full screen alert
 * (#TrafficAlertWidget).
 */
class TrafficMonitor {
  FlarmTraffic::AlarmType last = FlarmTraffic::AlarmType::NONE;

  /** The alert spoken last, while the alarm persists */
  std::optional<SpokenTrafficAlert> spoken;

  TimeStamp spoken_time = TimeStamp::Undefined();

  /** The alert shown, or nullptr; owned by the main window */
  TrafficAlertWidget *alert = nullptr;

  /**
   * When the alarm shown on #alert cleared.  The times here are taken
   * from a steady clock: the GPS clock stops when the data does.
   */
  std::optional<std::chrono::steady_clock::time_point> alert_clear_time;

  /**
   * The alert was dismissed for this target at this level; it comes
   * back for another target or a higher level.
   */
  FlarmId dismissed_id = FlarmId::Undefined();
  FlarmTraffic::AlarmType dismissed_level = FlarmTraffic::AlarmType::NONE;

  /**
   * This target is muted, below #muted_level's next level, until
   * #muted_until: no sound, no speech and no alert screen.
   */
  FlarmId muted_id = FlarmId::Undefined();
  FlarmTraffic::AlarmType muted_level = FlarmTraffic::AlarmType::NONE;
  std::optional<std::chrono::steady_clock::time_point> muted_until;

public:
  void Reset() noexcept {
    last = FlarmTraffic::AlarmType::NONE;
    spoken.reset();
  }

  void Check() noexcept;

  /**
   * Show, update or close the alert for the current alarm.  Called by
   * Check(), and by the alert itself while it is shown.
   */
  void CheckAlert() noexcept;

  /** Close the alert; it comes back only for more danger. */
  void DismissAlert() noexcept;

  /** Close the alert and silence its target for a minute. */
  void MuteAlert() noexcept;

  /**
   * Called by #TrafficAlertWidget when it is deleted.  Replaced by
   * another page, it counts as dismissed.
   */
  void OnAlertClosed(const TrafficAlertWidget &widget) noexcept;

private:
  void Speak(bool escalated) noexcept;

  [[gnu::pure]]
  bool IsMuted() const noexcept;

  void CloseAlert() noexcept;
};

/**
 * Play SpokenTrafficAlert::Example(), so the pilot can try the voice
 * and its volume.  Nothing is played while FLARM reports an aircraft
 * alarm: the example would replace what is left of the real alert
 * with a made-up position.
 *
 * @return true if the example plays
 */
bool
PlaySpokenAlertExample() noexcept;
