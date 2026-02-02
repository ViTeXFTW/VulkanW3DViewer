#pragma once

#include "ui_window.hpp"

namespace w3d {

/// Floating tooltip that appears near the mouse cursor when hovering over objects.
/// Displays the type and name of the hovered mesh, bone, or joint.
///
/// Note: This window behaves differently from standard windows:
/// - Always visible (ignores visibility toggle)
/// - Positioned at mouse cursor
/// - No title bar or decorations
/// - Not in View menu (internal window)
class HoverTooltip : public UIWindow {
public:
  HoverTooltip();

  // UIWindow interface
  void draw(UIContext &ctx) override;
  const char *name() const override { return "##HoverTooltip"; }
};

} // namespace w3d
