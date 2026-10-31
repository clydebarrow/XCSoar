// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Settings.hpp"
#include "Features.hpp"

void
SoundSettings::SetDefaults()
{
  sound_task_enabled = true;
  sound_modes_enabled = true;
  sound_deadband = 5;

#ifdef HAVE_HOST_VOLUME
  /* the host's sounds are alarms, which should be heard */
  master_volume = 100;
#else
  master_volume = 50;
#endif

  vario.SetDefaults();
}
