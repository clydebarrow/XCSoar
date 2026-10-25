// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PixelTraits.hpp"

#ifdef GREYSCALE
using ActivePixelTraits = GreyscalePixelTraits;
#elif defined(USE_RGB565)
using ActivePixelTraits = RGB565PixelTraits;
#else
using ActivePixelTraits = BGRAPixelTraits;
#endif
