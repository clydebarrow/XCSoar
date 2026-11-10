// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FlightTrackActions.hpp"
#include "IGC/FlightTrack.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Dialogs/Message.hpp"
#include "Language/Language.hpp"
#include "Projection/WindowProjection.hpp"
#include "Pan.hpp"
#include "UIGlobals.hpp"
#include "LogFile.hpp"
#include "system/Path.hpp"

#include <algorithm>
#include <memory>

static std::shared_ptr<const FlightTrack> shown_track;

/**
 * Zoom #map so that #bounds fit, with a little room around them.
 */
static void
ZoomTo(GlueMapWindow &map, const GeoBounds &bounds) noexcept
{
  const GeoPoint center = bounds.GetCenter();
  const double radius = std::max({
      center.Distance(bounds.GetNorthWest()),
      center.Distance(bounds.GetSouthEast()),
      1000.,
    });

  WindowProjection projection = map.VisibleProjection();
  projection.SetScaleFromRadius(radius * 1.1);
  map.SetMapScale(projection.GetMapScale());
}

bool
ShowFlightTrack(Path path) noexcept
{
  auto track = std::make_shared<FlightTrack>();
  try {
    track->Load(path);
  } catch (...) {
    LogError(std::current_exception(), "Failed to read the flight");
    ShowMessageBox(_("The flight could not be read."), _("Show track"),
                   MB_OK | MB_ICONERROR);
    return false;
  }

  if (track->empty()) {
    ShowMessageBox(_("The flight has no valid fixes."), _("Show track"),
                   MB_OK | MB_ICONINFORMATION);
    return false;
  }

  const GeoBounds bounds = track->GetBounds();
  if (!PanTo(bounds.GetCenter()))
    return false;

  GlueMapWindow *map = UIGlobals::GetMapIfActive();
  if (map == nullptr)
    return false;

  shown_track = std::move(track);
  map->SetFlightTrack(shown_track);
  ZoomTo(*map, bounds);
  map->FullRedraw();
  return true;
}

void
HideFlightTrack() noexcept
{
  shown_track.reset();

  GlueMapWindow *map = UIGlobals::GetMap();
  if (map != nullptr) {
    map->SetFlightTrack(nullptr);
    map->FullRedraw();
  }
}

bool
IsFlightTrackShown() noexcept
{
  return shown_track != nullptr;
}
