// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "../Window.hpp"
#include "util/IntrusiveList.hxx"

#include <cassert>

struct PixelPoint;
struct PixelRect;
class Window;
class Canvas;

/**
 * A container for more #Window objects.  It is used by the SDL/OpenGL
 * #ContainerWindow implementation to manage its children.
 */
class WindowList {
  using List = IntrusiveList<Window,
                             IntrusiveListMemberHookTraits<&Window::siblings>>;

  List list;

public:
  ~WindowList() noexcept {
    assert(list.empty());
  }

public:
  void Add(Window &w) noexcept {
    assert(!Contains(w));

    list.push_back(w);
  }

  void Remove(Window &w) noexcept {
    assert(Contains(w));

    list.erase(list.iterator_to(w));
  }

  /**
   * Remove and destroy all contained windows.
   */
  void Clear() noexcept;

  [[gnu::pure]]
  bool Contains(const Window &w) const noexcept;

#ifdef USE_EXTERNAL_FB
  /**
   * Repaint #target, and then, in painting order, every visible
   * window above it that overlaps what was repainted, so a part of
   * the screen can be repainted without the rest.
   *
   * @param origin the position of the list's container on #canvas
   * @param target the window to repaint, which may be inside one of
   * the windows in this list; set to nullptr once it has been painted
   * @param damage the area repainted so far, in #canvas coordinates;
   * grows with each window painted after #target
   * @return false if this cannot be done (a transparent window would
   * have to be painted over itself, or a window is partly off the
   * canvas); the caller must then repaint everything
   */
  bool PaintAbove(Canvas &canvas, PixelPoint origin, const Window *&target,
                  PixelRect &damage) noexcept;
#endif

  /**
   * Is this window covered by a sibling?
   */
  [[gnu::pure]]
  bool IsCovered(const Window &w) const noexcept;

  void BringToTop(Window &w) noexcept;
  void BringToBottom(Window &w) noexcept;
  void PlaceBelow(Window &w, Window &sibling) noexcept;

  /**
   * Locate a window by its relative coordinates.
   *
   * @return the topmost visible window at that position, which may be
   * disabled, or nullptr if there is none
   */
  [[gnu::pure]]
  Window *FindAt(PixelPoint p) noexcept;

  [[gnu::pure]]
  static Window *FindControl(List::iterator i, List::iterator end) noexcept;

  [[gnu::pure]]
  static Window *FindControl(List::reverse_iterator i,
                             List::reverse_iterator end) noexcept;

  [[gnu::pure]]
  Window *FindFirstControl() noexcept;

  [[gnu::pure]]
  Window *FindLastControl() noexcept;

  [[gnu::pure]]
  Window *FindNextChildControl(Window *reference) noexcept;

  [[gnu::pure]]
  Window *FindPreviousChildControl(Window *reference) noexcept;

  void Paint(Canvas &canvas) noexcept;
};
