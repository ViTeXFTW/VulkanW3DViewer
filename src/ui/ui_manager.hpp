#pragma once

#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "ui_context.hpp"
#include "ui_window.hpp"

namespace w3d {

/// Central manager for all UI windows.
/// Handles dockspace creation, menu bar, and window lifecycle.
///
/// Usage:
/// 1. Create UIManager
/// 2. Register windows via addWindow<T>() or addWindow(unique_ptr)
/// 3. Call draw(context) each frame
///
/// Example:
///   UIManager ui;
///   ui.addWindow<ConsoleWindow>();
///   ui.addWindow<ViewportWindow>();
///   // In render loop:
///   ui.draw(context);
class UIManager {
public:
  UIManager() = default;
  ~UIManager() = default;

  UIManager(const UIManager&) = delete;
  UIManager& operator=(const UIManager&) = delete;

  /// Add a window by constructing it in place
  /// Returns pointer to the created window for further configuration
  template <typename T, typename... Args>
  T* addWindow(Args&&... args);

  /// Add an already-constructed window
  /// Returns pointer to the added window
  UIWindow* addWindow(std::unique_ptr<UIWindow> window);

  /// Get a window by type (returns nullptr if not found)
  template <typename T>
  T* getWindow();

  /// Get a window by type (const version)
  template <typename T>
  const T* getWindow() const;

  /// Draw all UI: dockspace, menu bar, and windows
  /// @param ctx Shared context to pass to windows
  void draw(UIContext& ctx);

  /// Get number of registered windows
  size_t windowCount() const { return windows_.size(); }

  /// Enable or disable the demo window (debug builds only)
  void setShowDemoWindow(bool show) { showDemoWindow_ = show; }
  bool showDemoWindow() const { return showDemoWindow_; }

private:
  /// Draw the root dockspace that covers the entire viewport
  void drawDockspace();

  /// Draw the main menu bar
  void drawMenuBar(UIContext& ctx);

  std::vector<std::unique_ptr<UIWindow>> windows_;
  std::unordered_map<std::type_index, UIWindow*> windowsByType_;

  bool showDemoWindow_ = false;
};

// Template implementations

template <typename T, typename... Args>
T* UIManager::addWindow(Args&&... args) {
  static_assert(std::is_base_of_v<UIWindow, T>, "T must derive from UIWindow");

  auto window = std::make_unique<T>(std::forward<Args>(args)...);
  T* ptr = window.get();

  windowsByType_[std::type_index(typeid(T))] = ptr;
  windows_.push_back(std::move(window));

  return ptr;
}

template <typename T>
T* UIManager::getWindow() {
  static_assert(std::is_base_of_v<UIWindow, T>, "T must derive from UIWindow");

  auto it = windowsByType_.find(std::type_index(typeid(T)));
  if (it != windowsByType_.end()) {
    return static_cast<T*>(it->second);
  }
  return nullptr;
}

template <typename T>
const T* UIManager::getWindow() const {
  static_assert(std::is_base_of_v<UIWindow, T>, "T must derive from UIWindow");

  auto it = windowsByType_.find(std::type_index(typeid(T)));
  if (it != windowsByType_.end()) {
    return static_cast<const T*>(it->second);
  }
  return nullptr;
}

} // namespace w3d
