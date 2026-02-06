#pragma once

#include <cstdint>
#include <string_view>

namespace map {

// Map chunk version constants (from WorldHeightMap.cpp)
namespace MapChunkVersion {
// HeightMapData versions
constexpr uint32_t HEIGHT_MAP_VERSION_1 = 1;
constexpr uint32_t HEIGHT_MAP_VERSION_3 = 3;
constexpr uint32_t HEIGHT_MAP_VERSION_4 = 4;
constexpr uint32_t HEIGHT_MAP_CURRENT = HEIGHT_MAP_VERSION_4;

// BlendTileData versions
constexpr uint32_t BLEND_TILE_VERSION_1 = 1;
constexpr uint32_t BLEND_TILE_VERSION_3 = 3;
constexpr uint32_t BLEND_TILE_VERSION_4 = 4;
constexpr uint32_t BLEND_TILE_VERSION_5 = 5;
constexpr uint32_t BLEND_TILE_VERSION_6 = 6;
constexpr uint32_t BLEND_TILE_VERSION_7 = 7;
constexpr uint32_t BLEND_TILE_CURRENT = BLEND_TILE_VERSION_7;

// Objects versions
constexpr uint32_t OBJECTS_VERSION_2 = 2;
constexpr uint32_t OBJECTS_CURRENT = OBJECTS_VERSION_2;

// Lighting versions
constexpr uint32_t LIGHTING_VERSION_2 = 2;
constexpr uint32_t LIGHTING_VERSION_3 = 3;
constexpr uint32_t LIGHTING_CURRENT = LIGHTING_VERSION_3;
} // namespace MapChunkVersion

// Map chunk identifiers (text-based for map format)
// Unlike W3D which uses 4-byte codes, the map format uses string chunk names
struct MapChunkType {
  std::string_view name;

  constexpr bool operator==(const MapChunkType &other) const { return name == other.name; }

  constexpr bool operator==(std::string_view str) const { return name == str; }
};

// Map chunk type constants
namespace MapChunks {
constexpr MapChunkType HEIGHT_MAP_DATA{"HeightMapData"};
constexpr MapChunkType BLEND_TILE_DATA{"BlendTileData"};
constexpr MapChunkType WORLD_DICT{"WorldInfo"};
constexpr MapChunkType OBJECTS_LIST{"ObjectsList"};
constexpr MapChunkType OBJECT{"Object"};
constexpr MapChunkType GLOBAL_LIGHTING{"GlobalLighting"};
constexpr MapChunkType POLYGON_TRIGGERS{"PolygonTriggers"};
constexpr MapChunkType SIDES_LIST{"SidesList"};
} // namespace MapChunks

// Chunk header for map files (text-based chunk format)
struct MapChunkHeader {
  std::string name;
  uint32_t version = 0;
  uint32_t size = 0; // Size of chunk data (not including header)

  bool isContainer() const {
    // Map chunks are always containers (have sub-chunks)
    return true;
  }
};

// Helper to get chunk name for debugging
inline const char *ChunkTypeName(const MapChunkType &type) {
  return type.name.data();
}

} // namespace map
