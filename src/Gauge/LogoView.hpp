// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Bitmap.hpp"
#include "ui/canvas/Font.hpp"

#include <string>

struct PixelRect;
struct PixelPoint;
class Canvas;

class LogoView {
  Bitmap logo, big_logo, huge_logo, title, big_title, huge_title;
  Bitmap logo_rgba, big_logo_rgba, huge_logo_rgba;
  Bitmap title_rgba, big_title_rgba, huge_title_rgba;
  Bitmap white_title, huge_white_title;

  Font font;
#ifndef NDEBUG
  Font bold_font;
#endif

  /* the club branding, if SetBranding() was called */
  Bitmap club_logo;
  Font organisation_font, glider_font;

public:
  /**
   * A club's name and logo and the glider, drawn above a smaller
   * XCSoar logo.
   */
  struct Branding {
    std::string organisation;

    /** the glider's type and registration, in one line */
    std::string glider;

    /** an image file; empty for none */
    std::string logo_path;
  };

  /**
   * Draw this branding in all LogoViews created after this call.  The
   * object must stay valid while they exist.
   */
  static void SetBranding(const Branding *branding) noexcept;

  [[gnu::pure]]
  static bool HasBranding() noexcept;

  LogoView() noexcept;

  /**
   * Draws the XCSoar logo and the version number into the Canvas.
   *
   * @param canvas the Canvas to draw on
   * @param rc the region within the Canvas to draw into
   * @param dark_mode if true, use light-on-dark colors and white title
   */
  void draw(Canvas &canvas, const PixelRect &rc,
            bool dark_mode = false) noexcept;

private:
  void DrawXCSoar(Canvas &canvas, const PixelRect &rc, bool dark_mode,
                  PixelPoint version_position) noexcept;
  void DrawBranding(Canvas &canvas, const PixelRect &rc,
                    bool dark_mode) noexcept;
};
