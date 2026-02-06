#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace w3d::big {

/// Extractor for INI files from Command & Conquer BIG archives.
/// This class handles extraction and listing of INI files for
/// future parsing of object metadata.
class IniExtractor {
public:
  IniExtractor() = default;
  ~IniExtractor() = default;

  // Non-copyable, moveable
  IniExtractor(const IniExtractor &) = delete;
  IniExtractor &operator=(const IniExtractor &) = delete;
  IniExtractor(IniExtractor &&) noexcept = default;
  IniExtractor &operator=(IniExtractor &&) noexcept = default;

  /// Extract a specific INI file to cache
  /// @param iniFileName Name of the INI file (with or without .ini extension)
  /// @param cacheDirectory Directory to extract to
  /// @param outError Optional error output parameter
  /// @return Extracted file path if successful
  std::optional<std::filesystem::path> extractIni(const std::string &iniFileName,
                                                    const std::filesystem::path &cacheDirectory,
                                                    std::string *outError = nullptr);

  /// List all INI files available in the archives
  /// @param gameDirectory Path to game directory containing BIG files
  /// @param outError Optional error output parameter
  /// @return Vector of INI file paths within archives
  static std::vector<std::string> listIniFiles(const std::filesystem::path &gameDirectory,
                                                std::string *outError = nullptr);

  /// Extract all INI files to cache
  /// @param gameDirectory Path to game directory containing BIG files
  /// @param cacheDirectory Directory to extract to
  /// @param outError Optional error output parameter
  /// @return Number of files extracted, or 0 on error
  static size_t extractAllIni(const std::filesystem::path &gameDirectory,
                              const std::filesystem::path &cacheDirectory,
                              std::string *outError = nullptr);
};

} // namespace w3d::big
