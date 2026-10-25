// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/*
 * Runs XCSoarEsp32::Run() on the host for a few frames and writes the
 * last one to a PPM file.
 */

#include "XCSoarEsp32.hpp"

#include <cstdio>
#include <cstdlib>
#include <vector>

static constexpr unsigned WIDTH = 800, HEIGHT = 480;

static void
FrameReady(void *) noexcept
{
}

int
main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "Usage: %s OUTPUT.ppm\n", argv[0]);
    return EXIT_FAILURE;
  }

  std::vector<uint16_t> frame(WIDTH * HEIGHT);

  const XCSoarEsp32::Config config{
    .width = WIDTH,
    .height = HEIGHT,
    .frame = frame.data(),
    .frame_ready = FrameReady,
    .ctx = nullptr,
    .frame_interval_ms = 10,
    .max_frames = 30,
  };

  if (!XCSoarEsp32::Run(config))
    return EXIT_FAILURE;

  FILE *file = fopen(argv[1], "wb");
  if (file == nullptr) {
    perror(argv[1]);
    return EXIT_FAILURE;
  }

  fprintf(file, "P6\n%u %u\n255\n", WIDTH, HEIGHT);
  for (const uint16_t pixel : frame) {
    const unsigned char rgb[3] = {
      (unsigned char)((pixel >> 11) << 3),
      (unsigned char)(((pixel >> 5) & 0x3f) << 2),
      (unsigned char)((pixel & 0x1f) << 3),
    };
    fwrite(rgb, sizeof(rgb), 1, file);
  }

  fclose(file);
  return EXIT_SUCCESS;
}
