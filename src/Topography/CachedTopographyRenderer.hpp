// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "TopographyRenderer.hpp"
#include "Renderer/TransparentRendererCache.hpp"

#if !defined(ENABLE_OPENGL) && !defined(ESP_PLATFORM)
/* draw into a cache, which is reused while the map does not move; on
   ESP32 the screen-sized buffer costs too much memory */
#define USE_TOPOGRAPHY_CACHE
#endif

/**
 * Class used to manage and render vector topography layers
 */
class CachedTopographyRenderer {
  TopographyRenderer renderer;

#ifdef USE_TOPOGRAPHY_CACHE
  TransparentRendererCache cache;

  unsigned last_serial = 0;
#endif

public:
  CachedTopographyRenderer(const TopographyStore &store,
                           const TopographyLook &look) noexcept
    :renderer(store, look)
  {}

  void Flush() noexcept {
#ifdef USE_TOPOGRAPHY_CACHE
    cache.Invalidate();
#endif
  }

#ifndef USE_TOPOGRAPHY_CACHE
  void Draw(Canvas &canvas, const WindowProjection &projection) noexcept {
    renderer.Draw(canvas, projection);
  }
#else
  void Draw(Canvas &canvas, const WindowProjection &projection) noexcept;
#endif

  void DrawLabels(Canvas &canvas, const WindowProjection &projection,
                  LabelBlock &label_block) noexcept {
    renderer.DrawLabels(canvas, projection, label_block);
  }
};
