// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "EventFD.hxx"
#include "system/Error.hxx"

#include <cassert>

#ifdef ESP_PLATFORM
/* esp_vfs_eventfd accepts no flags; its read() never blocks */
#include <esp_vfs_eventfd.h>
using eventfd_t = uint64_t;
static constexpr int EVENTFD_FLAGS = 0;
#else
#include <sys/eventfd.h>
static constexpr int EVENTFD_FLAGS = EFD_NONBLOCK|EFD_CLOEXEC;
#endif

EventFD::EventFD()
	:fd(::eventfd(0, EVENTFD_FLAGS))
{
	if (!fd.IsDefined())
		throw MakeErrno("eventfd() failed");
}

bool
EventFD::Read() noexcept
{
	assert(fd.IsDefined());

	eventfd_t value;
	return fd.Read(std::as_writable_bytes(std::span{&value, 1})) == (ssize_t)sizeof(value);
}

void
EventFD::Write() noexcept
{
	assert(fd.IsDefined());

	static constexpr eventfd_t value = 1;
	[[maybe_unused]] ssize_t nbytes =
		fd.Write(&value, sizeof(value));
}
