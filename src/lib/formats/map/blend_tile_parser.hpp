#pragma once

#include <optional>
#include <string>

#include "data_chunk_reader.hpp"
#include "types.hpp"

namespace map {

constexpr uint16_t K_BLEND_TILE_VERSION_1 = 1;
constexpr uint16_t K_BLEND_TILE_VERSION_2 = 2;
constexpr uint16_t K_BLEND_TILE_VERSION_3 = 3;
constexpr uint16_t K_BLEND_TILE_VERSION_4 = 4;
constexpr uint16_t K_BLEND_TILE_VERSION_5 = 5;
constexpr uint16_t K_BLEND_TILE_VERSION_6 = 6;
constexpr uint16_t K_BLEND_TILE_VERSION_7 = 7;
constexpr uint16_t K_BLEND_TILE_VERSION_8 = 8;

class BlendTileParser {
public:
  static std::optional<BlendTileData> parse(DataChunkReader &reader, uint16_t version,
                                            int32_t heightMapWidth, int32_t heightMapHeight,
                                            std::string *outError = nullptr);

private:
  static bool readTileArrays(DataChunkReader &reader, BlendTileData &result, uint16_t version,
                             int32_t heightMapWidth, int32_t heightMapHeight,
                             std::string *outError);
  static bool readTextureClasses(DataChunkReader &reader, BlendTileData &result,
                                 std::string *outError);
  static bool readEdgeTextureClasses(DataChunkReader &reader, BlendTileData &result,
                                     std::string *outError);
  static bool readBlendTileInfos(DataChunkReader &reader, BlendTileData &result, uint16_t version,
                                 std::string *outError);
  static bool readCliffInfos(DataChunkReader &reader, BlendTileData &result, std::string *outError);
};

} // namespace map
