#pragma once

#include <memory>
#include <vector>

#include "ui_panel.hpp"
#include "ui_window.hpp"

namespace w3d {

class MapViewportWindow : public UIWindow {
public:
  MapViewportWindow();

  void draw(UIContext &ctx) override;
  const char *name() const override { return "Map Viewer"; }

  template <typename T, typename... Args>
  T *addPanel(Args &&...args);

  template <typename T>
  T *getPanel();

  size_t panelCount() const { return panels_.size(); }

private:
  std::vector<std::unique_ptr<UIPanel>> panels_;
};

template <typename T, typename... Args>
T *MapViewportWindow::addPanel(Args &&...args) {
  static_assert(std::is_base_of_v<UIPanel, T>, "T must derive from UIPanel");
  auto panel = std::make_unique<T>(std::forward<Args>(args)...);
  T *ptr = panel.get();
  panels_.push_back(std::move(panel));
  return ptr;
}

template <typename T>
T *MapViewportWindow::getPanel() {
  static_assert(std::is_base_of_v<UIPanel, T>, "T must derive from UIPanel");
  for (auto &panel : panels_) {
    if (T *p = dynamic_cast<T *>(panel.get())) {
      return p;
    }
  }
  return nullptr;
}

} // namespace w3d
