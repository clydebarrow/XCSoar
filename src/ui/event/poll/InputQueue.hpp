// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef USE_INJECTED_INPUT
/* no input devices: the host application injects touches with
   EventQueue::Inject() */
#elif defined(USE_LIBINPUT)
#include "libinput/LibInputHandler.hpp"
#else
#include "linux/MergeMouse.hpp"
#include "linux/Input.hpp"
#endif

#include "ui/dim/Point.hpp"
#include "ui/dim/Size.hpp"

#include <cstdint>

enum class DisplayOrientation : uint8_t;

namespace UI {

class EventQueue;
struct Event;

class InputEventQueue final {
#ifdef USE_INJECTED_INPUT
#elif defined(USE_LIBINPUT)
  LibInputHandler libinput_handler;
#else
  MergeMouse merge_mouse;
  LinuxInputDevice keyboard;
  LinuxInputDevice mouse;
#endif

public:
  explicit InputEventQueue(EventQueue &queue) noexcept;
  ~InputEventQueue() noexcept;

  void SetScreenSize([[maybe_unused]] PixelSize screen_size) noexcept {
  #ifdef USE_INJECTED_INPUT
  #elif defined(USE_LIBINPUT)
    libinput_handler.SetScreenSize(screen_size);
  #else
    merge_mouse.SetScreenSize(screen_size);
  #endif
  }

  void SetDisplayOrientation([[maybe_unused]] DisplayOrientation orientation) {
#ifdef USE_INJECTED_INPUT
#elif defined(USE_LIBINPUT)
    libinput_handler.SetDisplayOrientation(orientation);
#else
    merge_mouse.SetDisplayOrientation(orientation);
#endif
  }

  bool UsesSystemRotatedInput() const noexcept {
#ifdef USE_LIBINPUT
    return libinput_handler.UsesSystemRotatedInput();
#else
    return false;
#endif
  }

  bool HasPointer() const noexcept {
#ifdef USE_INJECTED_INPUT
    /* a touch screen */
    return false;
#elif defined(USE_LIBINPUT)
    return libinput_handler.HasPointer();
#else
    return merge_mouse.HasPointer();
#endif
  }

#ifdef USE_LIBINPUT
  bool HasTouchScreen() const noexcept {
    return libinput_handler.HasTouchScreen();
  }

  bool HasKeyboard() const noexcept {
    return libinput_handler.HasKeyboard();
  }
#endif

  PixelPoint GetMousePosition() const noexcept {
#ifdef USE_INJECTED_INPUT
    return {};
#elif defined(USE_LIBINPUT)
    return PixelPoint(libinput_handler.GetX(), libinput_handler.GetY());
#else
    return merge_mouse.GetPosition();
#endif
  }

  void Suspend() noexcept {
#ifdef USE_LIBINPUT
    libinput_handler.Suspend();
#endif
  }

  void Resume() noexcept {
#ifdef USE_LIBINPUT
    libinput_handler.Resume();
#endif
  }

  bool Generate(Event &event) noexcept;
};

} // namespace UI
