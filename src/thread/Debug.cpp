// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "thread/Debug.hpp"
#include "thread/Handle.hpp"

#ifndef NDEBUG

#if defined(ANDROID) || defined(ESP_PLATFORM)
/* on Android and on the ESP32, XCSoar's "main" thread is different
   from the process main thread */
static ThreadHandle main_thread;
#else
static ThreadHandle main_thread = ThreadHandle::GetCurrent();
#endif

#if defined(ANDROID) || defined(ESP_PLATFORM)

void
InitThreadDebug()
{
  main_thread = ThreadHandle::GetCurrent();
}

#endif

bool
InMainThread()
{
  return main_thread.IsInside();
}

#endif /* !NDEBUG */
