#include "map_loader.hpp"

#include <fstream>
#include <sstream>

#include "blend_tile_parser.hpp"
#include "data_chunk_reader.hpp"
#include "heightmap_parser.hpp"
#include "lighting_parser.hpp"
#include "objects_parser.hpp"
#include "sideslist_parser.hpp"
#include "triggers_parser.hpp"
#include "worldinfo_parser.hpp"

namespace map {

std::string MapFile::describe() const {
  return MapLoader::describe(*this);
}

std::optional<MapFile> MapLoader::load(const std::filesystem::path &path, std::string *outError) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file) {
    if (outError) {
      *outError = "Failed to open file: " + path.string();
    }
    return std::nullopt;
  }

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<uint8_t> buffer(static_cast<size_t>(size));
  if (!file.read(reinterpret_cast<char *>(buffer.data()), size)) {
    if (outError) {
      *outError = "Failed to read file: " + path.string();
    }
    return std::nullopt;
  }

  auto result = loadFromMemory(buffer.data(), buffer.size(), outError);
  if (result) {
    result->sourcePath = path.string();
  }
  return result;
}

std::optional<MapFile> MapLoader::loadFromMemory(const uint8_t *data, size_t size,
                                                 std::string *outError) {
  DataChunkReader reader;
  auto tocError = reader.loadFromMemory(std::span<const uint8_t>(data, size));
  if (tocError) {
    if (outError) {
      *outError = "Failed to parse TOC: " + *tocError;
    }
    return std::nullopt;
  }

  MapFile mapFile;
  bool hasHeightMap = false;

  while (!reader.atEnd()) {
    auto header = reader.openChunk(outError);
    if (!header) {
      return std::nullopt;
    }

    auto chunkName = reader.lookupName(header->id);
    if (!chunkName) {
      reader.closeChunk();
      continue;
    }

    std::string parseError;

    if (*chunkName == "HeightMapData") {
      auto heightMap = HeightMapParser::parse(reader, header->version, &parseError);
      if (!heightMap) {
        if (outError) {
          *outError = "Failed to parse HeightMapData: " + parseError;
        }
        return std::nullopt;
      }
      mapFile.heightMap = std::move(*heightMap);
      hasHeightMap = true;

    } else if (*chunkName == "BlendTileData") {
      if (!hasHeightMap) {
        if (outError) {
          *outError = "BlendTileData chunk found before HeightMapData";
        }
        return std::nullopt;
      }
      auto blendTiles = BlendTileParser::parse(reader, header->version, mapFile.heightMap.width,
                                               mapFile.heightMap.height, &parseError);
      if (!blendTiles) {
        if (outError) {
          *outError = "Failed to parse BlendTileData: " + parseError;
        }
        return std::nullopt;
      }
      mapFile.blendTiles = std::move(*blendTiles);

    } else if (*chunkName == "ObjectsList") {
      auto objects = ObjectsParser::parse(reader, header->version, &parseError);
      if (!objects) {
        if (outError) {
          *outError = "Failed to parse ObjectsList: " + parseError;
        }
        return std::nullopt;
      }
      mapFile.objects = std::move(*objects);

    } else if (*chunkName == "PolygonTriggers") {
      auto triggers = TriggersParser::parse(reader, header->version, &parseError);
      if (!triggers) {
        if (outError) {
          *outError = "Failed to parse PolygonTriggers: " + parseError;
        }
        return std::nullopt;
      }
      mapFile.triggers = std::move(*triggers);

    } else if (*chunkName == "GlobalLighting") {
      auto lighting = LightingParser::parse(reader, header->version, &parseError);
      if (!lighting) {
        if (outError) {
          *outError = "Failed to parse GlobalLighting: " + parseError;
        }
        return std::nullopt;
      }
      mapFile.lighting = std::move(*lighting);

    } else if (*chunkName == "WorldInfo") {
      auto worldInfo = WorldInfoParser::parse(reader, header->version, &parseError);
      if (!worldInfo) {
        if (outError) {
          *outError = "Failed to parse WorldInfo: " + parseError;
        }
        return std::nullopt;
      }
      mapFile.worldInfo = std::move(*worldInfo);

    } else if (*chunkName == "SidesList") {
      auto sides = SidesListParser::parse(reader, header->version, &parseError);
      if (!sides) {
        if (outError) {
          *outError = "Failed to parse SidesList: " + parseError;
        }
        return std::nullopt;
      }
      mapFile.sides = std::move(*sides);
    }

    reader.closeChunk();
  }

  return mapFile;
}

std::string MapLoader::describe(const MapFile &mapFile) {
  std::ostringstream oss;

  oss << "Map File Contents:\n";
  oss << "==================\n\n";

  if (!mapFile.sourcePath.empty()) {
    oss << "Source: " << mapFile.sourcePath << "\n\n";
  }

  if (mapFile.hasHeightMap()) {
    const auto &hm = mapFile.heightMap;
    oss << "HeightMap:\n";
    oss << "  Dimensions: " << hm.width << " x " << hm.height << "\n";
    oss << "  World size: " << (hm.width * MAP_XY_FACTOR) << " x " << (hm.height * MAP_XY_FACTOR)
        << " units\n";
    oss << "  Border size: " << hm.borderSize << "\n";
    oss << "  Boundaries: " << hm.boundaries.size() << "\n";

    if (!hm.data.empty()) {
      uint8_t minH = 255;
      uint8_t maxH = 0;
      for (uint8_t h : hm.data) {
        if (h < minH)
          minH = h;
        if (h > maxH)
          maxH = h;
      }
      oss << "  Height range: " << static_cast<int>(minH) << " - " << static_cast<int>(maxH)
          << " (world: " << (minH * MAP_HEIGHT_SCALE) << " - " << (maxH * MAP_HEIGHT_SCALE)
          << ")\n";
    }
    oss << "\n";
  }

  if (mapFile.hasBlendTiles()) {
    const auto &bt = mapFile.blendTiles;
    oss << "BlendTileData:\n";
    oss << "  Data size: " << bt.dataSize << "\n";
    oss << "  Bitmap tiles: " << bt.numBitmapTiles << "\n";
    oss << "  Blended tiles: " << bt.numBlendedTiles << "\n";
    oss << "  Cliff info: " << bt.numCliffInfo << "\n";
    oss << "  Texture classes: " << bt.textureClasses.size() << "\n";

    if (!bt.textureClasses.empty()) {
      oss << "  Terrain types: ";
      for (size_t i = 0; i < bt.textureClasses.size(); ++i) {
        if (i > 0)
          oss << ", ";
        oss << bt.textureClasses[i].name;
      }
      oss << "\n";
    }

    if (!bt.edgeTextureClasses.empty()) {
      oss << "  Edge texture classes: " << bt.edgeTextureClasses.size() << "\n";
    }
    oss << "\n";
  }

  if (mapFile.hasObjects()) {
    oss << "Objects: " << mapFile.objects.size() << "\n";

    int32_t roadPoints = 0;
    int32_t bridgePoints = 0;
    int32_t renderable = 0;
    for (const auto &obj : mapFile.objects) {
      if (obj.isRoadPoint())
        ++roadPoints;
      if (obj.isBridgePoint())
        ++bridgePoints;
      if (obj.shouldRender())
        ++renderable;
    }
    oss << "  Renderable: " << renderable << "\n";
    oss << "  Road points: " << roadPoints << "\n";
    oss << "  Bridge points: " << bridgePoints << "\n";
    oss << "\n";
  }

  if (mapFile.hasTriggers()) {
    oss << "Polygon Triggers: " << mapFile.triggers.size() << "\n";

    int32_t waterAreas = 0;
    int32_t rivers = 0;
    for (const auto &trigger : mapFile.triggers) {
      if (trigger.isWaterArea)
        ++waterAreas;
      if (trigger.isRiver)
        ++rivers;
    }
    if (waterAreas > 0)
      oss << "  Water areas: " << waterAreas << "\n";
    if (rivers > 0)
      oss << "  Rivers: " << rivers << "\n";
    oss << "\n";
  }

  if (mapFile.hasLighting()) {
    const auto &lit = mapFile.lighting;
    oss << "Global Lighting:\n";
    oss << "  Time of day: " << static_cast<int>(lit.currentTimeOfDay) << "\n";
    if (lit.shadowColor != 0) {
      oss << "  Shadow color: 0x" << std::hex << lit.shadowColor << std::dec << "\n";
    }
    oss << "\n";
  }

  if (mapFile.worldInfo.isValid()) {
    oss << "World Info:\n";
    oss << "  Weather: " << static_cast<int>(mapFile.worldInfo.weather) << "\n";
    oss << "\n";
  }

  if (mapFile.sides.isValid()) {
    oss << "Sides: " << mapFile.sides.sides.size() << "\n";
    for (const auto &side : mapFile.sides.sides) {
      oss << "  - " << side.name;
      if (!side.buildList.empty()) {
        oss << " (" << side.buildList.size() << " build list entries)";
      }
      oss << "\n";
    }
    if (!mapFile.sides.teams.empty()) {
      oss << "Teams: " << mapFile.sides.teams.size() << "\n";
    }
    oss << "\n";
  }

  return oss.str();
}

} // namespace map
