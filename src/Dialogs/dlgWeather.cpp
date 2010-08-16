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

#include "Dialogs/Internal.hpp"
#include "Units.hpp"
#include "LocalTime.hpp"
#include "Terrain/RasterWeather.hpp"
#include "Screen/Layout.hpp"
#include "DataField/Enum.hpp"
#include "MainWindow.hpp"
#include "Components.hpp"

#include <stdio.h>

static WndForm *wf = NULL;

static void
OnCloseClicked(WindowControl * Sender)
{
  (void)Sender;
  wf->SetModalResult(mrOK);
}

static void
OnDisplayItemData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  switch (Mode) {
  case DataField::daGet:
    Sender->Set((int)RASP.GetParameter());
    break;
  case DataField::daPut:
  case DataField::daChange:
    RASP.SetParameter(Sender->GetAsInteger());
    break;
  }
}

static void
RASPGetTime(DataField *Sender)
{
  DataFieldEnum* dfe;
  dfe = (DataFieldEnum*)Sender;
  int index = 0;
  for (unsigned i = 0; i < RasterWeather::MAX_WEATHER_TIMES; i++) {
    if (RASP.isWeatherAvailable(i)) {
      if (RASP.GetTime() == i)
        Sender->Set(index);

      index++;
    }
  }
}

static void
RASPSetTime(DataField *Sender)
{
  int index = 0;
  if (Sender->GetAsInteger() <= 0) {
    RASP.SetTime(0);
    return;
  }
  for (unsigned i = 0; i < RasterWeather::MAX_WEATHER_TIMES; i++) {
    if (RASP.isWeatherAvailable(i)) {
      if (index == Sender->GetAsInteger())
        RASP.SetTime(i);

      index++;
    }
  }
}

static void
OnTimeData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  switch (Mode) {
  case DataField::daGet:
    RASPGetTime(Sender);
    break;
  case DataField::daPut:
  case DataField::daChange:
    RASPSetTime(Sender);
    break;
  }
}

void
OnWeatherHelp(WindowControl * Sender)
{
  WndProperty *wp = (WndProperty*)Sender;
  int type = wp->GetDataField()->GetAsInteger();
  TCHAR caption[256];
  _tcscpy(caption, _("Weather parameters"));
  const TCHAR *label = RASP.ItemLabel(type);
  if (label != NULL) {
    _tcscat(caption, _T(": "));
    _tcscat(caption, label);
  }

  switch (type) {
  case 0:
    dlgHelpShowModal(XCSoarInterface::main_window,
                     caption, _T("Display terrain on map, no weather data displayed."));
    break;
  case 1:
    dlgHelpShowModal(XCSoarInterface::main_window,
                     caption, _T("Average dry thermal updraft strength near mid-BL height.  Subtract glider descent rate to get average vario reading for cloudless thermals.  Updraft strengths will be stronger than this forecast if convective clouds are present, since cloud condensation adds buoyancy aloft (i.e. this negects \"cloudsuck\").  W* depends upon both the surface heating and the BL depth."));
    break;
  case 2:
    dlgHelpShowModal(XCSoarInterface::main_window,
                     caption, _T("The speed and direction of the vector-averaged wind in the BL.  This prediction can be misleading if there is a large change in wind direction through the BL."));
    break;
  case 3:
    dlgHelpShowModal(XCSoarInterface::main_window,
                     caption, _T("Height of the top of the mixing layer, which for thermal convection is the average top of a dry thermal.  Over flat terrain, maximum thermalling heights will be lower due to the glider descent rate and other factors.  In the presence of clouds (which release additional buoyancy aloft, creating \"cloudsuck\") the updraft top will be above this forecast, but the maximum thermalling height will then be limited by the cloud base.  Further, when the mixing results from shear turbulence rather than thermal mixing this parameter is not useful for glider flying. "));
    break;
  case 4:
    dlgHelpShowModal(XCSoarInterface::main_window,
                     caption, _T("This parameter estimates the height above ground at which the average dry updraft strength drops below 225 fpm and is expected to give better quantitative numbers for the maximum cloudless thermalling height than the BL Top height, especially when mixing results from vertical wind shear rather than thermals.  (Note: the present assumptions tend to underpredict the max. thermalling height for dry consitions.) In the presence of clouds the maximum thermalling height may instead be limited by the cloud base.  Being for \"dry\" thermals, this parameter omits the effect of \"cloudsuck\"."));
    break;
  case 5:
    dlgHelpShowModal(XCSoarInterface::main_window,
                     caption, _T("This parameter provides an additional means of evaluating the formation of clouds within the BL and might be used either in conjunction with or instead of the other cloud prediction parameters.  It assumes a very simple relationship between cloud cover percentage and the maximum relative humidity within the BL.  The cloud base height is not predicted, but is expected to be below the BL Top height."));
    break;
  case 6:
    dlgHelpShowModal(XCSoarInterface::main_window,
                     caption, _T("The temperature at a height of 2m above ground level.  This can be compared to observed surface temperatures as an indication of model simulation accuracy; e.g. if observed surface temperatures are significantly below those forecast, then soaring conditions will be poorer than forecast."));
    break;
  case 7:
    dlgHelpShowModal(XCSoarInterface::main_window,
                     caption, _T("This parameter estimates the height at which the average dry updraft strength drops below 225 fpm and is expected to give better quantitative numbers for the maximum cloudless thermalling height than the BL Top height, especially when mixing results from vertical wind shear rather than thermals.  (Note: the present assumptions tend to underpredict the max. thermalling height for dry consitions.) In the presence of clouds the maximum thermalling height may instead be limited by the cloud base.  Being for \"dry\" thermals, this parameter omits the effect of \"cloudsuck\"."));
    break;
  case 8:
    dlgHelpShowModal(XCSoarInterface::main_window,
                     caption, _T("Maximum grid-area-averaged extensive upward or downward motion within the BL as created by horizontal wind convergence. Positive convergence is associated with local small-scale convergence lines.  Negative convergence (divergence) produces subsiding vertical motion, creating low-level inversions which limit thermalling heights."));
    break;
  };
}

static CallBackTableEntry_t CallBackTable[] = {
  DeclareCallBackEntry(OnTimeData),
  DeclareCallBackEntry(OnDisplayItemData),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnWeatherHelp),
  DeclareCallBackEntry(NULL)
};

void
dlgWeatherShowModal(void)
{

  wf = dlgLoadFromXML(CallBackTable, XCSoarInterface::main_window,
                      !Layout::landscape ?
                      _T("IDR_XML_WEATHER_L") : _T("IDR_XML_WEATHER"));
  if (wf == NULL)
    return;

  WndProperty* wp;

  wp = (WndProperty*)wf->FindByName(_T("prpTime"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_T("Now"));
    for (unsigned i = 1; i < RasterWeather::MAX_WEATHER_TIMES; i++) {
      if (RASP.isWeatherAvailable(i)) {
        TCHAR timetext[10];
        _stprintf(timetext, _T("%04d"), RASP.IndexToTime(i));
        dfe->addEnumText(timetext);
      }
    }

    RASPGetTime(dfe);

    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpDisplayItem"));
  DataFieldEnum* dfe;
  if (wp) {
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Terrain"));

    for (int i = 1; i <= 15; i++) {
      const TCHAR *label = RASP.ItemLabel(i);
      if (label != NULL)
        dfe->addEnumText(label);
    }
    dfe->Set(RASP.GetParameter());
    wp->RefreshDisplay();
  }

  wf->ShowModal();

  wp = (WndProperty*)wf->FindByName(_T("prpTime"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    RASPSetTime(dfe);
  }

  wp = (WndProperty*)wf->FindByName(_T("prpDisplayItem"));
  if (wp)
    RASP.SetParameter(wp->GetDataField()->GetAsInteger());

  delete wf;
}

/*
  Todo:
  - units conversion in routine
  - load on demand
  - time based search
  - fix dialog
  - put label in map window as to what is displayed if not terrain
      (next to AUX)
  - Draw a legend on screen?
  - Auto-advance time index of forecast if before current time
*/
