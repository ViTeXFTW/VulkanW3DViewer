#include "heightmap_parser.hpp"

namespace map {

std::optional<HeightMap> HeightMapParser::parse(DataChunkReader &reader, uint16_t version,
                                                std::string *outError) {
  HeightMap heightMap;

  bool success = false;
  switch (version) {
  case K_HEIGHT_MAP_VERSION_1:
    success = parseVersion1(reader, heightMap, outError);
    break;
  case K_HEIGHT_MAP_VERSION_2:
    success = parseVersion2(reader, heightMap, outError);
    break;
  case K_HEIGHT_MAP_VERSION_3:
    success = parseVersion3(reader, heightMap, outError);
    break;
  case K_HEIGHT_MAP_VERSION_4:
    success = parseVersion4(reader, heightMap, outError);
    break;
  default:
    if (outError) {
      *outError = "Unsupported HeightMapData version: " + std::to_string(version);
    }
    return std::nullopt;
  }

  if (!success) {
    return std::nullopt;
  }

  if (!heightMap.isValid()) {
    if (outError) {
      *outError = "Invalid heightmap: data size mismatch";
    }
    return std::nullopt;
  }

  return heightMap;
}

bool HeightMapParser::parseVersion1(DataChunkReader &reader, HeightMap &heightMap,
                                    std::string *outError) {
  auto width = reader.readInt(outError);
  if (!width)
    return false;
  auto height = reader.readInt(outError);
  if (!height)
    return false;

  heightMap.width = *width;
  heightMap.height = *height;
  heightMap.borderSize = 0;

  auto dataSize = reader.readInt(outError);
  if (!dataSize)
    return false;

  if (*dataSize != heightMap.width * heightMap.height) {
    if (outError) {
      *outError = "HeightMapData size mismatch";
    }
    return false;
  }

  heightMap.data.resize(*dataSize);
  if (!reader.readBytes(heightMap.data.data(), *dataSize, outError)) {
    return false;
  }

  std::vector<uint8_t> downsampledData;
  downsampledData.resize(heightMap.width * heightMap.height / 4);
  int32_t newWidth = heightMap.width / 2;
  int32_t newHeight = heightMap.height / 2;

  for (int32_t y = 0; y < newHeight; ++y) {
    for (int32_t x = 0; x < newWidth; ++x) {
      downsampledData[y * newWidth + x] = heightMap.data[(y * 2) * heightMap.width + (x * 2)];
    }
  }

  heightMap.width = newWidth;
  heightMap.height = newHeight;
  heightMap.data = std::move(downsampledData);
  heightMap.boundaries.push_back(glm::ivec2(heightMap.width, heightMap.height));

  return true;
}

bool HeightMapParser::parseVersion2(DataChunkReader &reader, HeightMap &heightMap,
                                    std::string *outError) {
  auto width = reader.readInt(outError);
  if (!width)
    return false;
  auto height = reader.readInt(outError);
  if (!height)
    return false;

  heightMap.width = *width;
  heightMap.height = *height;
  heightMap.borderSize = 0;

  auto dataSize = reader.readInt(outError);
  if (!dataSize)
    return false;

  if (*dataSize != heightMap.width * heightMap.height) {
    if (outError) {
      *outError = "HeightMapData size mismatch";
    }
    return false;
  }

  heightMap.data.resize(*dataSize);
  if (!reader.readBytes(heightMap.data.data(), *dataSize, outError)) {
    return false;
  }

  heightMap.boundaries.push_back(glm::ivec2(heightMap.width, heightMap.height));

  return true;
}

bool HeightMapParser::parseVersion3(DataChunkReader &reader, HeightMap &heightMap,
                                    std::string *outError) {
  auto width = reader.readInt(outError);
  if (!width)
    return false;
  auto height = reader.readInt(outError);
  if (!height)
    return false;

  heightMap.width = *width;
  heightMap.height = *height;

  auto borderSize = reader.readInt(outError);
  if (!borderSize)
    return false;
  heightMap.borderSize = *borderSize;

  auto dataSize = reader.readInt(outError);
  if (!dataSize)
    return false;

  if (*dataSize != heightMap.width * heightMap.height) {
    if (outError) {
      *outError = "HeightMapData size mismatch";
    }
    return false;
  }

  heightMap.data.resize(*dataSize);
  if (!reader.readBytes(heightMap.data.data(), *dataSize, outError)) {
    return false;
  }

  int32_t boundaryWidth = heightMap.width - 2 * heightMap.borderSize;
  int32_t boundaryHeight = heightMap.height - 2 * heightMap.borderSize;
  heightMap.boundaries.push_back(glm::ivec2(boundaryWidth, boundaryHeight));

  return true;
}

bool HeightMapParser::parseVersion4(DataChunkReader &reader, HeightMap &heightMap,
                                    std::string *outError) {
  auto width = reader.readInt(outError);
  if (!width)
    return false;
  auto height = reader.readInt(outError);
  if (!height)
    return false;

  heightMap.width = *width;
  heightMap.height = *height;

  auto borderSize = reader.readInt(outError);
  if (!borderSize)
    return false;
  heightMap.borderSize = *borderSize;

  auto numBoundaries = reader.readInt(outError);
  if (!numBoundaries)
    return false;

  heightMap.boundaries.reserve(*numBoundaries);
  for (int32_t i = 0; i < *numBoundaries; ++i) {
    auto x = reader.readInt(outError);
    if (!x)
      return false;
    auto y = reader.readInt(outError);
    if (!y)
      return false;
    heightMap.boundaries.push_back(glm::ivec2(*x, *y));
  }

  auto dataSize = reader.readInt(outError);
  if (!dataSize)
    return false;

  if (*dataSize != heightMap.width * heightMap.height) {
    if (outError) {
      *outError = "HeightMapData size mismatch";
    }
    return false;
  }

  heightMap.data.resize(*dataSize);
  if (!reader.readBytes(heightMap.data.data(), *dataSize, outError)) {
    return false;
  }

  return true;
}

} // namespace map
