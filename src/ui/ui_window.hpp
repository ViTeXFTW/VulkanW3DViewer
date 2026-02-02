#pragma once

#include <string>

namespace w3d {

// Forward declaration
struct UIContext;

/// Base class for all ImGui windows in the application.
/// Provides a common interface for window management and visibility control.
///
/// To create a new window:
/// 1. Inherit from UIWindow
/// 2. Implement draw(UIContext&) and name()
/// 3. Register with UIManager via addWindow()
class UIWindow {
public:
  virtual ~UIWindow() = default;

  /// Draw the window contents. Called every frame when visible.
  /// @param ctx Shared UI context containing application state
  virtual void draw(UIContext &ctx) = 0;

  /// Get the unique name of this window (used for ImGui window ID)
  virtual const char *name() const = 0;

  /// Check if window is visible
  bool isVisible() const { return visible_; }

  /// Set window visibility
  void setVisible(bool visible) { visible_ = visible; }

  /// Toggle window visibility
  void toggleVisible() { visible_ = !visible_; }

  /// Get mutable visibility pointer (for ImGui::MenuItem binding)
  bool *visiblePtr() { return &visible_; }

  /// Called at the start of each frame (before draw)
  virtual void onFrameBegin(UIContext & /*ctx*/) {}

  /// Called at the end of each frame (after draw)
  virtual void onFrameEnd(UIContext & /*ctx*/) {}

protected:
  bool visible_ = true;
};

} // namespace w3d
