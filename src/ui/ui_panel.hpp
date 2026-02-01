#pragma once

namespace w3d {

// Forward declaration
struct UIContext;

/// Base class for UI panels that compose into windows.
/// Panels are lightweight drawable components that can be combined
/// to create complex window layouts.
///
/// To create a new panel:
/// 1. Inherit from UIPanel
/// 2. Implement draw(UIContext&) and title()
/// 3. Add to a window (e.g., ViewportWindow) via addPanel()
class UIPanel {
public:
  virtual ~UIPanel() = default;

  /// Draw the panel contents
  /// @param ctx Shared UI context containing application state
  virtual void draw(UIContext& ctx) = 0;

  /// Get the title of this panel (displayed in collapsing header)
  virtual const char* title() const = 0;

  /// Check if panel is expanded
  bool isExpanded() const { return expanded_; }

  /// Set panel expanded state
  void setExpanded(bool expanded) { expanded_ = expanded; }

  /// Check if panel is enabled/active
  bool isEnabled() const { return enabled_; }

  /// Enable or disable the panel
  void setEnabled(bool enabled) { enabled_ = enabled; }

protected:
  bool expanded_ = true;
  bool enabled_ = true;
};

} // namespace w3d
