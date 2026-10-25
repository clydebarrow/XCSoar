// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCSoarEsp32.hpp"
#include "Host.hpp"
#include "thread/Debug.hpp"

#include <fmt/format.h>

#include <string>

/* XCSoar's main(), see src/XCSoar.cpp */
int
XCSoarMain(int argc, char **argv);

namespace XCSoarEsp32 {

bool
Run(const Config &config) noexcept
{
  /* the calling thread is XCSoar's main thread */
  InitThreadDebug();
  ConnectDisplay(config);

  /* XCSoar takes its screen size and data path from the command
     line */
  std::string size = fmt::format("-{}x{}", config.width, config.height);
  std::string datapath = fmt::format("-datapath={}", config.data_path);
  char program[] = "xcsoar";
  char *argv[] = {
    program,
    size.data(),
    datapath.data(),
    nullptr,
  };

  return XCSoarMain(3, argv) == 0;
}

} // namespace XCSoarEsp32
