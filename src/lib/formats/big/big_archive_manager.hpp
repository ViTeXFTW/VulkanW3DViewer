#pragma once

#include <big/archive.hpp>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace w3d::big {

/// RAII manager for extracting assets from BIG archives to cache.
/// Loads BIG archives and provides methods for extracting assets
/// to disk cache or memory buffers on-demand.
class BigArchiveManager {
public:
  BigArchiveManager() = default;
  ~BigArchiveManager();

  // Non-copyable, moveable
  BigArchiveManager(const BigArchiveManager &) = delete;
  BigArchiveManager &operator=(const BigArchiveManager &) = delete;
  BigArchiveManager(BigArchiveManager &&) noexcept = default;
  BigArchiveManager &operator=(BigArchiveManager &&) noexcept = default;

  /// Initialize with game directory
  /// @param gameDirectory Path to game directory containing BIG files
  /// @param outError Optional error output parameter
  /// @return true if initialization succeeded
  bool initialize(const std::filesystem::path &gameDirectory,
                  std::string *outError = nullptr);

  /// Extract asset to cache by archive path
  /// @param archivePath Path within the archive (e.g., "Art/W3D/model.w3d")
  /// @param outError Optional error output parameter
  /// @return Cached file path if extraction succeeded
  std::optional<std::filesystem::path> extractToCache(const std::string &archivePath,
                                                       std::string *outError = nullptr);

  /// Extract asset to memory buffer
  /// @param archivePath Path within the archive
  /// @param outError Optional error output parameter
  /// @return Vector of bytes if extraction succeeded
  std::optional<std::vector<uint8_t>> extractToMemory(const std::string &archivePath,
                                                       std::string *outError = nullptr);

  /// Check if initialized
  /// @return true if manager is initialized
  bool isInitialized() const { return initialized_; }

  /// Get game directory
  /// @return Path to game directory
  const std::filesystem::path &gameDirectory() const { return gameDirectory_; }

  /// Get cache directory
  /// @return Path to cache directory
  const std::filesystem::path &cacheDirectory() const { return cacheDirectory_; }

  /// Clear all cached files
  void clearCache();

  /// Get total cache size in bytes
  /// @return Cache directory size
  uintmax_t getCacheSize() const;

  /// Get list of loaded archive names
  /// @return Vector of archive filenames
  std::vector<std::string> loadedArchives() const;

private:
  bool initialized_ = false;
  std::filesystem::path gameDirectory_;
  std::filesystem::path cacheDirectory_;
  std::unordered_map<std::string, ::big::Archive> archives_;

  /// Find asset in all loaded archives
  /// @param archivePath Path to search for
  /// @return File entry if found, nullptr otherwise
  const ::big::FileEntry *findAsset(const std::string &archivePath) const;

  /// Get cache file path for an archive path
  /// @param archivePath Path within the archive
  /// @return Full path to cache file
  std::filesystem::path getCachePath(const std::string &archivePath) const;

  /// Ensure cache directory exists
  /// @param outError Optional error output parameter
  /// @return true if cache directory is ready
  bool ensureCacheDirectory(std::string *outError = nullptr);

  /// Load all BIG archives from game directory
  /// @param outError Optional error output parameter
  /// @return true if at least one archive was loaded
  bool loadArchives(std::string *outError = nullptr);

  /// Recursively calculate directory size
  /// @param path Directory path
  /// @return Total size in bytes
  static uintmax_t calculateDirectorySize(const std::filesystem::path &path);
};

} // namespace w3d::big
