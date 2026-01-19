#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace w3d {

class FileBrowser {
 public:
  using FileSelectedCallback = std::function<void(const std::filesystem::path&)>;

  FileBrowser();

  // Draw the file browser window
  void draw(bool* open = nullptr);

  // Set callback for when a file is selected
  void setFileSelectedCallback(FileSelectedCallback callback) {
    fileSelectedCallback_ = std::move(callback);
  }

  // Set the file extension filter (e.g., ".w3d")
  void setFilter(const std::string& extension) { filterExtension_ = extension; }

  // Open the browser at a specific path
  void openAt(const std::filesystem::path& path);

 private:
  void refreshDirectory();
  void navigateUp();
  void navigateTo(const std::filesystem::path& path);

  struct FileEntry {
    std::string name;
    std::filesystem::path path;
    bool isDirectory;
    uintmax_t size;
  };

  std::filesystem::path currentPath_;
  std::vector<FileEntry> entries_;
  std::string filterExtension_;
  FileSelectedCallback fileSelectedCallback_;
  int selectedIndex_ = -1;
  char pathInputBuffer_[512] = {};
};

}  // namespace w3d
