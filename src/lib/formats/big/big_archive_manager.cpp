#include "big_archive_manager.hpp"

#include <iostream>
#include <system_error>

#include "core/app_paths.hpp"
#include "core/debug.hpp"

#include <bigx/archive.hpp>

namespace w3d::big {

namespace {
// Known BIG archive files for C&C Generals
constexpr const char *kBigArchives[] = {"W3DZH.big", "TexturesZH.big", "INIZH.big", "TerrainZH.big",
                                        "MapsZH.big"};
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

  LOG_DEBUG("[BigArchiveManager] Loading BIG archives from: " << gameDirectory_.string() << "\n");

  // First, load the well-known archives from the root directory
  for (const char *archiveName : kBigArchives) {
    std::filesystem::path archivePath = gameDirectory_ / archiveName;

    std::string error;
    auto archive = ::bigx::Archive::open(archivePath, &error);

    if (archive) {
      archives_[archiveName] = std::move(*archive);
      loadedCount++;
      [[maybe_unused]] size_t fileCount = archive->fileCount();
      LOG_DEBUG("[BigArchiveManager] Loaded: " << archiveName << " (" << fileCount << " files)\n");
    } else {
      LOG_DEBUG("[BigArchiveManager] Skipped: " << archiveName << " - " << error << "\n");
    }
  }

  // Then, recursively search for any additional .big files in subdirectories
  std::error_code ec;
  [[maybe_unused]] size_t additionalCount = 0;
  for (const auto &entry : std::filesystem::recursive_directory_iterator(gameDirectory_, ec)) {
    if (ec) {
      continue; // Skip directories we can't access
    }

    if (entry.is_regular_file(ec)) {
      std::filesystem::path path = entry.path();
      if (path.extension() == ".big") {
        // Get relative path from game directory for the key
        std::filesystem::path relativePath = std::filesystem::relative(path, gameDirectory_);
        std::string key = relativePath.string();

        // Convert backslashes to forward slashes for consistency
        for (char &c : key) {
          if (c == '\\') {
            c = '/';
          }
        }

        // Skip if already loaded (the well-known archives above)
        if (archives_.find(key) != archives_.end()) {
          continue;
        }

        std::string error;
        auto archive = ::bigx::Archive::open(path, &error);

        if (archive) {
          archives_[key] = std::move(*archive);
          loadedCount++;
          additionalCount++;
          [[maybe_unused]] size_t fileCount = archive->fileCount();
          LOG_DEBUG("[BigArchiveManager] Found additional BIG: " << key << " (" << fileCount
                                                                 << " files)\n");
        }
      }
    }
  }

  LOG_DEBUG("[BigArchiveManager] Total archives loaded: " << loadedCount << " (" << additionalCount
                                                          << " additional)\n");

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

const ::bigx::FileEntry *BigArchiveManager::findAsset(const std::string &archivePath) const {
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

std::optional<std::filesystem::path>
BigArchiveManager::extractToCache(const std::string &archivePath, std::string *outError) {

  if (!initialized_) {
    if (outError) {
      *outError = "BigArchiveManager not initialized";
    }
    return std::nullopt;
  }

  // Determine cache file path
  std::filesystem::path cachePath = getCachePath(archivePath);

  // Check if already cached and valid
  std::error_code ec;
  if (std::filesystem::exists(cachePath, ec)) {
    // File exists - need to verify it's still valid by finding the entry
    for (const auto &pair : archives_) {
      const auto *entry = pair.second.findFile(archivePath);
      if (entry) {
        auto fileSize = std::filesystem::file_size(cachePath, ec);
        if (!ec && fileSize == entry->size) {
          return cachePath;
        }
        break; // Found the entry but size mismatch, need to re-extract
      }
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

  // Find and extract from the specific archive that contains this file
  for (auto &pair : archives_) {
    const auto *entry = pair.second.findFile(archivePath);
    if (entry) {
      // Extract from THIS archive using the entry from THIS archive
      std::string extractError;
      if (pair.second.extract(*entry, cachePath, &extractError)) {
        return cachePath;
      } else {
        if (outError) {
          *outError = "Failed to extract: " + extractError;
        }
        return std::nullopt;
      }
    }
  }

  if (outError) {
    *outError = "Asset not found in archives: " + archivePath;
  }
  return std::nullopt;
}

std::optional<std::vector<uint8_t>>
BigArchiveManager::extractToMemory(const std::string &archivePath, std::string *outError) {

  if (!initialized_) {
    if (outError) {
      *outError = "BigArchiveManager not initialized";
    }
    return std::nullopt;
  }

  // Find and extract from the specific archive that contains this file
  for (auto &pair : archives_) {
    const auto *entry = pair.second.findFile(archivePath);
    if (entry) {
      // Extract from THIS archive using the entry from THIS archive
      std::string extractError;
      auto data = pair.second.extractToMemory(*entry, &extractError);
      if (data) {
        return data;
      } else {
        if (outError) {
          *outError = "Failed to extract: " + extractError;
        }
        return std::nullopt;
      }
    }
  }

  if (outError) {
    *outError = "Asset not found in archives: " + archivePath;
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
