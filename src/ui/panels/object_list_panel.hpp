#pragma once

#include <string>

#include "../ui_panel.hpp"

namespace w3d {

class ObjectListPanel : public UIPanel {
public:
  const char *title() const override { return "Objects"; }
  void draw(UIContext &ctx) override;

private:
  char searchBuffer_[256] = {};
  std::string searchText_;
  int selectedIndex_ = -1;
};

} // namespace w3d
