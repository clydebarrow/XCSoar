/* SPDX-License-Identifier: MIT */
/* c-ares build configuration for ESP-IDF with lwIP (written for
   esp32/ThirdParty.cmake instead of the one c-ares generates) */

#ifndef __CARES_BUILD_H
#define __CARES_BUILD_H

#define CARES_TYPEOF_ARES_SOCKLEN_T socklen_t
#define CARES_TYPEOF_ARES_SSIZE_T ssize_t

#define CARES_HAVE_SYS_TYPES_H
#define CARES_HAVE_SYS_SOCKET_H
#define CARES_HAVE_SYS_SELECT_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

#endif /* __CARES_BUILD_H */
