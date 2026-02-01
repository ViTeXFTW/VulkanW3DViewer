#pragma once

#include "../ui_panel.hpp"

namespace w3d {

/// Panel displaying information about the currently loaded model.
/// Shows file path, mesh counts, hierarchy info, and skeleton details.
class ModelInfoPanel : public UIPanel {
public:
  const char *title() const override { return "Model Info"; }
  void draw(UIContext &ctx) override;
};

} // namespace w3d
