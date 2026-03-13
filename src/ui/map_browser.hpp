#pragma once

#include <functional>
#include <string>
#include <vector>

#include "ui_window.hpp"

namespace w3d {

struct UIContext;

class MapBrowser : public UIWindow {
public:
  using MapSelectedCallback = std::function<void(const std::string &)>;

  MapBrowser();

  void draw(UIContext &ctx) override;
  const char *name() const override { return "Map Browser"; }

  void setMapSelectedCallback(MapSelectedCallback callback) {
    mapSelectedCallback_ = std::move(callback);
  }

  void setAvailableMaps(const std::vector<std::string> &maps) { availableMaps_ = maps; }

  const std::string &searchText() const { return searchText_; }
  int selectedIndex() const { return selectedIndex_; }
  bool isBigArchiveMode() const { return bigArchiveMode_; }
  void setBigArchiveMode(bool enabled) { bigArchiveMode_ = enabled; }

private:
  static std::string getDisplayName(const std::string &fullPath);

  std::vector<std::string> availableMaps_;
  std::string searchText_;
  MapSelectedCallback mapSelectedCallback_;
  int selectedIndex_ = -1;
  bool bigArchiveMode_ = false;
  char searchBuffer_[256] = {};
};

} // namespace w3d
