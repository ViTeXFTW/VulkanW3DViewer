#pragma once

#include <memory>
#include <vector>

#include "ui_panel.hpp"
#include "ui_window.hpp"

namespace w3d {

/// Main viewport window containing multiple collapsible panels.
/// Serves as a container for ModelInfo, Animation, LOD, Camera, and Display panels.
///
/// This window uses a composition pattern where panels are added dynamically,
/// making it easy to extend with new panels in the future.
///
/// Usage:
///   ViewportWindow viewport;
///   // Panels are added automatically in constructor, or manually:
///   viewport.addPanel<CustomPanel>();
class ViewportWindow : public UIWindow {
public:
  /// Construct viewport with default panels
  ViewportWindow();

  // UIWindow interface
  void draw(UIContext &ctx) override;
  const char *name() const override { return "Viewport"; }

  /// Add a panel to this window
  /// Returns pointer to the created panel for further configuration
  template <typename T, typename... Args>
  T *addPanel(Args &&...args);

  /// Get a panel by type (returns nullptr if not found)
  template <typename T>
  T *getPanel();

  /// Get number of panels
  size_t panelCount() const { return panels_.size(); }

private:
  std::vector<std::unique_ptr<UIPanel>> panels_;
};

// Template implementations

template <typename T, typename... Args>
T *ViewportWindow::addPanel(Args &&...args) {
  static_assert(std::is_base_of_v<UIPanel, T>, "T must derive from UIPanel");

  auto panel = std::make_unique<T>(std::forward<Args>(args)...);
  T *ptr = panel.get();
  panels_.push_back(std::move(panel));
  return ptr;
}

template <typename T>
T *ViewportWindow::getPanel() {
  static_assert(std::is_base_of_v<UIPanel, T>, "T must derive from UIPanel");

  for (auto &panel : panels_) {
    if (T *p = dynamic_cast<T *>(panel.get())) {
      return p;
    }
  }
  return nullptr;
}

} // namespace w3d
