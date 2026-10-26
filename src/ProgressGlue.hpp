// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct PixelRect;

namespace ProgressGlue {

/**
 * Creates or updates the ProgressWindow
 * @param text the text inside the progress bar
 */
void Create(const char *text) noexcept;

/**
 * Show no progress window from now on: the program is shutting down.
 */
void Disable() noexcept;

void Move(const PixelRect &rc) noexcept;

/**
 * Closes the ProgressWindow
 */
void Close() noexcept;

/**
 * Updates the ProgressWindow to go up one step
 */
void Step() noexcept;

void SetValue(unsigned value) noexcept;
void SetRange(unsigned value) noexcept;
void SetStep(int step) noexcept;

} // namespace ProgressGlue
