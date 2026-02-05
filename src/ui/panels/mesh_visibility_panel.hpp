#pragma once

#include "../ui_panel.hpp"

#include <unordered_map>
#include <vector>

namespace w3d {

class HLodModel;

/// Panel for toggling visibility of individual meshes.
/// Displays meshes in a hierarchical tree based on bone attachment.
/// Parent bone checkboxes toggle all descendant meshes.
class MeshVisibilityPanel : public UIPanel {
public:
  const char *title() const override { return "Mesh Visibility"; }
  void draw(UIContext &ctx) override;

private:
  // Draw a bone node and its children recursively
  void drawBoneNode(UIContext &ctx, HLodModel *model, bool useSkinned, int boneIndex,
                    const std::unordered_map<int, std::vector<size_t>> &boneToMeshes,
                    const std::unordered_map<int, std::vector<size_t>> &boneChildren);

  // Draw checkbox for a single mesh
  void drawMeshCheckbox(UIContext &ctx, HLodModel *model, bool useSkinned, size_t meshIndex);

  // Fallback flat list when no skeleton is available
  void drawFlatList(UIContext &ctx, HLodModel *model, bool useSkinned);
};

} // namespace w3d
