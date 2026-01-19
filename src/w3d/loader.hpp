#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include "types.hpp"

namespace w3d {

// Main W3D file loader
class Loader {
 public:
  // Load a W3D file from disk
  // Returns std::nullopt on failure, with error message in outError if provided
  static std::optional<W3DFile> load(const std::filesystem::path& path,
                                     std::string* outError = nullptr);

  // Load W3D data from memory
  static std::optional<W3DFile> loadFromMemory(const uint8_t* data,
                                               size_t size,
                                               std::string* outError = nullptr);

  // Get a human-readable description of a W3D file
  static std::string describe(const W3DFile& file);
};

}  // namespace w3d
