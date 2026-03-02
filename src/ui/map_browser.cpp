#include "map_browser.hpp"

#include <algorithm>
#include <cstring>
#include <string>

#include <imgui.h>

namespace w3d {

std::string MapBrowser::getDisplayName(const std::string &fullPath) {
  std::string name = fullPath;
  // Map paths are like "maps/alpine assault/alpine assault"
  // Extract just the last component
  size_t lastSlash = name.find_last_of('/');
  if (lastSlash == std::string::npos) {
    lastSlash = name.find_last_of('\\');
  }
  if (lastSlash != std::string::npos) {
    name = name.substr(lastSlash + 1);
  }
  return name;
}

MapBrowser::MapBrowser() {
  visible_ = false;
}

void MapBrowser::draw(UIContext & /*ctx*/) {
  if (!ImGui::Begin(name(), visiblePtr())) {
    ImGui::End();
    return;
  }

  if (bigArchiveMode_) {
    ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "[BIG Archive Mode]");
    ImGui::SameLine();
    ImGui::Text("%zu maps available", availableMaps_.size());
  } else {
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "[File Browser Mode]");
    ImGui::SameLine();
    ImGui::Text("Use File > Open Map for file browser");
  }

  ImGui::Separator();

  ImGui::Text("Search:");
  ImGui::SetNextItemWidth(-1);
  if (ImGui::InputText("##MapSearch", searchBuffer_, sizeof(searchBuffer_))) {
    searchText_ = searchBuffer_;
    selectedIndex_ = -1;
  }

  if (searchText_.empty()) {
    ImGui::TextDisabled("Type to filter maps...");
  } else {
    ImGui::TextDisabled("Filtering: %s", searchText_.c_str());
  }

  ImGui::Separator();

  ImGui::BeginChild("MapList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 2),
                    ImGuiChildFlags_Borders);

  int displayedIndex = 0;
  for (const auto &mapPath : availableMaps_) {
    std::string displayName = getDisplayName(mapPath);
    std::string searchTarget = displayName + " " + mapPath;

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

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
      if (mapSelectedCallback_) {
        mapSelectedCallback_(mapPath);
      }
      visible_ = false;
    }

    displayedIndex++;
  }

  if (displayedIndex == 0) {
    ImGui::TextDisabled("No maps found");
  }

  ImGui::EndChild();

  if (selectedIndex_ >= 0) {
    const std::string *selectedMap = nullptr;
    int filteredCount = 0;
    for (const auto &mapPath : availableMaps_) {
      std::string displayName = getDisplayName(mapPath);
      std::string searchTarget = displayName + " " + mapPath;

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
        selectedMap = &mapPath;
        break;
      }
      filteredCount++;
    }

    if (selectedMap && ImGui::Button("Load Selected")) {
      if (mapSelectedCallback_) {
        mapSelectedCallback_(*selectedMap);
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
