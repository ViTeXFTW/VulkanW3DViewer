#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "ui_window.hpp"

namespace w3d {

/// Browse mode for the file browser
enum class BrowseMode {
  File,     ///< Select files (default behavior)
  Directory ///< Select directories/folders
};

/// Entry in the file browser listing
struct FileEntry {
  std::string name;
  std::filesystem::path path;
  bool isDirectory;
  uintmax_t size;
};

/// File browser window for selecting files or directories.
/// Supports directory navigation, file filtering, and double-click selection.
class FileBrowser : public UIWindow {
public:
  using PathSelectedCallback = std::function<void(const std::filesystem::path &)>;

  FileBrowser();

  // UIWindow interface
  void draw(UIContext &ctx) override;
  const char *name() const override { return title_.c_str(); }

  /// Set callback for when a path (file or directory) is selected
  void setPathSelectedCallback(PathSelectedCallback callback) {
    pathSelectedCallback_ = std::move(callback);
  }

  /// Set the file extension filter (e.g., ".w3d"). Only applies in File mode.
  void setFilter(const std::string &extension) { filterExtension_ = extension; }

  /// Set the browse mode (File or Directory selection)
  void setBrowseMode(BrowseMode mode) { browseMode_ = mode; }

  /// Get the current browse mode
  BrowseMode browseMode() const { return browseMode_; }

  /// Set the window title
  void setTitle(const std::string &title) { title_ = title; }

  /// Open the browser at a specific path
  void openAt(const std::filesystem::path &path);

  /// Get the current directory path
  const std::filesystem::path &currentPath() const { return currentPath_; }

  /// Get the list of entries in the current directory (for testing)
  const std::vector<FileEntry> &entries() const { return entries_; }

  /// Navigate to a specific directory
  void navigateTo(const std::filesystem::path &path);

  /// Navigate to parent directory
  void navigateUp();

  /// Refresh the current directory listing
  void refreshDirectory();

  /// Select the current directory (Directory mode only)
  void selectCurrentDirectory();

  /// Select a specific entry by index
  void selectEntry(int index);

  /// Get the currently selected index
  int selectedIndex() const { return selectedIndex_; }

private:
  void handleSelection(const FileEntry &entry);

  std::filesystem::path currentPath_;
  std::vector<FileEntry> entries_;
  std::string filterExtension_;
  std::string title_ = "File Browser";
  PathSelectedCallback pathSelectedCallback_;
  BrowseMode browseMode_ = BrowseMode::File;
  int selectedIndex_ = -1;
  char pathInputBuffer_[512] = {};
};

} // namespace w3d
