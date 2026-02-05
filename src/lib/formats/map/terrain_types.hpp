#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace map {

// World space conversion constants from WorldHeightMap
constexpr float MAP_XY_FACTOR = 10.0f;
constexpr float MAP_HEIGHT_SCALE = MAP_XY_FACTOR / 16.0f;

// Constants from WorldHeightMap.h
constexpr uint32_t NUM_SOURCE_TILES = 1024;
constexpr uint32_t NUM_BLEND_TILES = 16192;
constexpr uint32_t NUM_CLIFF_INFO = 32384;
constexpr uint32_t NUM_TEXTURE_CLASSES = 256;

// Heightmap data structure
struct HeightmapData {
  uint16_t width = 0;
  uint16_t height = 0;
  uint16_t borderSize = 0;
  std::vector<struct ICoord2D> boundaries;

  // Height values (0-255 range)
  std::vector<uint8_t> heights;

  // Total data size (width * height)
  uint32_t dataSize() const { return static_cast<uint32_t>(width * height); }

  // Check if heightmap is valid
  bool isValid() const { return !heights.empty() && heights.size() == dataSize(); }
};

// 2D integer coordinate (from legacy code)
struct ICoord2D {
  int32_t x = 0;
  int32_t y = 0;
};

// Single cell in the heightmap grid
struct TileIndex {
  uint16_t baseTile = 0;      // Primary texture tile index
  uint16_t blendTile = 0;     // Optional blend tile (0 = none)
  uint16_t extraBlendTile = 0; // Extra blend tile for 3-way blending (0 = none)
  uint16_t cliffInfo = 0;     // Cliff information index (0 = none)

  // Check if this tile has blending
  bool hasBlend() const { return blendTile != 0; }
  bool hasExtraBlend() const { return extraBlendTile != 0; }
  bool hasCliffInfo() const { return cliffInfo != 0; }
};

// Texture class information
struct TextureClass {
  int32_t globalTextureClass = -1;
  int32_t firstTile = 0;
  int32_t numTiles = 0;
  int32_t width = 0;
  bool isBlendEdgeTile = false;
  std::string name;
  ICoord2D positionInTexture;
};

// Cliff UV mapping information
struct CliffInfo {
  float u0 = 0.0f; // Upper left UV
  float v0 = 0.0f;
  float u1 = 0.0f; // Lower left UV
  float v1 = 0.0f;
  float u2 = 0.0f; // Lower right UV
  float v2 = 0.0f;
  float u3 = 0.0f; // Upper right UV
  float v3 = 0.0f;
  bool flip = false;      // Triangle flip flag
  bool mutant = false;    // Mutant mapping needed
  int16_t tileIndex = 0;  // Tile texture index
};

// Blend tile information
struct BlendTileInfo {
  int32_t blendNdx = 0;
  bool horiz = false;           // Horizontal blend
  bool vert = false;            // Vertical blend
  bool rightDiagonal = false;   // Right diagonal blend
  bool leftDiagonal = false;    // Left diagonal blend
  bool inverted = false;        // Inverted blend
  bool longDiagonal = false;    // Long diagonal blend
  int32_t customBlendEdgeClass = -1; // Custom blend edge class
};

// Complete terrain data
struct TerrainData {
  HeightmapData heightmap;
  std::vector<TileIndex> tiles;

  // Texture information
  std::vector<TextureClass> textureClasses;
  std::vector<TextureClass> edgeTextureClasses;

  // Blend information
  std::vector<BlendTileInfo> blendTiles;
  std::vector<CliffInfo> cliffInfoList;

  // Counts
  int32_t numBitmapTiles = 0;
  int32_t numBlendedTiles = 0;
  int32_t numCliffInfo = 0;

  // Cell state arrays (for flip and cliff states)
  std::vector<uint8_t> cellFlipState;
  std::vector<uint8_t> cellCliffState;
  int32_t flipStateWidth = 0;

  // Check if terrain data is valid
  bool isValid() const {
    return heightmap.isValid() &&
           tiles.size() == heightmap.dataSize();
  }
};

// Map object structure (for Phase 4)
struct MapObject {
  std::string name;
  std::string thingTemplate;  // Object type/template name

  // Position and orientation
  struct {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
  } position;

  float angle = 0.0f;

  // Object flags
  int32_t flags = 0;

  // Additional properties (key-value pairs)
  std::vector<std::pair<std::string, std::string>> properties;
};

// Complete map data (for Phase 4)
struct MapData {
  TerrainData terrain;
  std::vector<MapObject> objects;

  // World dictionary properties
  std::vector<std::pair<std::string, std::string>> worldDict;
};

} // namespace map
