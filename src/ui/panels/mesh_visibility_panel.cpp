#include <unordered_map>
#include <vector>

#include "../ui_context.hpp"
#include "mesh_visibility_panel.hpp"
#include "render/hlod_model.hpp"
#include "render/skeleton.hpp"

#include <imgui.h>


namespace w3d {

namespace {

// Build map of bone index -> mesh indices (only for meshes visible at current LOD)
template <typename MeshT>
std::unordered_map<int, std::vector<size_t>> buildBoneToMeshMap(const std::vector<MeshT> &meshes,
                                                                size_t currentLOD) {
  std::unordered_map<int, std::vector<size_t>> boneToMeshes;

  for (size_t i = 0; i < meshes.size(); ++i) {
    const auto &mesh = meshes[i];

    // Only include meshes visible at current LOD
    if (!mesh.isAggregate && mesh.lodLevel != currentLOD) {
      continue;
    }

    int boneIdx = -1;
    if constexpr (std::is_same_v<MeshT, HLodMeshGPU>) {
      boneIdx = mesh.boneIndex;
    } else {
      boneIdx = mesh.fallbackBoneIndex;
    }

    boneToMeshes[boneIdx].push_back(i);
  }

  return boneToMeshes;
}

// Build map of bone index -> child bone indices
std::unordered_map<int, std::vector<size_t>> buildBoneChildMap(const SkeletonPose *pose) {
  std::unordered_map<int, std::vector<size_t>> children;

  if (!pose) {
    return children;
  }

  for (size_t i = 0; i < pose->boneCount(); ++i) {
    int parent = pose->parentIndex(i);
    children[parent].push_back(i);
  }

  return children;
}

// Check if a bone or any descendant has meshes
bool boneHasMeshes(int boneIndex, const std::unordered_map<int, std::vector<size_t>> &boneToMeshes,
                   const std::unordered_map<int, std::vector<size_t>> &boneChildren) {
  // Check this bone
  if (boneToMeshes.count(boneIndex) && !boneToMeshes.at(boneIndex).empty()) {
    return true;
  }

  // Check children recursively
  auto childIt = boneChildren.find(boneIndex);
  if (childIt != boneChildren.end()) {
    for (size_t childIdx : childIt->second) {
      if (boneHasMeshes(static_cast<int>(childIdx), boneToMeshes, boneChildren)) {
        return true;
      }
    }
  }

  return false;
}

// Collect all mesh indices under a bone (including descendants)
void collectMeshIndices(int boneIndex,
                        const std::unordered_map<int, std::vector<size_t>> &boneToMeshes,
                        const std::unordered_map<int, std::vector<size_t>> &boneChildren,
                        std::vector<size_t> &outIndices) {
  // Add meshes directly attached to this bone
  auto meshIt = boneToMeshes.find(boneIndex);
  if (meshIt != boneToMeshes.end()) {
    for (size_t idx : meshIt->second) {
      outIndices.push_back(idx);
    }
  }

  // Recurse into children
  auto childIt = boneChildren.find(boneIndex);
  if (childIt != boneChildren.end()) {
    for (size_t childIdx : childIt->second) {
      collectMeshIndices(static_cast<int>(childIdx), boneToMeshes, boneChildren, outIndices);
    }
  }
}

} // namespace

void MeshVisibilityPanel::draw(UIContext &ctx) {
  if (!ctx.hlodModel || !ctx.hlodModel->hasData()) {
    ImGui::TextDisabled("No model loaded");
    return;
  }

  auto *model = ctx.hlodModel;
  bool useSkinned = ctx.renderState && ctx.renderState->useSkinnedRendering && model->hasSkinning();

  // Show All / Hide All buttons
  if (ImGui::Button("Show All")) {
    if (useSkinned) {
      model->setAllSkinnedMeshesHidden(false);
    } else {
      model->setAllMeshesHidden(false);
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Hide All")) {
    if (useSkinned) {
      model->setAllSkinnedMeshesHidden(true);
    } else {
      model->setAllMeshesHidden(true);
    }
  }

  ImGui::Separator();

  // Show mesh count
  auto visibleIndices =
      useSkinned ? model->visibleSkinnedMeshIndices() : model->visibleMeshIndices();
  ImGui::Text("Meshes: %zu visible", visibleIndices.size());
  ImGui::Separator();

  // Check if we have skeleton for hierarchical view
  if (!ctx.skeletonPose || ctx.skeletonPose->boneCount() == 0) {
    // Fall back to flat list
    drawFlatList(ctx, model, useSkinned);
    return;
  }

  // Build hierarchy data structures
  auto boneToMeshes = useSkinned ? buildBoneToMeshMap(model->skinnedMeshes(), model->currentLOD())
                                 : buildBoneToMeshMap(model->meshes(), model->currentLOD());

  auto boneChildren = buildBoneChildMap(ctx.skeletonPose);

  // Scrollable tree view
  if (ImGui::BeginChild("MeshTree", ImVec2(0, 200), true)) {
    // Find root bones (parentIndex == -1) and draw them
    auto rootIt = boneChildren.find(-1);
    if (rootIt != boneChildren.end()) {
      for (size_t rootIdx : rootIt->second) {
        drawBoneNode(ctx, model, useSkinned, static_cast<int>(rootIdx), boneToMeshes, boneChildren);
      }
    }

    // Also draw meshes with no bone attachment (boneIndex == -1)
    auto unattachedIt = boneToMeshes.find(-1);
    if (unattachedIt != boneToMeshes.end() && !unattachedIt->second.empty()) {
      if (ImGui::TreeNode("Unattached")) {
        for (size_t meshIdx : unattachedIt->second) {
          drawMeshCheckbox(ctx, model, useSkinned, meshIdx);
        }
        ImGui::TreePop();
      }
    }
  }
  ImGui::EndChild();
}

void MeshVisibilityPanel::drawBoneNode(
    UIContext &ctx, HLodModel *model, bool useSkinned, int boneIndex,
    const std::unordered_map<int, std::vector<size_t>> &boneToMeshes,
    const std::unordered_map<int, std::vector<size_t>> &boneChildren) {

  // Skip bones with no meshes in subtree
  if (!boneHasMeshes(boneIndex, boneToMeshes, boneChildren)) {
    return;
  }

  const auto *pose = ctx.skeletonPose;
  std::string boneName = pose->boneName(static_cast<size_t>(boneIndex));

  // Get meshes directly on this bone
  auto meshIt = boneToMeshes.find(boneIndex);
  bool hasMeshes = meshIt != boneToMeshes.end() && !meshIt->second.empty();

  // Get children that have meshes
  std::vector<size_t> relevantChildren;
  auto childIt = boneChildren.find(boneIndex);
  if (childIt != boneChildren.end()) {
    for (size_t childIdx : childIt->second) {
      if (boneHasMeshes(static_cast<int>(childIdx), boneToMeshes, boneChildren)) {
        relevantChildren.push_back(childIdx);
      }
    }
  }

  // Determine if this node needs a tree structure
  bool hasChildren = !relevantChildren.empty();

  ImGui::PushID(boneIndex);

  if (hasMeshes || hasChildren) {
    // Calculate checkbox state for all meshes under this bone
    std::vector<size_t> allMeshIndices;
    collectMeshIndices(boneIndex, boneToMeshes, boneChildren, allMeshIndices);

    int visibleCount = 0;
    for (size_t idx : allMeshIndices) {
      bool hidden = useSkinned ? model->isSkinnedMeshHidden(idx) : model->isMeshHidden(idx);
      if (!hidden) {
        visibleCount++;
      }
    }

    bool allVisible = (visibleCount == static_cast<int>(allMeshIndices.size()));
    bool noneVisible = (visibleCount == 0);
    bool mixedState = !allVisible && !noneVisible;

    // Draw tree node with checkbox
    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (!hasChildren && hasMeshes && meshIt->second.size() == 1) {
      // Leaf bone with single mesh - no need for tree expansion
      flags |= ImGuiTreeNodeFlags_Leaf;
    }

    // Checkbox for toggling all meshes under this bone
    if (mixedState) {
      // Mixed state - use different visual
      ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
      ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
    }

    bool nodeVisible = !noneVisible;
    if (ImGui::Checkbox("##toggle", &nodeVisible)) {
      // Toggle all meshes under this bone
      for (size_t idx : allMeshIndices) {
        if (useSkinned) {
          model->setSkinnedMeshHidden(idx, !nodeVisible);
        } else {
          model->setMeshHidden(idx, !nodeVisible);
        }
      }
    }

    if (mixedState) {
      ImGui::PopStyleColor(2);
    }

    ImGui::SameLine();

    bool nodeOpen = ImGui::TreeNodeEx(boneName.c_str(), flags);

    if (nodeOpen) {
      // Draw meshes directly attached to this bone
      if (hasMeshes) {
        for (size_t meshIdx : meshIt->second) {
          drawMeshCheckbox(ctx, model, useSkinned, meshIdx);
        }
      }

      // Draw child bones
      for (size_t childIdx : relevantChildren) {
        drawBoneNode(ctx, model, useSkinned, static_cast<int>(childIdx), boneToMeshes,
                     boneChildren);
      }

      ImGui::TreePop();
    }
  }

  ImGui::PopID();
}

void MeshVisibilityPanel::drawMeshCheckbox([[maybe_unused]] UIContext &ctx, HLodModel *model,
                                           bool useSkinned, size_t meshIndex) {
  std::string meshName;
  bool isAggregate = false;

  if (useSkinned) {
    const auto &mesh = model->skinnedMeshes()[meshIndex];
    meshName = mesh.name;
    isAggregate = mesh.isAggregate;
  } else {
    const auto &mesh = model->meshes()[meshIndex];
    meshName = mesh.name;
    isAggregate = mesh.isAggregate;
  }

  // Add aggregate indicator
  if (isAggregate) {
    meshName += " [A]";
  }

  bool visible =
      useSkinned ? !model->isSkinnedMeshHidden(meshIndex) : !model->isMeshHidden(meshIndex);

  ImGui::PushID(static_cast<int>(meshIndex) + 10000); // Offset to avoid ID collision with bones
  if (ImGui::Checkbox(meshName.c_str(), &visible)) {
    if (useSkinned) {
      model->setSkinnedMeshHidden(meshIndex, !visible);
    } else {
      model->setMeshHidden(meshIndex, !visible);
    }
  }
  ImGui::PopID();
}

void MeshVisibilityPanel::drawFlatList(UIContext &ctx, HLodModel *model, bool useSkinned) {
  if (ImGui::BeginChild("MeshList", ImVec2(0, 200), true)) {
    if (useSkinned) {
      const auto &meshes = model->skinnedMeshes();
      for (size_t i = 0; i < meshes.size(); ++i) {
        const auto &mesh = meshes[i];
        if (!mesh.isAggregate && mesh.lodLevel != model->currentLOD()) {
          continue;
        }
        drawMeshCheckbox(ctx, model, useSkinned, i);
      }
    } else {
      const auto &meshes = model->meshes();
      for (size_t i = 0; i < meshes.size(); ++i) {
        const auto &mesh = meshes[i];
        if (!mesh.isAggregate && mesh.lodLevel != model->currentLOD()) {
          continue;
        }
        drawMeshCheckbox(ctx, model, useSkinned, i);
      }
    }
  }
  ImGui::EndChild();
}

} // namespace w3d
