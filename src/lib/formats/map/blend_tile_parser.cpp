#include "blend_tile_parser.hpp"

#include <cstring>

namespace map {

std::optional<BlendTileData> BlendTileParser::parse(DataChunkReader &reader, uint16_t version,
                                                    int32_t heightMapWidth, int32_t heightMapHeight,
                                                    std::string *outError) {
  if (version < K_BLEND_TILE_VERSION_1 || version > K_BLEND_TILE_VERSION_8) {
    if (outError) {
      *outError = "Unsupported BlendTileData version: " + std::to_string(version);
    }
    return std::nullopt;
  }

  BlendTileData result;

  if (!readTileArrays(reader, result, version, heightMapWidth, heightMapHeight, outError)) {
    return std::nullopt;
  }

  if (!readTextureClasses(reader, result, outError)) {
    return std::nullopt;
  }

  if (version >= K_BLEND_TILE_VERSION_4) {
    if (!readEdgeTextureClasses(reader, result, outError)) {
      return std::nullopt;
    }
  }

  if (!readBlendTileInfos(reader, result, version, outError)) {
    return std::nullopt;
  }

  if (version >= K_BLEND_TILE_VERSION_5) {
    if (!readCliffInfos(reader, result, outError)) {
      return std::nullopt;
    }
  }

  if (!result.isValid()) {
    if (outError) {
      *outError = "Invalid BlendTileData: validation failed";
    }
    return std::nullopt;
  }

  return result;
}

bool BlendTileParser::readTileArrays(DataChunkReader &reader, BlendTileData &result,
                                     uint16_t version, int32_t heightMapWidth,
                                     int32_t heightMapHeight, std::string *outError) {
  auto dataSize = reader.readInt(outError);
  if (!dataSize) {
    return false;
  }
  result.dataSize = *dataSize;

  if (result.dataSize <= 0) {
    if (outError) {
      *outError = "BlendTileData dataSize must be positive";
    }
    return false;
  }

  size_t arrayByteSize = static_cast<size_t>(result.dataSize) * sizeof(int16_t);

  result.tileNdxes.resize(result.dataSize);
  if (!reader.readBytes(reinterpret_cast<uint8_t *>(result.tileNdxes.data()), arrayByteSize,
                        outError)) {
    return false;
  }

  result.blendTileNdxes.resize(result.dataSize);
  if (!reader.readBytes(reinterpret_cast<uint8_t *>(result.blendTileNdxes.data()), arrayByteSize,
                        outError)) {
    return false;
  }

  if (version >= K_BLEND_TILE_VERSION_6) {
    result.extraBlendTileNdxes.resize(result.dataSize);
    if (!reader.readBytes(reinterpret_cast<uint8_t *>(result.extraBlendTileNdxes.data()),
                          arrayByteSize, outError)) {
      return false;
    }
  }

  if (version >= K_BLEND_TILE_VERSION_5) {
    result.cliffInfoNdxes.resize(result.dataSize);
    if (!reader.readBytes(reinterpret_cast<uint8_t *>(result.cliffInfoNdxes.data()), arrayByteSize,
                          outError)) {
      return false;
    }
  }

  if (version >= K_BLEND_TILE_VERSION_7) {
    int32_t flipStateWidth;
    if (version == K_BLEND_TILE_VERSION_7) {
      flipStateWidth = (heightMapWidth + 1) / 8;
    } else {
      flipStateWidth = (heightMapWidth + 7) / 8;
    }
    size_t cliffStateSize = static_cast<size_t>(heightMapHeight) * flipStateWidth;
    result.cellCliffState.resize(cliffStateSize);
    if (!reader.readBytes(result.cellCliffState.data(), cliffStateSize, outError)) {
      return false;
    }
  }

  auto numBitmapTiles = reader.readInt(outError);
  if (!numBitmapTiles) {
    return false;
  }
  result.numBitmapTiles = *numBitmapTiles;

  auto numBlendedTiles = reader.readInt(outError);
  if (!numBlendedTiles) {
    return false;
  }
  result.numBlendedTiles = *numBlendedTiles;

  if (version >= K_BLEND_TILE_VERSION_5) {
    auto numCliffInfo = reader.readInt(outError);
    if (!numCliffInfo) {
      return false;
    }
    result.numCliffInfo = *numCliffInfo;
  }

  return true;
}

bool BlendTileParser::readTextureClasses(DataChunkReader &reader, BlendTileData &result,
                                         std::string *outError) {
  auto numTextureClasses = reader.readInt(outError);
  if (!numTextureClasses) {
    return false;
  }

  if (*numTextureClasses < 0) {
    if (outError) {
      *outError = "Negative texture class count";
    }
    return false;
  }

  result.textureClasses.reserve(*numTextureClasses);
  for (int32_t i = 0; i < *numTextureClasses; ++i) {
    TextureClass tc;

    auto firstTile = reader.readInt(outError);
    if (!firstTile) {
      return false;
    }
    tc.firstTile = *firstTile;

    auto numTiles = reader.readInt(outError);
    if (!numTiles) {
      return false;
    }
    tc.numTiles = *numTiles;

    auto width = reader.readInt(outError);
    if (!width) {
      return false;
    }
    tc.width = *width;

    auto legacy = reader.readInt(outError);
    if (!legacy) {
      return false;
    }

    auto name = reader.readAsciiString(outError);
    if (!name) {
      return false;
    }
    tc.name = std::move(*name);

    result.textureClasses.push_back(std::move(tc));
  }

  return true;
}

bool BlendTileParser::readEdgeTextureClasses(DataChunkReader &reader, BlendTileData &result,
                                             std::string *outError) {
  auto numEdgeTiles = reader.readInt(outError);
  if (!numEdgeTiles) {
    return false;
  }
  result.numEdgeTiles = *numEdgeTiles;

  auto numEdgeTextureClasses = reader.readInt(outError);
  if (!numEdgeTextureClasses) {
    return false;
  }

  if (*numEdgeTextureClasses < 0) {
    if (outError) {
      *outError = "Negative edge texture class count";
    }
    return false;
  }

  result.edgeTextureClasses.reserve(*numEdgeTextureClasses);
  for (int32_t i = 0; i < *numEdgeTextureClasses; ++i) {
    TextureClass tc;

    auto firstTile = reader.readInt(outError);
    if (!firstTile) {
      return false;
    }
    tc.firstTile = *firstTile;

    auto numTiles = reader.readInt(outError);
    if (!numTiles) {
      return false;
    }
    tc.numTiles = *numTiles;

    auto width = reader.readInt(outError);
    if (!width) {
      return false;
    }
    tc.width = *width;

    auto name = reader.readAsciiString(outError);
    if (!name) {
      return false;
    }
    tc.name = std::move(*name);

    result.edgeTextureClasses.push_back(std::move(tc));
  }

  return true;
}

bool BlendTileParser::readBlendTileInfos(DataChunkReader &reader, BlendTileData &result,
                                         uint16_t version, std::string *outError) {
  if (result.numBlendedTiles <= 0) {
    return true;
  }

  result.blendTileInfos.reserve(result.numBlendedTiles - 1);
  for (int32_t i = 1; i < result.numBlendedTiles; ++i) {
    BlendTileInfo info;

    auto blendNdx = reader.readInt(outError);
    if (!blendNdx) {
      return false;
    }
    info.blendNdx = *blendNdx;

    auto horiz = reader.readByte(outError);
    if (!horiz) {
      return false;
    }
    info.horiz = *horiz;

    auto vert = reader.readByte(outError);
    if (!vert) {
      return false;
    }
    info.vert = *vert;

    auto rightDiagonal = reader.readByte(outError);
    if (!rightDiagonal) {
      return false;
    }
    info.rightDiagonal = *rightDiagonal;

    auto leftDiagonal = reader.readByte(outError);
    if (!leftDiagonal) {
      return false;
    }
    info.leftDiagonal = *leftDiagonal;

    auto inverted = reader.readByte(outError);
    if (!inverted) {
      return false;
    }
    info.inverted = *inverted;

    if (version >= K_BLEND_TILE_VERSION_3) {
      auto longDiagonal = reader.readByte(outError);
      if (!longDiagonal) {
        return false;
      }
      info.longDiagonal = *longDiagonal;
    }

    if (version >= K_BLEND_TILE_VERSION_4) {
      auto customBlendEdgeClass = reader.readInt(outError);
      if (!customBlendEdgeClass) {
        return false;
      }
      info.customBlendEdgeClass = *customBlendEdgeClass;
    }

    auto flag = reader.readInt(outError);
    if (!flag) {
      return false;
    }
    if (*flag != FLAG_VAL) {
      if (outError) {
        *outError = "Invalid blend tile sentinel (expected 0x7ADA0000)";
      }
      return false;
    }

    result.blendTileInfos.push_back(info);
  }

  return true;
}

bool BlendTileParser::readCliffInfos(DataChunkReader &reader, BlendTileData &result,
                                     std::string *outError) {
  if (result.numCliffInfo <= 0) {
    return true;
  }

  result.cliffInfos.reserve(result.numCliffInfo - 1);
  for (int32_t i = 1; i < result.numCliffInfo; ++i) {
    CliffInfo ci;

    auto tileIndex = reader.readInt(outError);
    if (!tileIndex) {
      return false;
    }
    ci.tileIndex = *tileIndex;

    auto u0 = reader.readReal(outError);
    if (!u0) {
      return false;
    }
    ci.u0 = *u0;

    auto v0 = reader.readReal(outError);
    if (!v0) {
      return false;
    }
    ci.v0 = *v0;

    auto u1 = reader.readReal(outError);
    if (!u1) {
      return false;
    }
    ci.u1 = *u1;

    auto v1 = reader.readReal(outError);
    if (!v1) {
      return false;
    }
    ci.v1 = *v1;

    auto u2 = reader.readReal(outError);
    if (!u2) {
      return false;
    }
    ci.u2 = *u2;

    auto v2 = reader.readReal(outError);
    if (!v2) {
      return false;
    }
    ci.v2 = *v2;

    auto u3 = reader.readReal(outError);
    if (!u3) {
      return false;
    }
    ci.u3 = *u3;

    auto v3 = reader.readReal(outError);
    if (!v3) {
      return false;
    }
    ci.v3 = *v3;

    auto flip = reader.readByte(outError);
    if (!flip) {
      return false;
    }
    ci.flip = *flip;

    auto mutant = reader.readByte(outError);
    if (!mutant) {
      return false;
    }
    ci.mutant = *mutant;

    result.cliffInfos.push_back(ci);
  }

  return true;
}

} // namespace map
