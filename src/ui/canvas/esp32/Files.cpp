// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/custom/Files.hpp"
#include "ui/canvas/FontSearch.hpp"

/* fonts on the SD card take precedence over the ones embedded in the
   firmware (see esp32/src/RomFs.cpp) */
static constexpr const char *const font_search_paths[] = {
  "/sdcard/XCSoar/fonts",
  "/rom/fonts",
  nullptr
};

static constexpr const char *const all_font_paths[] = {
  "DejaVuSansCondensed.ttf",
  "Roboto-Regular.ttf",
  "DroidSans.ttf",
  nullptr
};

static constexpr const char *const all_bold_font_paths[] = {
  "DejaVuSansCondensed-Bold.ttf",
  "Roboto-Bold.ttf",
  "DroidSans-Bold.ttf",
  nullptr
};

static constexpr const char *const all_italic_font_paths[] = {
  "DejaVuSansCondensed-Oblique.ttf",
  "Roboto-Italic.ttf",
  nullptr
};

static constexpr const char *const all_bold_italic_font_paths[] = {
  "DejaVuSansCondensed-BoldOblique.ttf",
  "Roboto-BoldItalic.ttf",
  nullptr
};

static constexpr const char *const all_monospace_font_paths[] = {
  "DejaVuSansMono.ttf",
  "DroidSansMono.ttf",
  nullptr
};

AllocatedPath
FindDefaultFont() noexcept
{
  return FontSearch::FindFile(font_search_paths, all_font_paths);
}

AllocatedPath
FindDefaultBoldFont() noexcept
{
  return FontSearch::FindFile(font_search_paths, all_bold_font_paths);
}

AllocatedPath
FindDefaultItalicFont() noexcept
{
  return FontSearch::FindFile(font_search_paths, all_italic_font_paths);
}

AllocatedPath
FindDefaultBoldItalicFont() noexcept
{
  return FontSearch::FindFile(font_search_paths, all_bold_italic_font_paths);
}

AllocatedPath
FindDefaultMonospaceFont() noexcept
{
  return FontSearch::FindFile(font_search_paths, all_monospace_font_paths);
}
