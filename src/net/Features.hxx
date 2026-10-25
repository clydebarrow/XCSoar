// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#define HAVE_TCP

/* lwIP (ESP-IDF) has no local sockets */
#if defined(HAVE_POSIX) && !defined(ESP_PLATFORM)
#define HAVE_UN
#endif
