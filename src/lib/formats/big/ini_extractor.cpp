#include "ini_extractor.hpp"

#include <system_error>

#include <bigx/archive.hpp>

namespace w3d::big {

namespace {
// Known BIG archives that contain INI files
constexpr const char *kIniBigArchives[] = {
    "INIZH.big",
    "WindowsZH.big" // May contain additional INI files
};
} // namespace

std::optional<std::filesystem::path>
IniExtractor::extractIni(const std::string &iniFileName,
                         const std::filesystem::path &cacheDirectory, std::string *outError) {
  // This method is meant to be used with an already-open BigArchiveManager
  // For now, it's a placeholder for future functionality
  // The actual extraction would be done via BigArchiveManager
  (void)iniFileName;
  (void)cacheDirectory;

  if (outError) {
    *outError = "INI extraction should be done via BigArchiveManager";
  }
  return std::nullopt;
}

std::vector<std::string> IniExtractor::listIniFiles(const std::filesystem::path &gameDirectory,
                                                    std::string *outError) {
  std::vector<std::string> iniFiles;

  // Scan each known INI archive
  for (const char *archiveName : kIniBigArchives) {
    std::filesystem::path archivePath = gameDirectory / archiveName;

    std::string error;
    auto archive = ::bigx::Archive::open(archivePath, &error);

    if (!archive) {
      continue; // Skip archives that don't exist
    }

    // Collect all .ini files
    for (const auto &file : archive->files()) {
      if (file.path.length() > 4 && file.path.substr(file.path.length() - 4) == ".ini") {
        iniFiles.push_back(file.path);
      }
    }
  }

  if (iniFiles.empty() && outError) {
    *outError = "No INI files found in game directory";
  }

  return iniFiles;
}

size_t IniExtractor::extractAllIni(const std::filesystem::path &gameDirectory,
                                   const std::filesystem::path &cacheDirectory,
                                   std::string *outError) {
  size_t extractedCount = 0;

  // Create cache directory if it doesn't exist
  std::error_code ec;
  if (!std::filesystem::exists(cacheDirectory, ec)) {
    if (!std::filesystem::create_directories(cacheDirectory, ec)) {
      if (outError) {
        *outError = "Failed to create cache directory: " + ec.message();
      }
      return 0;
    }
  }

  // Scan each known INI archive
  for (const char *archiveName : kIniBigArchives) {
    std::filesystem::path archivePath = gameDirectory / archiveName;

    std::string error;
    auto archive = ::bigx::Archive::open(archivePath, &error);

    if (!archive) {
      continue; // Skip archives that don't exist
    }

    // Extract all .ini files
    for (const auto &file : archive->files()) {
      if (file.path.length() > 4 && file.path.substr(file.path.length() - 4) == ".ini") {
        // Determine cache file path
        std::filesystem::path cachePath = cacheDirectory / file.path;

        // Create parent directories
        std::filesystem::path parentDir = cachePath.parent_path();
        if (!parentDir.empty() && !std::filesystem::exists(parentDir, ec)) {
          std::filesystem::create_directories(parentDir, ec);
        }

        // Extract file
        std::string extractError;
        if (archive->extract(file, cachePath, &extractError)) {
          extractedCount++;
        }
      }
    }
  }

  if (extractedCount == 0 && outError) {
    *outError = "No INI files extracted";
  }

  return extractedCount;
}

} // namespace w3d::big
