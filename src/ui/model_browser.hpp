#pragma once

#include <functional>
#include <string>
#include <vector>

#include "ui_window.hpp"

namespace w3d {

struct UIContext;

/// Model browser window for selecting models from BIG archives.
/// Shows a searchable list of available models and allows loading them.
class ModelBrowser : public UIWindow {
public:
  using ModelSelectedCallback = std::function<void(const std::string &)>;

  ModelBrowser();

  // UIWindow interface
  void draw(UIContext &ctx) override;
  const char *name() const override { return "Model Browser"; }

  /// Set callback for when a model is selected
  void setModelSelectedCallback(ModelSelectedCallback callback) {
    modelSelectedCallback_ = std::move(callback);
  }

  /// Set the list of available models
  void setAvailableModels(const std::vector<std::string> &models) { availableModels_ = models; }

  /// Set the list of available textures
  void setAvailableTextures(const std::vector<std::string> &textures) {
    availableTextures_ = textures;
  }

  /// Get the currently selected search text
  const std::string &searchText() const { return searchText_; }

  /// Get the currently selected model index
  int selectedIndex() const { return selectedIndex_; }

  /// Check if BIG archive mode is available
  bool isBigArchiveMode() const { return bigArchiveMode_; }

  /// Enable/disable BIG archive mode
  void setBigArchiveMode(bool enabled) { bigArchiveMode_ = enabled; }

private:
  std::vector<std::string> availableModels_;
  std::vector<std::string> availableTextures_;
  std::string searchText_;
  ModelSelectedCallback modelSelectedCallback_;
  int selectedIndex_ = -1;
  bool bigArchiveMode_ = false;
  char searchBuffer_[256] = {};
};

} // namespace w3d
