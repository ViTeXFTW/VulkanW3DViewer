#include "model_browser.hpp"

#include <algorithm>
#include <cstring>
#include <string>

#include <imgui.h>

namespace w3d {

namespace {
// Extract just the filename from a path for display
// e.g., "art/w3d/vehicles/tank" -> "tank"
std::string getDisplayName(const std::string &fullPath) {
  size_t lastSlash = fullPath.find_last_of('/');
  if (lastSlash == std::string::npos) {
    lastSlash = fullPath.find_last_of('\\');
  }
  return (lastSlash != std::string::npos) ? fullPath.substr(lastSlash + 1) : fullPath;
}
} // namespace

ModelBrowser::ModelBrowser() {
  // Start hidden by default
  visible_ = false;
}

void ModelBrowser::draw(UIContext & /*ctx*/) {
  if (!ImGui::Begin(name(), visiblePtr())) {
    ImGui::End();
    return;
  }

  // Show mode indicator
  if (bigArchiveMode_) {
    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "[BIG Archive Mode]");
    ImGui::SameLine();
    ImGui::Text("%zu models available", availableModels_.size());
  } else {
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "[File Browser Mode]");
    ImGui::SameLine();
    ImGui::Text("Use File > Open for file browser");
  }

  ImGui::Separator();

  // Search box
  ImGui::Text("Search:");
  ImGui::SetNextItemWidth(-1);
  if (ImGui::InputText("##Search", searchBuffer_, sizeof(searchBuffer_))) {
    searchText_ = searchBuffer_;
    // Reset selection when search changes
    selectedIndex_ = -1;
  }

  // Filter hint
  if (searchText_.empty()) {
    ImGui::TextDisabled("Type to filter models...");
  } else {
    ImGui::TextDisabled("Filtering: %s", searchText_.c_str());
  }

  ImGui::Separator();

  // Model list
  ImGui::BeginChild("ModelList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 2),
                    ImGuiChildFlags_Borders);

  int displayedIndex = 0;
  for (const auto &model : availableModels_) {
    // Get display name (just filename) for cleaner UI
    std::string displayName = getDisplayName(model);
    std::string searchTarget = displayName + " " + model; // Search both name and full path

    // Apply search filter
    if (!searchText_.empty()) {
      std::string searchLower = searchText_;
      std::string targetLower = searchTarget;
      std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);
      std::transform(targetLower.begin(), targetLower.end(), targetLower.begin(), ::tolower);
      if (targetLower.find(searchLower) == std::string::npos) {
        continue;
      }
    }

    bool isSelected = (displayedIndex == selectedIndex_);
    if (ImGui::Selectable(displayName.c_str(), isSelected)) {
      selectedIndex_ = displayedIndex;
    }

    // Double-click to load
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
      if (modelSelectedCallback_) {
        modelSelectedCallback_(model); // Use full path for loading
      }
      visible_ = false;
    }

    displayedIndex++;
  }

  if (displayedIndex == 0) {
    ImGui::TextDisabled("No models found");
  }

  ImGui::EndChild();

  // Action buttons
  if (selectedIndex_ >= 0) {
    // Find the actual model name at the selected index (after filtering)
    int actualIndex = -1;
    int filteredCount = 0;
    for (const auto &model : availableModels_) {
      std::string displayName = getDisplayName(model);
      std::string searchTarget = displayName + " " + model;

      if (!searchText_.empty()) {
        std::string searchLower = searchText_;
        std::string targetLower = searchTarget;
        std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);
        std::transform(targetLower.begin(), targetLower.end(), targetLower.begin(), ::tolower);
        if (targetLower.find(searchLower) == std::string::npos) {
          continue;
        }
      }
      if (filteredCount == selectedIndex_) {
        actualIndex = static_cast<int>(&model - &availableModels_[0]);
        break;
      }
      filteredCount++;
    }

    if (actualIndex >= 0 && ImGui::Button("Load Selected")) {
      if (modelSelectedCallback_) {
        modelSelectedCallback_(availableModels_[actualIndex]);
      }
      visible_ = false;
    }
    ImGui::SameLine();
  }

  if (ImGui::Button("Cancel")) {
    visible_ = false;
  }

  ImGui::End();
}

} // namespace w3d
