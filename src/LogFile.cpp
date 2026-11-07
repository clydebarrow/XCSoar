// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DataFilePath.hpp"
#include "LogFile.hpp"
#include "LocalPath.hpp"
#include "Asset.hpp"
#include "io/FileOutputStream.hxx"
#include "io/BufferedOutputStream.hxx"
#include "Formatter/TimeFormatter.hpp"
#include "time/BrokenDateTime.hpp"
#include "system/Path.hpp"
#include "system/FileUtil.hpp"
#include "io/UniqueFileDescriptor.hxx"
#include "util/Exception.hxx"

#include <fmt/format.h>

#include <cwchar>

#ifdef ESP_PLATFORM
#include <algorithm>
#include <string>
#include <vector>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <windef.h> // for MAX_PATH

#ifdef ANDROID
#include <android/log.h>
#include <fcntl.h>
#endif

#ifdef ESP_PLATFORM

/**
 * How many earlier logs are kept, as "xcsoar-YYYYMMDD-HHMMSS.log"
 */
static constexpr std::size_t MAX_OLD_LOGS = 10;

/**
 * Keep the log of the last run, named after the time it was last
 * written to, so that the log of a flight survives the restarts after
 * it; delete all but the newest #MAX_OLD_LOGS.
 */
static void
ArchiveLog(Path path) noexcept
{
  if (!File::Exists(path))
    return;

  const BrokenDateTime t{File::GetLastModification(path)};
  const auto name = fmt::format("xcsoar-{:04}{:02}{:02}-{:02}{:02}{:02}",
                                t.year, t.month, t.day,
                                t.hour, t.minute, t.second);
  auto archived = LocalPath(fmt::format("{}.log", name).c_str());
  for (unsigned n = 2; File::Exists(archived) && n < 100; ++n)
    archived = LocalPath(fmt::format("{}-{}.log", name, n).c_str());
  File::Replace(path, archived);

  struct Collector final : File::Visitor {
    std::vector<std::string> names;

    void Visit([[maybe_unused]] Path full, Path filename) override {
      names.emplace_back(filename.c_str());
    }
  } collector;

  try {
    Directory::VisitSpecificFiles(LocalPath(""), "xcsoar-[0-9]*.log", collector);
  } catch (...) {
    return;
  }

  /* the names sort by time, oldest first */
  auto &names = collector.names;
  if (names.size() <= MAX_OLD_LOGS)
    return;

  std::sort(names.begin(), names.end());
  for (std::size_t i = 0; i + MAX_OLD_LOGS < names.size(); ++i)
    File::Delete(LocalPath(names[i].c_str()));
}

#endif

static FileOutputStream
OpenLog()
{
  static bool initialised = false;
  static AllocatedPath path = nullptr;

  if (!initialised) {
    initialised = true;

    /* delete the obsolete log file */
    File::Delete(LocalPath("xcsoar-startup.log"));

    path = LocalPath("xcsoar.log");

#ifdef ESP_PLATFORM
    ArchiveLog(path);
#else
    File::Replace(path, LocalPath("xcsoar-old.log"));
#endif

#ifdef ANDROID
    /* redirect stdout/stderr to xcsoar-startup.log on Android so we
       get debug logs from libraries and output from child processes
       there */
    UniqueFileDescriptor fd;
    if (fd.Open(path.c_str(), O_APPEND|O_CREAT|O_WRONLY, 0666)) {
      fd.CheckDuplicate(FileDescriptor(STDOUT_FILENO));
      fd.CheckDuplicate(FileDescriptor(STDERR_FILENO));
    }
#endif
  }

  return FileOutputStream{path, FileOutputStream::Mode::APPEND_OR_CREATE};
}

void
LogString(std::string_view s) noexcept
{
#ifdef ANDROID
  __android_log_print(ANDROID_LOG_INFO, "XCSoar", "%.*s",
                      int(s.size()), s.data());
#elif defined(HAVE_POSIX) && (!defined(NDEBUG) || defined(ESP_PLATFORM))
  /* the ESP32 has no terminal; this goes to the serial console */
  fprintf(stderr, "%.*s\n",
          int(s.size()), s.data());
#endif

  try {
    auto fos = OpenLog();
    BufferedOutputStream bos{fos};

    bos.Write('[');

    {
      char time_buffer[32];
      FormatISO8601(time_buffer, BrokenDateTime::NowUTC());
      bos.Write(time_buffer);
    }

    bos.Write("] ");
    bos.Write(s);
    bos.NewLine();

    bos.Flush();
    fos.Commit();
  } catch (...) {
  }
}

void
LogVFmt(fmt::string_view format_str, fmt::format_args args) noexcept
{
	fmt::memory_buffer buffer;
#if FMT_VERSION >= 80000
	fmt::vformat_to(std::back_inserter(buffer), format_str, args);
#else
	fmt::vformat_to(buffer, format_str, args);
#endif
	LogString({buffer.data(), buffer.size()});
}

void
LogFormat(const char *fmt, ...) noexcept
{
  char buf[MAX_PATH];
  va_list ap;

  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
  va_end(ap);

  LogString(buf);
}

void
LogError(std::exception_ptr e) noexcept
{
  LogString(GetFullMessage(e));
}

void
LogError(std::exception_ptr e, const char *msg) noexcept
{
  LogFormat("%s: %s", msg, GetFullMessage(e).c_str());
}
