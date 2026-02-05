#include "terrain_loader.hpp"

#include <fstream>
#include <iostream>

#include "map_chunk_reader.hpp"

namespace map {

// Helper to read entire file into memory
static std::optional<std::vector<uint8_t>> readFile(const std::filesystem::path &path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file) {
    return std::nullopt;
  }

  const auto fileSize = file.tellg();
  if (fileSize <= 0) {
    return std::nullopt;
  }

  std::vector<uint8_t> data(static_cast<size_t>(fileSize));
  file.seekg(0, std::ios::beg);
  if (!file.read(reinterpret_cast<char *>(data.data()), data.size())) {
    return std::nullopt;
  }

  return data;
}

std::optional<TerrainData> TerrainLoader::loadTerrain(const std::filesystem::path &mapPath) {
  auto fileData = readFile(mapPath);
  if (!fileData) {
    return std::nullopt;
  }

  TerrainData terrain;
  MapChunkReader reader(*fileData);

  // Map files are chunk-based, similar to W3D but with text chunk names
  // We need to parse the top-level chunks
  while (!reader.atEnd()) {
    auto headerOpt = reader.peekChunkHeader();
    if (!headerOpt) {
      break;
    }

    auto header = *headerOpt;
    std::string chunkName = header.name;

    // Read the full chunk (header + data)
    MapChunkReader chunkReader = reader.subReader(12 + header.size);

    if (chunkName == MapChunks::HEIGHT_MAP_DATA.name) {
      if (!parseHeightMapData(chunkReader, header.version, terrain)) {
        std::cerr << "Failed to parse HeightMapData chunk\n";
        return std::nullopt;
      }
    } else if (chunkName == MapChunks::BLEND_TILE_DATA.name) {
      if (!parseBlendTileData(chunkReader, header.version, terrain)) {
        std::cerr << "Failed to parse BlendTileData chunk\n";
        return std::nullopt;
      }
    } else {
      // Skip unknown chunks
      chunkReader.skip(header.size);
    }
  }

  if (!terrain.isValid()) {
    std::cerr << "Loaded terrain data is invalid\n";
    return std::nullopt;
  }

  return terrain;
}

std::optional<HeightmapData> TerrainLoader::loadHeightmap(const std::filesystem::path &mapPath) {
  auto terrainOpt = loadTerrain(mapPath);
  if (!terrainOpt) {
    return std::nullopt;
  }
  return terrainOpt->heightmap;
}

std::optional<MapData> TerrainLoader::loadMap(const std::filesystem::path &mapPath) {
  auto fileData = readFile(mapPath);
  if (!fileData) {
    return std::nullopt;
  }

  MapData map;
  MapChunkReader reader(*fileData);

  // Map files are chunk-based, similar to W3D but with text chunk names
  while (!reader.atEnd()) {
    auto headerOpt = reader.peekChunkHeader();
    if (!headerOpt) {
      break;
    }

    auto header = *headerOpt;
    std::string chunkName = header.name;

    // Read the full chunk (header + data)
    MapChunkReader chunkReader = reader.subReader(12 + header.size);

    if (chunkName == MapChunks::HEIGHT_MAP_DATA.name) {
      if (!parseHeightMapData(chunkReader, header.version, map.terrain)) {
        std::cerr << "Failed to parse HeightMapData chunk\n";
        return std::nullopt;
      }
    } else if (chunkName == MapChunks::BLEND_TILE_DATA.name) {
      if (!parseBlendTileData(chunkReader, header.version, map.terrain)) {
        std::cerr << "Failed to parse BlendTileData chunk\n";
        return std::nullopt;
      }
    } else if (chunkName == MapChunks::WORLD_DICT.name) {
      if (!parseWorldDict(chunkReader, header.version, map)) {
        std::cerr << "Failed to parse WorldDict chunk\n";
        return std::nullopt;
      }
    } else if (chunkName == MapChunks::OBJECTS_LIST.name) {
      if (!parseObjectsList(chunkReader, header.version, map)) {
        std::cerr << "Failed to parse ObjectsList chunk\n";
        return std::nullopt;
      }
    } else {
      // Skip unknown chunks
      chunkReader.skip(header.size);
    }
  }

  if (!map.terrain.isValid()) {
    std::cerr << "Loaded map data has invalid terrain\n";
    return std::nullopt;
  }

  return map;
}

bool TerrainLoader::parseHeightMapData(MapChunkReader &reader, uint32_t version,
                                       TerrainData &terrain) {
  auto &heightmap = terrain.heightmap;

  // Read width and height
  heightmap.width = reader.read<uint16_t>();
  heightmap.height = reader.read<uint16_t>();

  // Read border size (version >= 3)
  if (version >= MapChunkVersion::HEIGHT_MAP_VERSION_3) {
    heightmap.borderSize = reader.read<uint16_t>();
  } else {
    heightmap.borderSize = 0;
  }

  // Read boundaries (version >= 4)
  if (version >= MapChunkVersion::HEIGHT_MAP_VERSION_4) {
    int32_t numBorders = reader.read<int32_t>();
    heightmap.boundaries.resize(numBorders);
    for (int32_t i = 0; i < numBorders; ++i) {
      heightmap.boundaries[i].x = reader.read<int32_t>();
      heightmap.boundaries[i].y = reader.read<int32_t>();
    }
  } else {
    // Single boundary for older versions
    heightmap.boundaries.resize(1);
    heightmap.boundaries[0].x = heightmap.width - 2 * heightmap.borderSize;
    heightmap.boundaries[0].y = heightmap.height - 2 * heightmap.borderSize;
  }

  // Read data size
  uint32_t dataSize = reader.read<uint32_t>();

  // Validate data size
  if (dataSize != static_cast<uint32_t>(heightmap.width * heightmap.height)) {
    std::cerr << "Heightmap data size mismatch: expected "
              << (heightmap.width * heightmap.height) << " got " << dataSize << "\n";
    return false;
  }

  // Read height data
  heightmap.heights = reader.readByteArray(dataSize);

  // Handle version 1 resizing (old format had 2x resolution)
  if (version == MapChunkVersion::HEIGHT_MAP_VERSION_1) {
    int32_t newWidth = (heightmap.width + 1) / 2;
    int32_t newHeight = (heightmap.height + 1) / 2;

    std::vector<uint8_t> resizedData(newWidth * newHeight);
    for (int32_t j = 0; j < newHeight; ++j) {
      for (int32_t i = 0; i < newWidth; ++i) {
        resizedData[j * newWidth + i] = heightmap.heights[2 * j * heightmap.width + 2 * i];
      }
    }

    heightmap.width = newWidth;
    heightmap.height = newHeight;
    heightmap.heights = std::move(resizedData);
  }

  return true;
}

bool TerrainLoader::parseBlendTileData(MapChunkReader &reader, uint32_t version,
                                       TerrainData &terrain) {
  auto &heightmap = terrain.heightmap;

  // Verify length matches expected size
  int32_t length = reader.read<int32_t>();
  if (length != static_cast<int32_t>(heightmap.dataSize())) {
    std::cerr << "BlendTileData length mismatch: expected " << heightmap.dataSize()
              << " got " << length << "\n";
    return false;
  }

  // Allocate tile arrays
  terrain.tiles.resize(heightmap.dataSize());

  // Read tile indices
  auto tileNdxes = reader.readArray<int16_t>(heightmap.dataSize());
  auto blendTileNdxes = reader.readArray<int16_t>(heightmap.dataSize());

  // Copy to tile array
  for (size_t i = 0; i < heightmap.dataSize(); ++i) {
    terrain.tiles[i].baseTile = static_cast<uint16_t>(tileNdxes[i]);
    terrain.tiles[i].blendTile = static_cast<uint16_t>(blendTileNdxes[i]);
  }

  // Read extra blend tiles (version >= 6)
  if (version >= MapChunkVersion::BLEND_TILE_VERSION_6) {
    auto extraBlendTileNdxes = reader.readArray<int16_t>(heightmap.dataSize());
    for (size_t i = 0; i < heightmap.dataSize(); ++i) {
      terrain.tiles[i].extraBlendTile = static_cast<uint16_t>(extraBlendTileNdxes[i]);
    }
  }

  // Read cliff info indices (version >= 5)
  if (version >= MapChunkVersion::BLEND_TILE_VERSION_5) {
    auto cliffInfoNdxes = reader.readArray<int16_t>(heightmap.dataSize());
    for (size_t i = 0; i < heightmap.dataSize(); ++i) {
      terrain.tiles[i].cliffInfo = static_cast<uint16_t>(cliffInfoNdxes[i]);
    }
  }

  // Read cell cliff state (version >= 7)
  if (version >= MapChunkVersion::BLEND_TILE_VERSION_7) {
    int32_t byteWidth = (heightmap.width + 7) / 8;
    int32_t numBytes = heightmap.height * byteWidth;
    terrain.cellCliffState = reader.readByteArray(numBytes);
    terrain.flipStateWidth = byteWidth;
  } else {
    // Initialize cliff state from heights (will be done later)
    terrain.cellCliffState.clear();
    int32_t byteWidth = (heightmap.width + 7) / 8;
    terrain.flipStateWidth = byteWidth;
    terrain.cellCliffState.resize(heightmap.height * byteWidth, 0);
  }

  // Initialize cell flip state
  {
    int32_t byteWidth = (heightmap.width + 7) / 8;
    int32_t numBytes = heightmap.height * byteWidth;
    terrain.cellFlipState.resize(numBytes, 0);
  }

  // Read counts
  terrain.numBitmapTiles = reader.read<int32_t>();
  terrain.numBlendedTiles = reader.read<int32_t>();

  if (version >= MapChunkVersion::BLEND_TILE_VERSION_5) {
    terrain.numCliffInfo = reader.read<int32_t>();
  } else {
    terrain.numCliffInfo = 1; // cliffInfo[0] is the default info
  }

  // Read texture classes
  int32_t numTextureClasses = reader.read<int32_t>();
  terrain.textureClasses.resize(numTextureClasses);

  for (int32_t i = 0; i < numTextureClasses; ++i) {
    terrain.textureClasses[i].globalTextureClass = -1;
    terrain.textureClasses[i].firstTile = reader.read<int32_t>();
    terrain.textureClasses[i].numTiles = reader.read<int32_t>();
    terrain.textureClasses[i].width = reader.read<int32_t>();

    // Legacy field - read but ignore
    [[maybe_unused]] int32_t legacy = reader.read<int32_t>();

    // Read texture name (null-terminated string)
    terrain.textureClasses[i].name = reader.readNullString(256);

    // Note: In the legacy code, this is where readTexClass would be called
    // to load the actual texture data. For now, we skip this.
  }

  // Read edge texture classes (version >= 4)
  if (version >= MapChunkVersion::BLEND_TILE_VERSION_4) {
    [[maybe_unused]] int32_t numEdgeTiles = reader.read<int32_t>();
    int32_t numEdgeTextureClasses = reader.read<int32_t>();
    terrain.edgeTextureClasses.resize(numEdgeTextureClasses);

    for (int32_t i = 0; i < numEdgeTextureClasses; ++i) {
      terrain.edgeTextureClasses[i].globalTextureClass = -1;
      terrain.edgeTextureClasses[i].firstTile = reader.read<int32_t>();
      terrain.edgeTextureClasses[i].numTiles = reader.read<int32_t>();
      terrain.edgeTextureClasses[i].width = reader.read<int32_t>();
      terrain.edgeTextureClasses[i].name = reader.readNullString(256);

      // Note: Texture loading would happen here in the legacy code
    }
  }

  // Read blended tiles
  terrain.blendTiles.resize(terrain.numBlendedTiles);

  // blendTiles[0] is the implied transparent tile
  terrain.blendTiles[0].blendNdx = 0;

  for (int32_t i = 1; i < terrain.numBlendedTiles; ++i) {
    terrain.blendTiles[i].blendNdx = reader.read<int32_t>();
    terrain.blendTiles[i].horiz = reader.read<uint8_t>() != 0;
    terrain.blendTiles[i].vert = reader.read<uint8_t>() != 0;
    terrain.blendTiles[i].rightDiagonal = reader.read<uint8_t>() != 0;
    terrain.blendTiles[i].leftDiagonal = reader.read<uint8_t>() != 0;
    terrain.blendTiles[i].inverted = reader.read<uint8_t>() != 0;

    if (version >= MapChunkVersion::BLEND_TILE_VERSION_3) {
      terrain.blendTiles[i].longDiagonal = reader.read<uint8_t>() != 0;
    } else {
      terrain.blendTiles[i].longDiagonal = false;
    }

    if (version >= MapChunkVersion::BLEND_TILE_VERSION_4) {
      terrain.blendTiles[i].customBlendEdgeClass = reader.read<int32_t>();
    } else {
      terrain.blendTiles[i].customBlendEdgeClass = -1;
    }

    // Read and verify flag
    int32_t flag = reader.read<int32_t>();
    constexpr int32_t EXPECTED_FLAG = 0x7ADA0000;
    if (flag != EXPECTED_FLAG) {
      std::cerr << "Invalid blend tile flag at index " << i << ": expected 0x"
                << std::hex << EXPECTED_FLAG << " got 0x" << flag << std::dec << "\n";
      return false;
    }
  }

  // Read cliff info (version >= 5)
  if (version >= MapChunkVersion::BLEND_TILE_VERSION_5) {
    terrain.cliffInfoList.resize(terrain.numCliffInfo);

    // cliffInfo[0] is the default info
    terrain.cliffInfoList[0] = CliffInfo{};

    for (int32_t i = 1; i < terrain.numCliffInfo; ++i) {
      terrain.cliffInfoList[i].tileIndex = reader.read<int16_t>();
      terrain.cliffInfoList[i].u0 = reader.read<float>();
      terrain.cliffInfoList[i].v0 = reader.read<float>();
      terrain.cliffInfoList[i].u1 = reader.read<float>();
      terrain.cliffInfoList[i].v1 = reader.read<float>();
      terrain.cliffInfoList[i].u2 = reader.read<float>();
      terrain.cliffInfoList[i].v2 = reader.read<float>();
      terrain.cliffInfoList[i].u3 = reader.read<float>();
      terrain.cliffInfoList[i].v3 = reader.read<float>();
      terrain.cliffInfoList[i].flip = reader.read<uint8_t>() != 0;
      terrain.cliffInfoList[i].mutant = reader.read<uint8_t>() != 0;
    }
  } else {
    terrain.cliffInfoList.resize(1);
    terrain.cliffInfoList[0] = CliffInfo{};
  }

  // Handle version 1 resizing (old format had 2x resolution)
  if (version == MapChunkVersion::BLEND_TILE_VERSION_1) {
    int32_t newWidth = (heightmap.width + 1) / 2;
    int32_t newHeight = (heightmap.height + 1) / 2;
    int32_t newDataSize = newWidth * newHeight;

    std::vector<TileIndex> resizedTiles(newDataSize);

    for (int32_t j = 0; j < newHeight; ++j) {
      for (int32_t i = 0; i < newWidth; ++i) {
        resizedTiles[j * newWidth + i] = terrain.tiles[2 * j * heightmap.width + 2 * i];
        // Clear blend and cliff info for resized tiles
        resizedTiles[j * newWidth + i].blendTile = 0;
        resizedTiles[j * newWidth + i].extraBlendTile = 0;
        resizedTiles[j * newWidth + i].cliffInfo = 0;
      }
    }

    heightmap.width = newWidth;
    heightmap.height = newHeight;
    terrain.tiles = std::move(resizedTiles);
    terrain.numBlendedTiles = 1;
    terrain.numCliffInfo = 1;
  }

  return true;
}

bool TerrainLoader::parseWorldDict([[maybe_unused]] MapChunkReader &reader, [[maybe_unused]] uint32_t version,
                                   MapData &map) {
  // WorldDict contains global key-value pairs for the map
  // Format: Dict (length + key-value pairs)
  //
  // For a minimal implementation, we skip parsing the dict
  // In a full implementation, we would:
  // 1. Read dict length (uint16)
  // 2. For each pair, read keyAndType (int32) and value
  // 3. Store in map.worldDict

  // For now, just skip the content
  // The dict length would be: uint16_t dictLen = reader.read<uint16_t>();
  // Then we would iterate dictLen times reading pairs

  (void)map; // Unused for now

  return true;
}

bool TerrainLoader::parseObjectsList(MapChunkReader &reader, [[maybe_unused]] uint32_t version,
                                     MapData &map) {
  // ObjectsList contains sub-chunks, each representing one object
  // Each sub-chunk has name "Object" and contains:
  // - position (3 floats: x, y, z)
  // - angle (float)
  // - flags (int32)
  // - name (null-terminated string)
  // - properties dict (only if version >= 2)

  // Read sub-chunks until we reach the end of the ObjectsList chunk
  while (!reader.atEnd()) {
    auto headerOpt = reader.peekChunkHeader();
    if (!headerOpt) {
      break;
    }

    auto header = *headerOpt;
    std::string chunkName = header.name;

    // Read the full chunk (header + data)
    MapChunkReader chunkReader = reader.subReader(12 + header.size);

    if (chunkName == "Obj" || chunkName == "Obje" || chunkName == "Objec") {
      // Object chunk (4-char name, truncated from "Object")
      if (!parseObject(chunkReader, header.version, map)) {
        std::cerr << "Failed to parse Object chunk\n";
        return false;
      }
    } else {
      // Skip unknown sub-chunks
      chunkReader.skip(header.size);
    }
  }

  return true;
}

bool TerrainLoader::parseObject(MapChunkReader &reader, uint32_t version,
                                MapData &map) {
  // Object chunk format (from ParseObjectData in WorldHeightMap.cpp):
  // - x (float) - X position
  // - y (float) - Y position
  // - z (float) - Z position (set to 0 if version <= 2)
  // - angle (float) - rotation angle in degrees
  // - flags (int32) - object flags
  // - name (null-terminated string) - thing template name
  // - properties dict (only if version >= 2)

  MapObject obj;

  // Read position
  obj.position.x = reader.read<float>();
  obj.position.y = reader.read<float>();
  obj.position.z = reader.read<float>();

  // Version 2 and earlier have z = 0
  if (version <= MapChunkVersion::OBJECTS_VERSION_2) {
    obj.position.z = 0.0f;
  }

  // Read angle
  obj.angle = reader.read<float>();

  // Read flags
  obj.flags = reader.read<int32_t>();

  // Read name (thing template name)
  obj.thingTemplate = reader.readNullString(256);
  obj.name = obj.thingTemplate; // Default to template name

  // Read properties dict if version >= 2
  if (version >= MapChunkVersion::OBJECTS_VERSION_2) {
    // Read dict length
    uint16_t dictLen = reader.read<uint16_t>();

    // For a minimal implementation, we skip the dict values
    // In a full implementation, we would parse each key-value pair
    for (uint16_t i = 0; i < dictLen; ++i) {
      // Read keyAndType (int32)
      // Key is in high 24 bits, type is in low 8 bits
      int32_t keyAndType = reader.read<int32_t>();
      int32_t type = keyAndType & 0xFF;
      [[maybe_unused]] int32_t keyIndex = keyAndType >> 8;

      // Read value based on type
      switch (type) {
        case 0: // DICT_BOOL
          reader.read<uint8_t>();
          break;
        case 1: // DICT_INT
          reader.read<int32_t>();
          break;
        case 2: // DICT_REAL
          reader.read<float>();
          break;
        case 3: // DICT_ASCIISTRING
          reader.readNullString(256);
          break;
        case 4: // DICT_UNICODESTRING
          // Skip wide string (read until null wchar)
          while (reader.read<uint16_t>() != 0) {
            // Keep reading until we find null terminator
            if (reader.atEnd()) {
              break;
            }
          }
          break;
        default:
          // Unknown type, skip this dict entry
          break;
      }
    }
  }

  // Add object to map
  map.objects.push_back(std::move(obj));

  return true;
}

} // namespace map
