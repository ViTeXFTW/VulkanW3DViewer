#include "object_list_panel.hpp"

#include <algorithm>

#include "../ui_context.hpp"
#include "lib/formats/map/types.hpp"

#include <imgui.h>

namespace w3d {

void ObjectListPanel::draw(UIContext &ctx) {
  if (!ctx.hasMap()) {
    ImGui::TextDisabled("No map loaded");
    return;
  }

  const auto &objects = ctx.loadedMap->objects;
  if (objects.empty()) {
    ImGui::TextDisabled("No objects in map");
    return;
  }

  ImGui::Text("%zu objects", objects.size());

  ImGui::Text("Search:");
  ImGui::SetNextItemWidth(-1);
  if (ImGui::InputText("##ObjSearch", searchBuffer_, sizeof(searchBuffer_))) {
    searchText_ = searchBuffer_;
    selectedIndex_ = -1;
  }

  ImGui::Separator();

  float listHeight = ImGui::GetContentRegionAvail().y;
  if (listHeight < 100.0f)
    listHeight = 200.0f;

  ImGui::BeginChild("ObjectList", ImVec2(0, listHeight), ImGuiChildFlags_Borders);

  int displayedIndex = 0;
  for (size_t i = 0; i < objects.size(); ++i) {
    const auto &obj = objects[i];

    if (!obj.shouldRender()) {
      continue;
    }

    // Extract just the template name (after last /)
    std::string displayName = obj.templateName;
    size_t lastSlash = displayName.find_last_of('/');
    if (lastSlash != std::string::npos) {
      displayName = displayName.substr(lastSlash + 1);
    }

    if (!searchText_.empty()) {
      std::string searchLower = searchText_;
      std::string targetLower = obj.templateName;
      std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);
      std::transform(targetLower.begin(), targetLower.end(), targetLower.begin(), ::tolower);
      if (targetLower.find(searchLower) == std::string::npos) {
        continue;
      }
    }

    char label[256];
    snprintf(label, sizeof(label), "%s##%zu", displayName.c_str(), i);

    bool isSelected = (displayedIndex == selectedIndex_);
    if (ImGui::Selectable(label, isSelected)) {
      selectedIndex_ = displayedIndex;
    }

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      ImGui::Text("Template: %s", obj.templateName.c_str());
      ImGui::Text("Position: (%.1f, %.1f, %.1f)", obj.position.x, obj.position.y, obj.position.z);
      ImGui::Text("Angle: %.1f deg", glm::degrees(obj.angle));
      ImGui::EndTooltip();
    }

    displayedIndex++;
  }

  if (displayedIndex == 0) {
    ImGui::TextDisabled("No matching objects");
  }

  ImGui::EndChild();
}

} // namespace w3d
