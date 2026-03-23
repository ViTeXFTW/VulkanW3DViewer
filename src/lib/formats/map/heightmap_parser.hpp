#pragma once

#include <optional>
#include <string>

#include "data_chunk_reader.hpp"
#include "types.hpp"

namespace map {

constexpr uint16_t K_HEIGHT_MAP_VERSION_1 = 1;
constexpr uint16_t K_HEIGHT_MAP_VERSION_2 = 2;
constexpr uint16_t K_HEIGHT_MAP_VERSION_3 = 3;
constexpr uint16_t K_HEIGHT_MAP_VERSION_4 = 4;

class HeightMapParser {
public:
  static std::optional<HeightMap> parse(DataChunkReader &reader, uint16_t version,
                                        std::string *outError = nullptr);

private:
  static bool parseVersion1(DataChunkReader &reader, HeightMap &heightMap, std::string *outError);
  static bool parseVersion2(DataChunkReader &reader, HeightMap &heightMap, std::string *outError);
  static bool parseVersion3(DataChunkReader &reader, HeightMap &heightMap, std::string *outError);
  static bool parseVersion4(DataChunkReader &reader, HeightMap &heightMap, std::string *outError);
};

} // namespace map
