// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/*
 * Runs XCSoarEsp32::Run() on the host for a few frames, injecting
 * touches, and writes the last frame to a PPM file.
 */

#include "XCSoarEsp32.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

static constexpr unsigned WIDTH = 800, HEIGHT = 480;

static std::vector<unsigned char> last_frame;
static unsigned frame_count;

/* touches, one per frame: a tap on the pause button, then a drag
   that is still held when the test ends */
static void
InjectTouches() noexcept
{
  using XCSoarEsp32::InjectPointer;
  using XCSoarEsp32::PointerAction;

  const unsigned n = frame_count++;
  if (n == 2)
    InjectPointer(PointerAction::DOWN, WIDTH - 40, 40);
  else if (n == 3)
    InjectPointer(PointerAction::UP, WIDTH - 40, 40);
  else if (n == 5)
    InjectPointer(PointerAction::DOWN, 100, 300);
  else if (n > 5 && n <= 15)
    InjectPointer(PointerAction::MOVE, 100 + (n - 5) * 30,
                  300 - (n - 5) * 15);
}

/* keep the last frame as RGB for the PPM file */
static void
FrameReady(const void *pixels, unsigned pitch, const XCSoarEsp32::Rect *,
           unsigned, void *) noexcept
{
  InjectTouches();

  last_frame.clear();
  const bool rgb565 = XCSoarEsp32::BytesPerPixel() == 2;
  for (unsigned y = 0; y < HEIGHT; ++y) {
    const auto *row = static_cast<const unsigned char *>(pixels) + y * pitch;
    for (unsigned x = 0; x < WIDTH; ++x) {
      if (rgb565) {
        uint16_t v;
        memcpy(&v, row + x * 2, sizeof(v));
        const unsigned r = v >> 11, g = (v >> 5) & 0x3f, b = v & 0x1f;
        last_frame.insert(last_frame.end(),
                          {(unsigned char)((r << 3) | (r >> 2)),
                           (unsigned char)((g << 2) | (g >> 4)),
                           (unsigned char)((b << 3) | (b >> 2))});
      } else {
        const unsigned char *bgra = row + x * 4;
        last_frame.insert(last_frame.end(), {bgra[2], bgra[1], bgra[0]});
      }
    }
  }
}

int
main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "Usage: %s OUTPUT.ppm\n", argv[0]);
    return EXIT_FAILURE;
  }

  const XCSoarEsp32::Config config{
    .width = WIDTH,
    .height = HEIGHT,
    .frame_ready = FrameReady,
    .ctx = nullptr,
    .data_path = nullptr,
    .dpi = 0,
    .frame_interval_ms = 10,
    .max_frames = 30,
  };

  if (!XCSoarEsp32::RunDemo(config))
    return EXIT_FAILURE;

  FILE *file = fopen(argv[1], "wb");
  if (file == nullptr) {
    perror(argv[1]);
    return EXIT_FAILURE;
  }

  fprintf(file, "P6\n%u %u\n255\n", WIDTH, HEIGHT);
  fwrite(last_frame.data(), 1, last_frame.size(), file);
  fclose(file);
  return EXIT_SUCCESS;
}
