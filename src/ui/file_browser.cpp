#include "file_browser.hpp"

#include <algorithm>
#include <cstring>

#include <imgui.h>

namespace w3d {

FileBrowser::FileBrowser() {
  // Start at current working directory
  currentPath_ = std::filesystem::current_path();
  refreshDirectory();

  // Start hidden by default
  visible_ = false;
}

void FileBrowser::draw(UIContext & /*ctx*/) {
  if (!ImGui::Begin(name(), visiblePtr())) {
    ImGui::End();
    return;
  }

  // Path navigation bar
  strncpy_s(pathInputBuffer_, sizeof(pathInputBuffer_), currentPath_.string().c_str(),
            sizeof(pathInputBuffer_) - 1);

  ImGui::SetNextItemWidth(-60);
  if (ImGui::InputText("##Path", pathInputBuffer_, sizeof(pathInputBuffer_),
                       ImGuiInputTextFlags_EnterReturnsTrue)) {
    std::filesystem::path newPath(pathInputBuffer_);
    if (std::filesystem::exists(newPath) && std::filesystem::is_directory(newPath)) {
      navigateTo(newPath);
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Go")) {
    std::filesystem::path newPath(pathInputBuffer_);
    if (std::filesystem::exists(newPath) && std::filesystem::is_directory(newPath)) {
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

  // Show filter info only in File mode
  if (browseMode_ == BrowseMode::File) {
    ImGui::SameLine();
    ImGui::Text("Filter: %s", filterExtension_.empty() ? "*" : filterExtension_.c_str());
  } else {
    ImGui::SameLine();
    ImGui::Text("(Selecting folder)");
  }

  ImGui::Separator();

  // File list
  ImGui::BeginChild("FileList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()),
                    ImGuiChildFlags_Borders);

  for (size_t i = 0; i < entries_.size(); ++i) {
    const auto &entry = entries_[i];

    // Icon based on type
    const char *icon = entry.isDirectory ? "[D] " : "[F] ";

    std::string label = icon + entry.name;

    bool isSelected = (static_cast<int>(i) == selectedIndex_);
    if (ImGui::Selectable(label.c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick)) {
      selectedIndex_ = static_cast<int>(i);

      if (ImGui::IsMouseDoubleClicked(0)) {
        handleSelection(entry);
      }
    }

    // Show file size for files (only in File mode)
    if (!entry.isDirectory && browseMode_ == BrowseMode::File) {
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

  // Action buttons based on mode
  if (browseMode_ == BrowseMode::Directory) {
    // Directory mode: "Select This Folder" button
    if (ImGui::Button("Select This Folder")) {
      selectCurrentDirectory();
    }
    ImGui::SameLine();
    // Also allow selecting a highlighted directory
    bool canOpenSelected =
        selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(entries_.size());
    if (canOpenSelected) {
      const auto &entry = entries_[selectedIndex_];
      if (entry.isDirectory) {
        if (ImGui::Button("Open Selected")) {
          navigateTo(entry.path);
        }
        ImGui::SameLine();
      }
    }
  } else {
    // File mode: "Open" button
    if (ImGui::Button("Open") && selectedIndex_ >= 0 &&
        selectedIndex_ < static_cast<int>(entries_.size())) {
      const auto &entry = entries_[selectedIndex_];
      handleSelection(entry);
    }
    ImGui::SameLine();
  }

  if (ImGui::Button("Cancel")) {
    setVisible(false);
  }

  ImGui::End();
}

void FileBrowser::handleSelection(const FileEntry &entry) {
  if (entry.isDirectory) {
    // Always navigate into directories on double-click
    navigateTo(entry.path);
  } else if (browseMode_ == BrowseMode::File && pathSelectedCallback_) {
    // Select file only in File mode
    pathSelectedCallback_(entry.path);
  }
}

void FileBrowser::refreshDirectory() {
  entries_.clear();
  selectedIndex_ = -1;

  if (!std::filesystem::exists(currentPath_)) {
    return;
  }

  try {
    for (const auto &dirEntry : std::filesystem::directory_iterator(currentPath_)) {
      FileEntry entry;
      entry.name = dirEntry.path().filename().string();
      entry.path = dirEntry.path();
      entry.isDirectory = dirEntry.is_directory();

      // Get file size safely
      if (!entry.isDirectory) {
        try {
          entry.size = dirEntry.file_size();
        } catch (const std::filesystem::filesystem_error &) {
          entry.size = 0;
        }
      } else {
        entry.size = 0;
      }

      // In Directory mode, only show directories
      if (browseMode_ == BrowseMode::Directory && !entry.isDirectory) {
        continue;
      }

      // Apply filter for files (only in File mode)
      if (browseMode_ == BrowseMode::File && !entry.isDirectory && !filterExtension_.empty()) {
        std::string ext = dirEntry.path().extension().string();
        // Case-insensitive comparison
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        std::string filterLower = filterExtension_;
        std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), ::tolower);
        if (ext != filterLower) {
          continue;
        }
      }

      entries_.push_back(entry);
    }

    // Sort: directories first, then alphabetically
    std::sort(entries_.begin(), entries_.end(), [](const FileEntry &a, const FileEntry &b) {
      if (a.isDirectory != b.isDirectory) {
        return a.isDirectory > b.isDirectory;
      }
      return a.name < b.name;
    });

  } catch (const std::filesystem::filesystem_error &) {
    // Permission denied or other error
  }
}

void FileBrowser::navigateUp() {
  if (currentPath_.has_parent_path() && currentPath_ != currentPath_.root_path()) {
    currentPath_ = currentPath_.parent_path();
    refreshDirectory();
  }
}

void FileBrowser::navigateTo(const std::filesystem::path &path) {
  if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
    currentPath_ = path;
    refreshDirectory();
  }
}

void FileBrowser::openAt(const std::filesystem::path &path) {
  if (std::filesystem::exists(path)) {
    if (std::filesystem::is_directory(path)) {
      currentPath_ = path;
    } else {
      currentPath_ = path.parent_path();
    }
    refreshDirectory();
  }
}

void FileBrowser::selectCurrentDirectory() {
  if (pathSelectedCallback_) {
    pathSelectedCallback_(currentPath_);
  }
}

void FileBrowser::selectEntry(int index) {
  if (index >= 0 && index < static_cast<int>(entries_.size())) {
    selectedIndex_ = index;
  }
}

} // namespace w3d
