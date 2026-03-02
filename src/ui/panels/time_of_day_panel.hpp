#pragma once

#include "../ui_panel.hpp"

namespace w3d {

class TimeOfDayPanel : public UIPanel {
public:
  const char *title() const override { return "Time of Day"; }
  void draw(UIContext &ctx) override;
};

} // namespace w3d
