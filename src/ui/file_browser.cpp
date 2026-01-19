#include "file_browser.hpp"

#include <algorithm>
#include <cstring>

#include <imgui.h>

namespace w3d {

FileBrowser::FileBrowser() {
  // Start at current working directory
  currentPath_ = std::filesystem::current_path();
  refreshDirectory();
}

void FileBrowser::draw(bool* open) {
  if (!ImGui::Begin("File Browser", open)) {
    ImGui::End();
    return;
  }

  // Path navigation bar
  std::strncpy(pathInputBuffer_, currentPath_.string().c_str(),
               sizeof(pathInputBuffer_) - 1);
  pathInputBuffer_[sizeof(pathInputBuffer_) - 1] = '\0';

  ImGui::SetNextItemWidth(-60);
  if (ImGui::InputText("##Path", pathInputBuffer_, sizeof(pathInputBuffer_),
                       ImGuiInputTextFlags_EnterReturnsTrue)) {
    std::filesystem::path newPath(pathInputBuffer_);
    if (std::filesystem::exists(newPath) &&
        std::filesystem::is_directory(newPath)) {
      navigateTo(newPath);
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Go")) {
    std::filesystem::path newPath(pathInputBuffer_);
    if (std::filesystem::exists(newPath) &&
        std::filesystem::is_directory(newPath)) {
      navigateTo(newPath);
    }
  }

  // Back button and refresh
  if (ImGui::Button("Up")) {
    navigateUp();
  }
  ImGui::SameLine();
  if (ImGui::Button("Refresh")) {
    refreshDirectory();
  }
  ImGui::SameLine();
  ImGui::Text("Filter: %s",
              filterExtension_.empty() ? "*" : filterExtension_.c_str());

  ImGui::Separator();

  // File list
  ImGui::BeginChild("FileList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()),
                    ImGuiChildFlags_Borders);

  for (size_t i = 0; i < entries_.size(); ++i) {
    const auto& entry = entries_[i];

    // Icon based on type
    const char* icon = entry.isDirectory ? "[D] " : "[F] ";

    std::string label = icon + entry.name;

    bool isSelected = (static_cast<int>(i) == selectedIndex_);
    if (ImGui::Selectable(label.c_str(), isSelected,
                          ImGuiSelectableFlags_AllowDoubleClick)) {
      selectedIndex_ = static_cast<int>(i);

      if (ImGui::IsMouseDoubleClicked(0)) {
        if (entry.isDirectory) {
          navigateTo(entry.path);
        } else if (fileSelectedCallback_) {
          fileSelectedCallback_(entry.path);
        }
      }
    }

    // Show file size for files
    if (!entry.isDirectory) {
      ImGui::SameLine(ImGui::GetWindowWidth() - 100);
      if (entry.size < 1024) {
        ImGui::Text("%llu B", static_cast<unsigned long long>(entry.size));
      } else if (entry.size < 1024 * 1024) {
        ImGui::Text("%.1f KB", entry.size / 1024.0);
      } else {
        ImGui::Text("%.1f MB", entry.size / (1024.0 * 1024.0));
      }
    }
  }

  ImGui::EndChild();

  // Open button
  if (ImGui::Button("Open") && selectedIndex_ >= 0 &&
      selectedIndex_ < static_cast<int>(entries_.size())) {
    const auto& entry = entries_[selectedIndex_];
    if (entry.isDirectory) {
      navigateTo(entry.path);
    } else if (fileSelectedCallback_) {
      fileSelectedCallback_(entry.path);
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Cancel") && open) {
    *open = false;
  }

  ImGui::End();
}

void FileBrowser::refreshDirectory() {
  entries_.clear();
  selectedIndex_ = -1;

  try {
    for (const auto& dirEntry :
         std::filesystem::directory_iterator(currentPath_)) {
      FileEntry entry;
      entry.name = dirEntry.path().filename().string();
      entry.path = dirEntry.path();
      entry.isDirectory = dirEntry.is_directory();
      entry.size = entry.isDirectory ? 0 : dirEntry.file_size();

      // Apply filter for files
      if (!entry.isDirectory && !filterExtension_.empty()) {
        std::string ext = dirEntry.path().extension().string();
        // Case-insensitive comparison
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        std::string filterLower = filterExtension_;
        std::transform(filterLower.begin(), filterLower.end(),
                       filterLower.begin(), ::tolower);
        if (ext != filterLower) {
          continue;
        }
      }

      entries_.push_back(entry);
    }

    // Sort: directories first, then alphabetically
    std::sort(entries_.begin(), entries_.end(),
              [](const FileEntry& a, const FileEntry& b) {
                if (a.isDirectory != b.isDirectory) {
                  return a.isDirectory > b.isDirectory;
                }
                return a.name < b.name;
              });

  } catch (const std::filesystem::filesystem_error&) {
    // Permission denied or other error
  }
}

void FileBrowser::navigateUp() {
  if (currentPath_.has_parent_path() &&
      currentPath_ != currentPath_.root_path()) {
    currentPath_ = currentPath_.parent_path();
    refreshDirectory();
  }
}

void FileBrowser::navigateTo(const std::filesystem::path& path) {
  if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
    currentPath_ = path;
    refreshDirectory();
  }
}

void FileBrowser::openAt(const std::filesystem::path& path) {
  if (std::filesystem::exists(path)) {
    if (std::filesystem::is_directory(path)) {
      currentPath_ = path;
    } else {
      currentPath_ = path.parent_path();
    }
    refreshDirectory();
  }
}

}  // namespace w3d
