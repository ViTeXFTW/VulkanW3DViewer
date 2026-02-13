#include "big_archive_manager.hpp"

#include <bigx/archive.hpp>
#include <fstream>
#include <iostream>
#include <system_error>

#include "core/app_paths.hpp"

namespace w3d::big {

namespace {
  // Known BIG archive files for C&C Generals
  constexpr const char *kBigArchives[] = {
      "W3DZH.big",
      "TexturesZH.big",
      "INIZH.big",
      "TerrainZH.big",
      "MapsZH.big"
  };
} // namespace

BigArchiveManager::~BigArchiveManager() {
  // Archives are RAII and close automatically
}

bool BigArchiveManager::ensureCacheDirectory(std::string *outError) {
  auto appDataDir = AppPaths::appDataDir();
  if (!appDataDir) {
    if (outError) {
      *outError = "Failed to get application data directory";
    }
    return false;
  }

  cacheDirectory_ = *appDataDir / "big_cache";

  // Create cache directory if it doesn't exist
  std::error_code ec;
  if (!std::filesystem::exists(cacheDirectory_, ec)) {
    if (!std::filesystem::create_directories(cacheDirectory_, ec)) {
      if (outError) {
        *outError = "Failed to create cache directory: " + ec.message();
      }
      return false;
    }
  }

  return true;
}

bool BigArchiveManager::loadArchives(std::string *outError) {
  size_t loadedCount = 0;

  for (const char *archiveName : kBigArchives) {
    std::filesystem::path archivePath = gameDirectory_ / archiveName;

    std::string error;
    auto archive = ::big::Archive::open(archivePath, &error);

    if (archive) {
      archives_[archiveName] = std::move(*archive);
      loadedCount++;
    }
  }

  if (loadedCount == 0) {
    if (outError) {
      *outError = "No BIG archives found in game directory";
    }
    return false;
  }

  return true;
}

bool BigArchiveManager::initialize(const std::filesystem::path &gameDirectory,
                                    std::string *outError) {
  gameDirectory_ = gameDirectory;

  // Verify game directory exists
  std::error_code ec;
  if (!std::filesystem::exists(gameDirectory_, ec)) {
    if (outError) {
      *outError = "Game directory does not exist: " + gameDirectory_.string();
    }
    return false;
  }

  // Set up cache directory
  if (!ensureCacheDirectory(outError)) {
    return false;
  }

  // Load archives
  if (!loadArchives(outError)) {
    return false;
  }

  initialized_ = true;
  return true;
}

const ::big::FileEntry *BigArchiveManager::findAsset(const std::string &archivePath) const {
  // Search in all loaded archives
  for (const auto &pair : archives_) {
    const auto *entry = pair.second.findFile(archivePath);
    if (entry) {
      return entry;
    }
  }
  return nullptr;
}

std::filesystem::path BigArchiveManager::getCachePath(const std::string &archivePath) const {
  // Normalize path (forward slashes) and prepend cache directory
  std::string normalizedPath = archivePath;

  // Replace backslashes with forward slashes
  for (char &c : normalizedPath) {
    if (c == '\\') {
      c = '/';
    }
  }

  return cacheDirectory_ / normalizedPath;
}

std::optional<std::filesystem::path> BigArchiveManager::extractToCache(
    const std::string &archivePath,
    std::string *outError) {

  if (!initialized_) {
    if (outError) {
      *outError = "BigArchiveManager not initialized";
    }
    return std::nullopt;
  }

  // Find asset in archives
  const auto *entry = findAsset(archivePath);
  if (!entry) {
    if (outError) {
      *outError = "Asset not found in archives: " + archivePath;
    }
    return std::nullopt;
  }

  // Determine cache file path
  std::filesystem::path cachePath = getCachePath(archivePath);

  // Check if already cached
  std::error_code ec;
  if (std::filesystem::exists(cachePath, ec)) {
    // Verify file size matches
    auto fileSize = std::filesystem::file_size(cachePath, ec);
    if (!ec && fileSize == entry->size) {
      return cachePath;
    }
  }

  // Create parent directories
  std::filesystem::path parentDir = cachePath.parent_path();
  if (!parentDir.empty() && !std::filesystem::exists(parentDir, ec)) {
    if (!std::filesystem::create_directories(parentDir, ec)) {
      if (outError) {
        *outError = "Failed to create cache directory: " + ec.message();
      }
      return std::nullopt;
    }
  }

  // Extract from archives (first one that has it)
  for (auto &pair : archives_) {
    std::string extractError;
    if (pair.second.extract(*entry, cachePath, &extractError)) {
      return cachePath;
    }
  }

  if (outError) {
    *outError = "Failed to extract asset: " + archivePath;
  }
  return std::nullopt;
}

std::optional<std::vector<uint8_t>> BigArchiveManager::extractToMemory(
    const std::string &archivePath,
    std::string *outError) {

  if (!initialized_) {
    if (outError) {
      *outError = "BigArchiveManager not initialized";
    }
    return std::nullopt;
  }

  // Find asset in archives
  const auto *entry = findAsset(archivePath);
  if (!entry) {
    if (outError) {
      *outError = "Asset not found in archives: " + archivePath;
    }
    return std::nullopt;
  }

  // Extract from archives (first one that has it)
  for (auto &pair : archives_) {
    std::string extractError;
    auto data = pair.second.extractToMemory(*entry, &extractError);
    if (data) {
      return data;
    }
  }

  if (outError) {
    *outError = "Failed to extract asset: " + archivePath;
  }
  return std::nullopt;
}

void BigArchiveManager::clearCache() {
  if (cacheDirectory_.empty()) {
    return;
  }

  std::error_code ec;
  if (std::filesystem::exists(cacheDirectory_, ec)) {
    std::filesystem::remove_all(cacheDirectory_, ec);
  }

  // Recreate empty cache directory
  ensureCacheDirectory(nullptr);
}

uintmax_t BigArchiveManager::getCacheSize() const {
  if (cacheDirectory_.empty()) {
    return 0;
  }

  return calculateDirectorySize(cacheDirectory_);
}

uintmax_t BigArchiveManager::calculateDirectorySize(const std::filesystem::path &path) {
  uintmax_t totalSize = 0;
  std::error_code ec;

  for (const auto &entry : std::filesystem::recursive_directory_iterator(path, ec)) {
    if (entry.is_regular_file(ec)) {
      totalSize += entry.file_size(ec);
    }
  }

  return totalSize;
}

std::vector<std::string> BigArchiveManager::loadedArchives() const {
  std::vector<std::string> names;
  names.reserve(archives_.size());

  for (const auto &pair : archives_) {
    names.push_back(pair.first);
  }

  return names;
}

} // namespace w3d::big
