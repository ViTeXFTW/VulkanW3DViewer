#pragma once

#include <filesystem>
#include <optional>
#include <stdexcept>

#include "terrain_types.hpp"

namespace map {

// Forward declaration
class MapChunkReader;

// Exception for terrain loading errors
class TerrainLoadError : public std::runtime_error {
public:
  explicit TerrainLoadError(const std::string &msg) : std::runtime_error(msg) {}
};

// Terrain loader for Command & Conquer: Generals .map files
class TerrainLoader {
public:
  // Load terrain from .map file
  // Returns std::nullopt if the file could not be loaded
  std::optional<TerrainData> loadTerrain(const std::filesystem::path &mapPath);

  // Load heightmap only (minimal implementation for quick testing)
  // Returns std::nullopt if the file could not be loaded
  std::optional<HeightmapData> loadHeightmap(const std::filesystem::path &mapPath);

  // Load complete map data (terrain + objects)
  // Returns std::nullopt if the file could not be loaded
  std::optional<MapData> loadMap(const std::filesystem::path &mapPath);

private:
  // Parse HeightMapData chunk
  bool parseHeightMapData(MapChunkReader &reader, uint32_t version, TerrainData &terrain);

  // Parse BlendTileData chunk
  bool parseBlendTileData(MapChunkReader &reader, uint32_t version, TerrainData &terrain);

  // Parse WorldDict chunk
  bool parseWorldDict(MapChunkReader &reader, uint32_t version, MapData &map);

  // Parse ObjectsList chunk
  bool parseObjectsList(MapChunkReader &reader, uint32_t version, MapData &map);

  // Parse single Object chunk
  bool parseObject(MapChunkReader &reader, uint32_t version, MapData &map);
};

} // namespace map
