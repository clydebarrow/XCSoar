// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/Bitmap.hpp"
#include "Screen/Debug.hpp"
#include "ResourceLoader.hpp"
#include "ResourceId.hpp"

#ifdef USE_MEMORY_CANVAS
#ifdef __ARM_NEON__
#include "ui/canvas/memory/NEON.hpp"
#endif
#include "ui/canvas/memory/RasterCanvas.hpp"
#include "ui/canvas/memory/PixelOperations.hpp"
#endif

Bitmap::Bitmap(ResourceId id)
{
  Load(id);
}

bool
Bitmap::Load(ResourceId id, Type type)
{
  assert(IsScreenInitialized());

  ResourceLoader::Data data = ResourceLoader::Load(id);
  if (data.data() == nullptr)
    return false;

  return Load(data, type);
}

#ifdef USE_MEMORY_CANVAS

bool
Bitmap::LoadStretch(ResourceId id, unsigned zoom)
{
  assert(zoom > 0);

  if (!Load(id))
    return false;

  Scale(GetSize() * zoom);
  return true;
}

void
Bitmap::Scale(PixelSize new_size) noexcept
{
  const bool _has_colors = has_colors;

  Bitmap scaled;
  scaled.Create(new_size);

  {
    RasterCanvas canvas{scaled.buffer};
    canvas.ScaleRectangle({}, new_size,
                          buffer.data, buffer.pitch, buffer.size);
  }

  *this = std::move(scaled);
  has_colors = _has_colors;
}

#endif
