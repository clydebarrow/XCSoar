/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "GlueMapWindow.hpp"
#include "Components.hpp"
#include "Interface.hpp"
#include "DrawThread.hpp"

GlueMapWindow::GlueMapWindow()
  :idle_robin(2), is_gesture(false), ignore_single_click(true) {}

/**
 * This idle function allows progressive scanning of visibility etc
 */
bool
GlueMapWindow::Idle(const bool do_force)
{
  bool still_dirty=false;

  // StartTimer();

  if (do_force) {
    idle_robin = 2;
    terrain_dirty = true;
    topology_dirty = true;
    weather_dirty = true;

    UpdateTopologyCache();
  }

  do {
    idle_robin = (idle_robin + 1) % 3;
    switch (idle_robin) {
    case 0:
      /// \todo bug: this will delay servicing if EnableTopology was false and then
      /// switched on, until do_force is true again

      UpdateTopology(do_force);
      break;

    case 1:
      UpdateTerrain();
      break;

    case 2:
      UpdateWeather();
      break;
    }

  } while (RenderTimeAvailable() &&
           !draw_thread->is_triggered() &&
           (still_dirty = terrain_dirty || topology_dirty || weather_dirty));

  return still_dirty;
}

void
GlueMapWindow::SetFullScreen(bool full_screen)
{
  if (full_screen == GetFullScreen())
    return;

  XCSoarInterface::SetSettingsMap().FullScreen = full_screen;
  SetMapRect(full_screen ? MapRectBig : MapRectSmall);
}

/**
 * Triggers the drawTrigger and is called by
 * the on_mouse_up event in case of panning
 */
void
GlueMapWindow::RefreshMap()
{
  MapWindowTimer::InterruptTimer();
  draw_thread->trigger_redraw();
}
