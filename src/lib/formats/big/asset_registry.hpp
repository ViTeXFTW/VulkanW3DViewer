#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace w3d::big {

/// Registry of all discoverable assets from BIG archives and custom paths.
/// Scans BIG archives to build a list of available models and textures,
/// and manages search paths for asset resolution.
class AssetRegistry {
public:
  AssetRegistry() = default;
  ~AssetRegistry() = default;

  // Non-copyable, moveable
  AssetRegistry(const AssetRegistry &) = delete;
  AssetRegistry &operator=(const AssetRegistry &) = delete;
  AssetRegistry(AssetRegistry &&) noexcept = default;
  AssetRegistry &operator=(AssetRegistry &&) noexcept = default;

  /// Scan BIG archives and build asset registry
  /// @param gameDirectory Path to game directory containing BIG files
  /// @param outError Optional error output parameter
  /// @return true if scanning succeeded, false on error
  bool scanArchives(const std::filesystem::path &gameDirectory, std::string *outError = nullptr);

  /// Add a custom search path
  /// @param path Directory path to add to search paths
  void addSearchPath(const std::filesystem::path &path);

  /// Remove a search path
  /// @param path Directory path to remove from search paths
  void removeSearchPath(const std::filesystem::path &path);

  /// Get all search paths (including auto-detected from BIG)
  /// @return Vector of search path directories
  const std::vector<std::filesystem::path> &searchPaths() const { return searchPaths_; }

  /// Get model names found in archives (for UI display)
  /// @return Vector of model asset names
  const std::vector<std::string> &availableModels() const { return availableModels_; }

  /// Get texture names found in archives (for UI display)
  /// @return Vector of texture asset names
  const std::vector<std::string> &availableTextures() const { return availableTextures_; }

  /// Get INI file names found in archives (for UI display)
  /// @return Vector of INI file names
  const std::vector<std::string> &availableIniFiles() const { return availableIniFiles_; }

  /// Clear the registry
  void clear();

  /// Check if registry has been scanned
  /// @return true if archives have been scanned
  bool isScanned() const { return scanned_; }

  /// Get the game directory used for scanning
  /// @return Path to game directory
  const std::filesystem::path &gameDirectory() const { return gameDirectory_; }

  /// Get cache directory for extracted assets
  /// @return Path to cache directory
  const std::filesystem::path &cacheDirectory() const { return cacheDirectory_; }

  /// Get archive path for a model asset
  /// @param modelName Name of the model (e.g., "avvehicle.tank")
  /// @return Archive path if found, empty otherwise
  std::string getModelArchivePath(const std::string &modelName) const;

  /// Get archive path for a texture asset
  /// @param textureName Name of the texture (e.g., "exptankburn")
  /// @return Archive path if found, empty otherwise
  std::string getTextureArchivePath(const std::string &textureName) const;

private:
  bool scanned_ = false;
  std::filesystem::path gameDirectory_;
  std::filesystem::path cacheDirectory_;
  std::vector<std::filesystem::path> searchPaths_;
  std::vector<std::string> availableModels_;
  std::vector<std::string> availableTextures_;
  std::vector<std::string> availableIniFiles_;
  std::unordered_map<std::string, std::string> modelArchivePaths_;     // name -> archive path
  std::unordered_map<std::string, std::string> textureArchivePaths_;   // name -> archive path
  std::unordered_map<std::string, std::string> textureBaseNameToPath_; // base name -> full path

  /// Scan a single archive file
  /// @param archivePath Path to the BIG archive file
  /// @param archiveName Name identifier for the archive
  /// @param outError Optional error output parameter
  /// @return true if scanning succeeded
  bool scanArchive(const std::filesystem::path &archivePath, const std::string &archiveName,
                   std::string *outError);

  /// Set up cache directory
  /// @param outError Optional error output parameter
  /// @return true if cache directory is ready
  bool setupCacheDirectory(std::string *outError);

  /// Normalize asset name (lowercase, remove extension)
  /// @param name Asset name to normalize
  /// @return Normalized name
  static std::string normalizeAssetName(const std::string &name);
};

} // namespace w3d::big
