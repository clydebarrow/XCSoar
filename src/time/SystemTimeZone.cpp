// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SystemTimeZone.hpp"

#ifdef _WIN32
#include <timezoneapi.h>
#else
#include <time.h>
#endif

int
GetCurrentTimeZoneOffset() noexcept
{
#ifdef _WIN32
  TIME_ZONE_INFORMATION tzi;
  const DWORD result = GetTimeZoneInformation(&tzi);
  if (result == TIME_ZONE_ID_INVALID)
    return 0;

  int offset = -tzi.Bias * 60;

  if (result == TIME_ZONE_ID_DAYLIGHT)
    offset -= tzi.DaylightBias * 60;
  else
    offset -= tzi.StandardBias * 60;

  return offset;
#else
  /* localtime_r() is not required to consult the time zone
     configuration again, and glibc indeed does not: without this
     tzset(), a time zone which was reconfigured after our first call
     (e.g. after travelling) would never be picked up.  Daylight saving
     time transitions would still work, because those rules are part of
     the time zone which was loaded already */
  tzset();

  const time_t t = time(nullptr);

  struct tm tm;
  if (localtime_r(&t, &tm) == nullptr)
    return 0;

#ifdef ESP_PLATFORM
  /* newlib has no tm_gmtoff: read the UTC time as local time, which
     gives a time that is too early by the offset */
  struct tm utc;
  if (gmtime_r(&t, &utc) == nullptr)
    return 0;
  utc.tm_isdst = tm.tm_isdst;
  return (int)(t - mktime(&utc));
#else
  /* tm_gmtoff is a BSD extension which is available on all of our
     POSIX targets (glibc, Bionic, macOS); it already includes the
     daylight saving time correction */
  return (int)tm.tm_gmtoff;
#endif
#endif
}
