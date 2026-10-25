#pragma once

#ifdef __linux__
#define USE_EVENTFD
#define USE_SIGNALFD
#define USE_EPOLL
#elif defined(ESP_PLATFORM)
/* ESP-IDF has no pipe(), but esp_vfs_eventfd provides eventfd() */
#define USE_EVENTFD
#endif

#define HAVE_THREADED_EVENT_LOOP
