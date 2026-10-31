// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/WindowWidget.hpp"
#include "ui/event/PeriodicTimer.hpp"

class TrafficMonitor;

/**
 * Covers the main window while a collision alarm lasts: the direction
 * of the traffic, its height, distance and time to close proximity,
 * with buttons to dismiss the alert and to mute the aircraft.  It shows
 * the current FlarmStatus from the blackboard.
 */
class TrafficAlertWidget final : public WindowWidget {
  TrafficMonitor &monitor;

  /**
   * Checks the alarm while shown: without new GPS fixes the monitors
   * do not run, and the alert would stay with stale data.
   */
  UI::PeriodicTimer timer;

public:
  explicit TrafficAlertWidget(TrafficMonitor &_monitor) noexcept;

  ~TrafficAlertWidget() noexcept override;

  /**
   * Repaint with the latest alarm.
   */
  void Update() noexcept;

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
};
