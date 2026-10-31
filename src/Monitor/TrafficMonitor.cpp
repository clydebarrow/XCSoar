// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TrafficMonitor.hpp"
#include "TrafficAlertWidget.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "PageActions.hpp"
#include "Input/InputEvents.hpp"
#include "Form/Button.hpp"
#include "Audio/Sound.hpp"

#ifdef ESP_PLATFORM
#include "HostTrafficAlarm.hpp"
#endif

#include <chrono>

using namespace std::chrono;

/**
 * While the alarm persists, speak a changed clock position or
 * vertical position not more often than this.
 */
static constexpr auto REPEAT_INTERVAL = seconds(5);

/** How long the Mute button silences a target. */
static constexpr auto MUTE_DURATION = minutes(1);

/** The alert screen stays this long after the alarm cleared. */
static constexpr auto ALERT_CLOSE_DELAY = seconds(3);

/* FLARM alarm type of a collision alarm with an aircraft */
static constexpr uint8_t ALARM_TYPE_AIRCRAFT = 2;

bool
PlaySpokenAlertExample() noexcept
{
  if (SpokenTrafficAlert::FromStatus(CommonInterface::Basic().flarm.status))
    return false;

  const auto clips = SpokenTrafficAlert::Example().GetClips();
  return PlayResources(clips);
}

void
TrafficMonitor::Speak(bool escalated) noexcept
{
  const auto &basic = CommonInterface::Basic();
  const auto alert = SpokenTrafficAlert::FromStatus(basic.flarm.status);
  if (!alert) {
    spoken.reset();
    return;
  }

  /* speak a new or higher alarm at once, and a change while it
     persists only after a while, so the clock position does not
     chatter at a boundary */
  if (!escalated &&
      (*alert == spoken ||
       (spoken_time.IsDefined() &&
        basic.clock - spoken_time < REPEAT_INTERVAL)))
    return;

  spoken = alert;
  spoken_time = basic.clock;

  const auto clips = alert->GetClips();
  PlayResources(clips);
}

bool
TrafficMonitor::IsMuted() const noexcept
{
  const auto &status = CommonInterface::Basic().flarm.status;
  return muted_until && steady_clock::now() < *muted_until &&
    status.target_id == muted_id && status.alarm_level <= muted_level;
}

void
TrafficMonitor::CloseAlert() noexcept
{
  if (alert == nullptr)
    return;

  /* reloading the current page brings back the map, the InfoBoxes
     and full screen as they were; it is deferred, because the alert's
     own button may be calling */
  alert = nullptr;
  PageActions::ScheduleUpdate();
}

void
TrafficMonitor::DismissAlert() noexcept
{
  const auto &status = CommonInterface::Basic().flarm.status;
  dismissed_id = status.target_id;
  dismissed_level = status.alarm_level;
  CloseAlert();
}

void
TrafficMonitor::OnAlertClosed(const TrafficAlertWidget &widget) noexcept
{
  if (alert != &widget)
    return;

  alert = nullptr;
  const auto &status = CommonInterface::Basic().flarm.status;
  dismissed_id = status.target_id;
  dismissed_level = status.alarm_level;
}

void
TrafficMonitor::MuteAlert() noexcept
{
  const auto &basic = CommonInterface::Basic();
  muted_id = basic.flarm.status.target_id;
  /* an urgent alarm always comes through */
  muted_level = std::min(basic.flarm.status.alarm_level,
                         FlarmTraffic::AlarmType::IMPORTANT);
  muted_until = steady_clock::now() + MUTE_DURATION;
  spoken.reset();
  DismissAlert();
}

/**
 * Is this a collision warning?  An "info alert" is not, and it must
 * not escalate the alarm level either.
 */
static constexpr bool
IsWarning(FlarmTraffic::AlarmType level) noexcept
{
  return level != FlarmTraffic::AlarmType::NONE &&
    level != FlarmTraffic::AlarmType::INFO_ALERT;
}

void
TrafficMonitor::CheckAlert() noexcept
{
  const auto &basic = CommonInterface::Basic();
  const auto &status = basic.flarm.status;
  const bool is_warning = status.available && IsWarning(status.alarm_level);

  if (!is_warning || status.alarm_type != ALARM_TYPE_AIRCRAFT) {
    dismissed_id = FlarmId::Undefined();
    dismissed_level = FlarmTraffic::AlarmType::NONE;

    if (alert != nullptr) {
      const auto now = steady_clock::now();
      if (!alert_clear_time) {
        alert_clear_time = now;
        /* show that the alarm has cleared */
        alert->Update();
      } else if (now - *alert_clear_time >= ALERT_CLOSE_DELAY)
        CloseAlert();
    }
    return;
  }

  alert_clear_time.reset();

  if (alert != nullptr) {
    alert->Update();
    return;
  }

  using AlertPopup = TrafficSettings::AlertPopup;
  const auto popup = CommonInterface::GetUISettings().traffic.alert_popup;
  if (popup == AlertPopup::OFF)
    return;

  /* while circling, other gliders in the thermal raise alarms all the
     time */
  const auto min_level = popup == AlertPopup::URGENT ||
    CommonInterface::Calculated().circling
    ? FlarmTraffic::AlarmType::URGENT
    : FlarmTraffic::AlarmType::IMPORTANT;
  if (status.alarm_level < min_level || IsMuted())
    return;

  if (status.target_id == dismissed_id &&
      status.alarm_level <= dismissed_level)
    return;

  auto &main_window = *CommonInterface::main_window;
  main_window.SetFullScreen(true);
  alert = new TrafficAlertWidget(*this);
  main_window.SetWidget(alert);
  /* hides the traffic gauge, which would cover the alert */
  InputEvents::SetFlavour("TrafficAlert");
}

void
TrafficMonitor::Check() noexcept
{
  const auto &status = CommonInterface::Basic().flarm.status;

  const auto alarm_level = status.available
    ? status.alarm_level
    : FlarmTraffic::AlarmType::NONE;

  const bool is_warning = IsWarning(alarm_level);
  const bool was_warning = IsWarning(last);

  /* warn on a new alarm and on each escalation, but not while the
     same alarm persists */
  const bool escalated = is_warning &&
    (!was_warning || alarm_level > last);

  last = alarm_level;

  CheckAlert();

  if (IsMuted()) {
    spoken.reset();
    return;
  }

  if (CommonInterface::GetUISettings().traffic.spoken_alerts) {
    Speak(escalated);
  } else {
    /* once turned on again, speak the current alarm as a new one */
    spoken.reset();
    spoken_time = TimeStamp::Undefined();
  }

  if (!escalated)
    return;

#ifdef HAVE_VIBRATOR
  PlayHapticFeedback(HapticFeedbackType::ALARM);
#endif

#ifdef ESP_PLATFORM
  HostTrafficAlarm(alarm_level);
#endif
}
