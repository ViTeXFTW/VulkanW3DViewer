#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include "types.hpp"

namespace map {

class MapLoader {
public:
  static std::optional<MapFile> load(const std::filesystem::path &path,
                                     std::string *outError = nullptr);

  static std::optional<MapFile> loadFromMemory(const uint8_t *data, size_t size,
                                               std::string *outError = nullptr);

  static std::string describe(const MapFile &mapFile);
};

} // namespace map
