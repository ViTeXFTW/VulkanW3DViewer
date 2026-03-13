#include <cstring>

#include "../../src/lib/formats/map/blend_tile_parser.hpp"
#include "../../src/lib/formats/map/data_chunk_reader.hpp"
#include "../../src/lib/formats/map/types.hpp"

#include <gtest/gtest.h>

using namespace map;

class BlendTileParserTest : public ::testing::Test {
protected:
  std::vector<uint8_t> buildTOC(const std::vector<std::pair<std::string, uint32_t>> &entries) {
    std::vector<uint8_t> data;

    uint32_t magic = DATA_CHUNK_MAGIC;
    data.insert(data.end(), reinterpret_cast<const uint8_t *>(&magic),
                reinterpret_cast<const uint8_t *>(&magic) + 4);

    int32_t count = static_cast<int32_t>(entries.size());
    data.insert(data.end(), reinterpret_cast<const uint8_t *>(&count),
                reinterpret_cast<const uint8_t *>(&count) + 4);

    for (const auto &[name, id] : entries) {
      uint8_t nameLen = static_cast<uint8_t>(name.size());
      data.push_back(nameLen);
      data.insert(data.end(), name.begin(), name.end());
      data.insert(data.end(), reinterpret_cast<const uint8_t *>(&id),
                  reinterpret_cast<const uint8_t *>(&id) + 4);
    }

    return data;
  }

  void appendChunkHeader(std::vector<uint8_t> &data, uint32_t id, uint16_t version,
                         int32_t dataSize) {
    data.insert(data.end(), reinterpret_cast<const uint8_t *>(&id),
                reinterpret_cast<const uint8_t *>(&id) + 4);
    data.insert(data.end(), reinterpret_cast<const uint8_t *>(&version),
                reinterpret_cast<const uint8_t *>(&version) + 2);
    data.insert(data.end(), reinterpret_cast<const uint8_t *>(&dataSize),
                reinterpret_cast<const uint8_t *>(&dataSize) + 4);
  }

  void appendInt32(std::vector<uint8_t> &data, int32_t value) {
    data.insert(data.end(), reinterpret_cast<const uint8_t *>(&value),
                reinterpret_cast<const uint8_t *>(&value) + 4);
  }

  void appendFloat(std::vector<uint8_t> &data, float value) {
    data.insert(data.end(), reinterpret_cast<const uint8_t *>(&value),
                reinterpret_cast<const uint8_t *>(&value) + 4);
  }

  void appendInt16(std::vector<uint8_t> &data, int16_t value) {
    data.insert(data.end(), reinterpret_cast<const uint8_t *>(&value),
                reinterpret_cast<const uint8_t *>(&value) + 2);
  }

  void appendByte(std::vector<uint8_t> &data, int8_t value) {
    data.push_back(static_cast<uint8_t>(value));
  }

  void appendAsciiString(std::vector<uint8_t> &data, const std::string &str) {
    uint16_t len = static_cast<uint16_t>(str.size());
    data.insert(data.end(), reinterpret_cast<const uint8_t *>(&len),
                reinterpret_cast<const uint8_t *>(&len) + 2);
    data.insert(data.end(), str.begin(), str.end());
  }

  void appendInt16Array(std::vector<uint8_t> &data, int32_t count, int16_t fillValue = 0) {
    for (int32_t i = 0; i < count; ++i) {
      appendInt16(data, fillValue);
    }
  }

  void appendTextureClass(std::vector<uint8_t> &data, int32_t firstTile, int32_t numTiles,
                          int32_t width, const std::string &name) {
    appendInt32(data, firstTile);
    appendInt32(data, numTiles);
    appendInt32(data, width);
    appendInt32(data, 0);
    appendAsciiString(data, name);
  }

  void appendEdgeTextureClass(std::vector<uint8_t> &data, int32_t firstTile, int32_t numTiles,
                              int32_t width, const std::string &name) {
    appendInt32(data, firstTile);
    appendInt32(data, numTiles);
    appendInt32(data, width);
    appendAsciiString(data, name);
  }

  void appendBlendTileInfoV2(std::vector<uint8_t> &data, int32_t blendNdx, int8_t horiz,
                             int8_t vert, int8_t rightDiag, int8_t leftDiag, int8_t inverted) {
    appendInt32(data, blendNdx);
    appendByte(data, horiz);
    appendByte(data, vert);
    appendByte(data, rightDiag);
    appendByte(data, leftDiag);
    appendByte(data, inverted);
    appendInt32(data, FLAG_VAL);
  }

  void appendBlendTileInfoV3(std::vector<uint8_t> &data, int32_t blendNdx, int8_t horiz,
                             int8_t vert, int8_t rightDiag, int8_t leftDiag, int8_t inverted,
                             int8_t longDiag) {
    appendInt32(data, blendNdx);
    appendByte(data, horiz);
    appendByte(data, vert);
    appendByte(data, rightDiag);
    appendByte(data, leftDiag);
    appendByte(data, inverted);
    appendByte(data, longDiag);
    appendInt32(data, FLAG_VAL);
  }

  void appendBlendTileInfoV4(std::vector<uint8_t> &data, int32_t blendNdx, int8_t horiz,
                             int8_t vert, int8_t rightDiag, int8_t leftDiag, int8_t inverted,
                             int8_t longDiag, int32_t customEdge) {
    appendInt32(data, blendNdx);
    appendByte(data, horiz);
    appendByte(data, vert);
    appendByte(data, rightDiag);
    appendByte(data, leftDiag);
    appendByte(data, inverted);
    appendByte(data, longDiag);
    appendInt32(data, customEdge);
    appendInt32(data, FLAG_VAL);
  }

  void appendCliffInfo(std::vector<uint8_t> &data, int32_t tileIndex, float u0, float v0, float u1,
                       float v1, float u2, float v2, float u3, float v3, int8_t flip,
                       int8_t mutant) {
    appendInt32(data, tileIndex);
    appendFloat(data, u0);
    appendFloat(data, v0);
    appendFloat(data, u1);
    appendFloat(data, v1);
    appendFloat(data, u2);
    appendFloat(data, v2);
    appendFloat(data, u3);
    appendFloat(data, v3);
    appendByte(data, flip);
    appendByte(data, mutant);
  }

  static constexpr int32_t kWidth = 8;
  static constexpr int32_t kHeight = 8;
  static constexpr int32_t kDataSize = kWidth * kHeight;
};

TEST_F(BlendTileParserTest, ParsesVersion1) {
  auto data = buildTOC({
      {"BlendTileData", 1}
  });

  appendChunkHeader(data, 1, K_BLEND_TILE_VERSION_1, 0);
  size_t chunkSizePos = data.size() - 4;

  appendInt32(data, kDataSize);
  appendInt16Array(data, kDataSize, 5);
  appendInt16Array(data, kDataSize, 0);

  appendInt32(data, 10);
  appendInt32(data, 1);

  appendInt32(data, 1);
  appendTextureClass(data, 0, 4, 2, "TEDesert1");

  int32_t actualSize = static_cast<int32_t>(data.size() - chunkSizePos - 4);
  std::memcpy(&data[chunkSizePos], &actualSize, 4);

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());

  auto result = BlendTileParser::parse(reader, header->version, kWidth, kHeight);
  ASSERT_TRUE(result.has_value());

  EXPECT_EQ(result->dataSize, kDataSize);
  EXPECT_EQ(static_cast<int32_t>(result->tileNdxes.size()), kDataSize);
  EXPECT_EQ(static_cast<int32_t>(result->blendTileNdxes.size()), kDataSize);
  EXPECT_TRUE(result->extraBlendTileNdxes.empty());
  EXPECT_TRUE(result->cliffInfoNdxes.empty());
  EXPECT_TRUE(result->cellCliffState.empty());
  EXPECT_EQ(result->numBitmapTiles, 10);
  EXPECT_EQ(result->numBlendedTiles, 1);
  EXPECT_EQ(result->numCliffInfo, 0);
  EXPECT_EQ(result->textureClasses.size(), 1u);
  EXPECT_EQ(result->textureClasses[0].name, "TEDesert1");
  EXPECT_EQ(result->textureClasses[0].firstTile, 0);
  EXPECT_EQ(result->textureClasses[0].numTiles, 4);
  EXPECT_EQ(result->textureClasses[0].width, 2);
  EXPECT_TRUE(result->edgeTextureClasses.empty());
  EXPECT_TRUE(result->blendTileInfos.empty());
  EXPECT_TRUE(result->cliffInfos.empty());
  EXPECT_TRUE(result->isValid());

  for (int32_t i = 0; i < kDataSize; ++i) {
    EXPECT_EQ(result->tileNdxes[i], 5);
    EXPECT_EQ(result->blendTileNdxes[i], 0);
  }
}

TEST_F(BlendTileParserTest, ParsesVersion2WithBlendTiles) {
  auto data = buildTOC({
      {"BlendTileData", 1}
  });

  appendChunkHeader(data, 1, K_BLEND_TILE_VERSION_2, 0);
  size_t chunkSizePos = data.size() - 4;

  appendInt32(data, kDataSize);
  appendInt16Array(data, kDataSize, 3);
  appendInt16Array(data, kDataSize, 1);

  appendInt32(data, 8);
  appendInt32(data, 3);

  appendInt32(data, 1);
  appendTextureClass(data, 0, 4, 2, "GrassLight");

  appendBlendTileInfoV2(data, 2, 1, 0, 0, 0, 0);
  appendBlendTileInfoV2(data, 4, 0, 1, 0, 0, 1);

  int32_t actualSize = static_cast<int32_t>(data.size() - chunkSizePos - 4);
  std::memcpy(&data[chunkSizePos], &actualSize, 4);

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());

  auto result = BlendTileParser::parse(reader, header->version, kWidth, kHeight);
  ASSERT_TRUE(result.has_value());

  EXPECT_EQ(result->numBlendedTiles, 3);
  EXPECT_EQ(result->blendTileInfos.size(), 2u);
  EXPECT_EQ(result->blendTileInfos[0].blendNdx, 2);
  EXPECT_EQ(result->blendTileInfos[0].horiz, 1);
  EXPECT_EQ(result->blendTileInfos[0].vert, 0);
  EXPECT_EQ(result->blendTileInfos[0].inverted, 0);
  EXPECT_EQ(result->blendTileInfos[0].longDiagonal, 0);
  EXPECT_EQ(result->blendTileInfos[0].customBlendEdgeClass, -1);
  EXPECT_EQ(result->blendTileInfos[1].blendNdx, 4);
  EXPECT_EQ(result->blendTileInfos[1].vert, 1);
  EXPECT_EQ(result->blendTileInfos[1].inverted, 1);
  EXPECT_TRUE(result->isValid());
}

TEST_F(BlendTileParserTest, ParsesVersion3WithLongDiagonal) {
  auto data = buildTOC({
      {"BlendTileData", 1}
  });

  appendChunkHeader(data, 1, K_BLEND_TILE_VERSION_3, 0);
  size_t chunkSizePos = data.size() - 4;

  appendInt32(data, kDataSize);
  appendInt16Array(data, kDataSize, 0);
  appendInt16Array(data, kDataSize, 0);

  appendInt32(data, 4);
  appendInt32(data, 2);

  appendInt32(data, 1);
  appendTextureClass(data, 0, 4, 2, "SnowHeavy");

  appendBlendTileInfoV3(data, 1, 0, 0, 1, 0, 0, 1);

  int32_t actualSize = static_cast<int32_t>(data.size() - chunkSizePos - 4);
  std::memcpy(&data[chunkSizePos], &actualSize, 4);

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());

  auto result = BlendTileParser::parse(reader, header->version, kWidth, kHeight);
  ASSERT_TRUE(result.has_value());

  EXPECT_EQ(result->blendTileInfos.size(), 1u);
  EXPECT_EQ(result->blendTileInfos[0].rightDiagonal, 1);
  EXPECT_EQ(result->blendTileInfos[0].longDiagonal, 1);
  EXPECT_TRUE(result->isValid());
}

TEST_F(BlendTileParserTest, ParsesVersion4WithEdgeTextureClasses) {
  auto data = buildTOC({
      {"BlendTileData", 1}
  });

  appendChunkHeader(data, 1, K_BLEND_TILE_VERSION_4, 0);
  size_t chunkSizePos = data.size() - 4;

  appendInt32(data, kDataSize);
  appendInt16Array(data, kDataSize, 1);
  appendInt16Array(data, kDataSize, 0);

  appendInt32(data, 8);
  appendInt32(data, 2);

  appendInt32(data, 1);
  appendTextureClass(data, 0, 4, 2, "Urban1");

  appendInt32(data, 2);
  appendInt32(data, 1);
  appendEdgeTextureClass(data, 0, 2, 1, "CliffEdge1");

  appendBlendTileInfoV4(data, 3, 1, 0, 0, 0, 0, 0, 0);

  int32_t actualSize = static_cast<int32_t>(data.size() - chunkSizePos - 4);
  std::memcpy(&data[chunkSizePos], &actualSize, 4);

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());

  auto result = BlendTileParser::parse(reader, header->version, kWidth, kHeight);
  ASSERT_TRUE(result.has_value());

  EXPECT_EQ(result->numEdgeTiles, 2);
  EXPECT_EQ(result->edgeTextureClasses.size(), 1u);
  EXPECT_EQ(result->edgeTextureClasses[0].name, "CliffEdge1");
  EXPECT_EQ(result->edgeTextureClasses[0].firstTile, 0);
  EXPECT_EQ(result->edgeTextureClasses[0].numTiles, 2);
  EXPECT_EQ(result->edgeTextureClasses[0].width, 1);
  EXPECT_EQ(result->blendTileInfos[0].customBlendEdgeClass, 0);
  EXPECT_TRUE(result->isValid());
}

TEST_F(BlendTileParserTest, ParsesVersion5WithCliffInfo) {
  auto data = buildTOC({
      {"BlendTileData", 1}
  });

  appendChunkHeader(data, 1, K_BLEND_TILE_VERSION_5, 0);
  size_t chunkSizePos = data.size() - 4;

  appendInt32(data, kDataSize);
  appendInt16Array(data, kDataSize, 0);
  appendInt16Array(data, kDataSize, 0);
  appendInt16Array(data, kDataSize, 1);

  appendInt32(data, 4);
  appendInt32(data, 1);
  appendInt32(data, 2);

  appendInt32(data, 1);
  appendTextureClass(data, 0, 4, 2, "Desert1");

  appendInt32(data, 0);
  appendInt32(data, 0);

  appendCliffInfo(data, 5, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1, 0);

  int32_t actualSize = static_cast<int32_t>(data.size() - chunkSizePos - 4);
  std::memcpy(&data[chunkSizePos], &actualSize, 4);

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());

  auto result = BlendTileParser::parse(reader, header->version, kWidth, kHeight);
  ASSERT_TRUE(result.has_value());

  EXPECT_EQ(result->numCliffInfo, 2);
  EXPECT_EQ(result->cliffInfos.size(), 1u);
  EXPECT_EQ(result->cliffInfos[0].tileIndex, 5);
  EXPECT_FLOAT_EQ(result->cliffInfos[0].u0, 0.0f);
  EXPECT_FLOAT_EQ(result->cliffInfos[0].v0, 0.0f);
  EXPECT_FLOAT_EQ(result->cliffInfos[0].u1, 0.0f);
  EXPECT_FLOAT_EQ(result->cliffInfos[0].v1, 1.0f);
  EXPECT_FLOAT_EQ(result->cliffInfos[0].u2, 1.0f);
  EXPECT_FLOAT_EQ(result->cliffInfos[0].v2, 1.0f);
  EXPECT_FLOAT_EQ(result->cliffInfos[0].u3, 1.0f);
  EXPECT_FLOAT_EQ(result->cliffInfos[0].v3, 0.0f);
  EXPECT_EQ(result->cliffInfos[0].flip, 1);
  EXPECT_EQ(result->cliffInfos[0].mutant, 0);
  EXPECT_EQ(static_cast<int32_t>(result->cliffInfoNdxes.size()), kDataSize);
  EXPECT_TRUE(result->isValid());
}

TEST_F(BlendTileParserTest, ParsesVersion6WithExtraBlend) {
  auto data = buildTOC({
      {"BlendTileData", 1}
  });

  appendChunkHeader(data, 1, K_BLEND_TILE_VERSION_6, 0);
  size_t chunkSizePos = data.size() - 4;

  appendInt32(data, kDataSize);
  appendInt16Array(data, kDataSize, 2);
  appendInt16Array(data, kDataSize, 0);
  appendInt16Array(data, kDataSize, 3);
  appendInt16Array(data, kDataSize, 0);

  appendInt32(data, 4);
  appendInt32(data, 1);
  appendInt32(data, 1);

  appendInt32(data, 1);
  appendTextureClass(data, 0, 4, 2, "Asphalt1");

  appendInt32(data, 0);
  appendInt32(data, 0);

  int32_t actualSize = static_cast<int32_t>(data.size() - chunkSizePos - 4);
  std::memcpy(&data[chunkSizePos], &actualSize, 4);

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());

  auto result = BlendTileParser::parse(reader, header->version, kWidth, kHeight);
  ASSERT_TRUE(result.has_value());

  EXPECT_EQ(static_cast<int32_t>(result->extraBlendTileNdxes.size()), kDataSize);
  for (int32_t i = 0; i < kDataSize; ++i) {
    EXPECT_EQ(result->extraBlendTileNdxes[i], 3);
  }
  EXPECT_TRUE(result->isValid());
}

TEST_F(BlendTileParserTest, ParsesVersion7WithCliffState) {
  auto data = buildTOC({
      {"BlendTileData", 1}
  });

  appendChunkHeader(data, 1, K_BLEND_TILE_VERSION_7, 0);
  size_t chunkSizePos = data.size() - 4;

  appendInt32(data, kDataSize);
  appendInt16Array(data, kDataSize, 0);
  appendInt16Array(data, kDataSize, 0);
  appendInt16Array(data, kDataSize, 0);
  appendInt16Array(data, kDataSize, 0);

  int32_t flipStateWidthV7 = (kWidth + 1) / 8;
  size_t cliffStateSize = static_cast<size_t>(kHeight) * flipStateWidthV7;
  for (size_t i = 0; i < cliffStateSize; ++i) {
    data.push_back(0xAA);
  }

  appendInt32(data, 4);
  appendInt32(data, 1);
  appendInt32(data, 1);

  appendInt32(data, 1);
  appendTextureClass(data, 0, 4, 2, "Rock1");

  appendInt32(data, 0);
  appendInt32(data, 0);

  int32_t actualSize = static_cast<int32_t>(data.size() - chunkSizePos - 4);
  std::memcpy(&data[chunkSizePos], &actualSize, 4);

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());

  auto result = BlendTileParser::parse(reader, header->version, kWidth, kHeight);
  ASSERT_TRUE(result.has_value());

  EXPECT_EQ(result->cellCliffState.size(), cliffStateSize);
  for (size_t i = 0; i < cliffStateSize; ++i) {
    EXPECT_EQ(result->cellCliffState[i], 0xAA);
  }
  EXPECT_TRUE(result->isValid());
}

TEST_F(BlendTileParserTest, ParsesVersion8WithCorrectedCliffStateWidth) {
  auto data = buildTOC({
      {"BlendTileData", 1}
  });

  appendChunkHeader(data, 1, K_BLEND_TILE_VERSION_8, 0);
  size_t chunkSizePos = data.size() - 4;

  appendInt32(data, kDataSize);
  appendInt16Array(data, kDataSize, 0);
  appendInt16Array(data, kDataSize, 0);
  appendInt16Array(data, kDataSize, 0);
  appendInt16Array(data, kDataSize, 0);

  int32_t flipStateWidthV8 = (kWidth + 7) / 8;
  size_t cliffStateSize = static_cast<size_t>(kHeight) * flipStateWidthV8;
  for (size_t i = 0; i < cliffStateSize; ++i) {
    data.push_back(0x55);
  }

  appendInt32(data, 4);
  appendInt32(data, 1);
  appendInt32(data, 1);

  appendInt32(data, 1);
  appendTextureClass(data, 0, 4, 2, "TEDesert1");

  appendInt32(data, 0);
  appendInt32(data, 0);

  int32_t actualSize = static_cast<int32_t>(data.size() - chunkSizePos - 4);
  std::memcpy(&data[chunkSizePos], &actualSize, 4);

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());

  auto result = BlendTileParser::parse(reader, header->version, kWidth, kHeight);
  ASSERT_TRUE(result.has_value());

  EXPECT_EQ(result->cellCliffState.size(), cliffStateSize);
  for (size_t i = 0; i < cliffStateSize; ++i) {
    EXPECT_EQ(result->cellCliffState[i], 0x55);
  }
  EXPECT_TRUE(result->isValid());
}

TEST_F(BlendTileParserTest, ParsesFullVersion8WithAllFeatures) {
  auto data = buildTOC({
      {"BlendTileData", 1}
  });

  appendChunkHeader(data, 1, K_BLEND_TILE_VERSION_8, 0);
  size_t chunkSizePos = data.size() - 4;

  appendInt32(data, kDataSize);

  for (int32_t i = 0; i < kDataSize; ++i) {
    appendInt16(data, static_cast<int16_t>(i % 8));
  }
  for (int32_t i = 0; i < kDataSize; ++i) {
    appendInt16(data, static_cast<int16_t>(i < 4 ? 1 : 0));
  }
  appendInt16Array(data, kDataSize, 2);
  for (int32_t i = 0; i < kDataSize; ++i) {
    appendInt16(data, static_cast<int16_t>(i == 0 ? 1 : 0));
  }

  int32_t flipStateWidth = (kWidth + 7) / 8;
  size_t cliffStateSize = static_cast<size_t>(kHeight) * flipStateWidth;
  for (size_t i = 0; i < cliffStateSize; ++i) {
    data.push_back(static_cast<uint8_t>(i & 0xFF));
  }

  appendInt32(data, 16);
  appendInt32(data, 3);
  appendInt32(data, 2);

  appendInt32(data, 2);
  appendTextureClass(data, 0, 4, 2, "TEDesert1");
  appendTextureClass(data, 4, 4, 2, "GrassLight");

  appendInt32(data, 4);
  appendInt32(data, 1);
  appendEdgeTextureClass(data, 0, 4, 2, "CliffDesert");

  appendBlendTileInfoV4(data, 2, 1, 0, 0, 0, 0, 0, -1);
  appendBlendTileInfoV4(data, 5, 0, 1, 0, 0, static_cast<int8_t>(INVERTED_MASK), 1, 0);

  appendCliffInfo(data, 3, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0, 1);

  int32_t actualSize = static_cast<int32_t>(data.size() - chunkSizePos - 4);
  std::memcpy(&data[chunkSizePos], &actualSize, 4);

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());

  auto result = BlendTileParser::parse(reader, header->version, kWidth, kHeight);
  ASSERT_TRUE(result.has_value());

  EXPECT_EQ(result->dataSize, kDataSize);
  EXPECT_EQ(result->numBitmapTiles, 16);
  EXPECT_EQ(result->numBlendedTiles, 3);
  EXPECT_EQ(result->numCliffInfo, 2);

  EXPECT_EQ(result->tileNdxes[0], 0);
  EXPECT_EQ(result->tileNdxes[3], 3);
  EXPECT_EQ(result->tileNdxes[7], 7);

  EXPECT_EQ(result->blendTileNdxes[0], 1);
  EXPECT_EQ(result->blendTileNdxes[4], 0);

  EXPECT_EQ(static_cast<int32_t>(result->extraBlendTileNdxes.size()), kDataSize);
  EXPECT_EQ(result->extraBlendTileNdxes[0], 2);

  EXPECT_EQ(result->cliffInfoNdxes[0], 1);
  EXPECT_EQ(result->cliffInfoNdxes[1], 0);

  EXPECT_EQ(result->textureClasses.size(), 2u);
  EXPECT_EQ(result->textureClasses[0].name, "TEDesert1");
  EXPECT_EQ(result->textureClasses[1].name, "GrassLight");
  EXPECT_EQ(result->textureClasses[1].firstTile, 4);

  EXPECT_EQ(result->edgeTextureClasses.size(), 1u);
  EXPECT_EQ(result->edgeTextureClasses[0].name, "CliffDesert");
  EXPECT_EQ(result->numEdgeTiles, 4);

  EXPECT_EQ(result->blendTileInfos.size(), 2u);
  EXPECT_EQ(result->blendTileInfos[0].blendNdx, 2);
  EXPECT_EQ(result->blendTileInfos[0].horiz, 1);
  EXPECT_EQ(result->blendTileInfos[0].customBlendEdgeClass, -1);
  EXPECT_EQ(result->blendTileInfos[1].blendNdx, 5);
  EXPECT_EQ(result->blendTileInfos[1].vert, 1);
  EXPECT_EQ(result->blendTileInfos[1].inverted, INVERTED_MASK);
  EXPECT_EQ(result->blendTileInfos[1].longDiagonal, 1);
  EXPECT_EQ(result->blendTileInfos[1].customBlendEdgeClass, 0);

  EXPECT_EQ(result->cliffInfos.size(), 1u);
  EXPECT_FLOAT_EQ(result->cliffInfos[0].u0, 0.1f);
  EXPECT_FLOAT_EQ(result->cliffInfos[0].v0, 0.2f);
  EXPECT_FLOAT_EQ(result->cliffInfos[0].u3, 0.7f);
  EXPECT_FLOAT_EQ(result->cliffInfos[0].v3, 0.8f);
  EXPECT_EQ(result->cliffInfos[0].tileIndex, 3);
  EXPECT_EQ(result->cliffInfos[0].flip, 0);
  EXPECT_EQ(result->cliffInfos[0].mutant, 1);

  EXPECT_TRUE(result->isValid());
}

TEST_F(BlendTileParserTest, RejectsUnsupportedVersion) {
  auto data = buildTOC({
      {"BlendTileData", 1}
  });
  appendChunkHeader(data, 1, 99, 4);
  appendInt32(data, 0);

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value());

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());

  std::string parseError;
  auto result = BlendTileParser::parse(reader, header->version, kWidth, kHeight, &parseError);
  EXPECT_FALSE(result.has_value());
  EXPECT_NE(parseError.find("Unsupported"), std::string::npos);
}

TEST_F(BlendTileParserTest, RejectsInvalidFlagSentinel) {
  auto data = buildTOC({
      {"BlendTileData", 1}
  });

  appendChunkHeader(data, 1, K_BLEND_TILE_VERSION_2, 0);
  size_t chunkSizePos = data.size() - 4;

  appendInt32(data, kDataSize);
  appendInt16Array(data, kDataSize, 0);
  appendInt16Array(data, kDataSize, 0);

  appendInt32(data, 4);
  appendInt32(data, 2);

  appendInt32(data, 1);
  appendTextureClass(data, 0, 4, 2, "Desert1");

  appendInt32(data, 1);
  appendByte(data, 1);
  appendByte(data, 0);
  appendByte(data, 0);
  appendByte(data, 0);
  appendByte(data, 0);
  appendInt32(data, 0xDEADBEEF);

  int32_t actualSize = static_cast<int32_t>(data.size() - chunkSizePos - 4);
  std::memcpy(&data[chunkSizePos], &actualSize, 4);

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value());

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());

  std::string parseError;
  auto result = BlendTileParser::parse(reader, header->version, kWidth, kHeight, &parseError);
  EXPECT_FALSE(result.has_value());
  EXPECT_NE(parseError.find("sentinel"), std::string::npos);
}

TEST_F(BlendTileParserTest, ParsesMultipleTextureClasses) {
  auto data = buildTOC({
      {"BlendTileData", 1}
  });

  appendChunkHeader(data, 1, K_BLEND_TILE_VERSION_2, 0);
  size_t chunkSizePos = data.size() - 4;

  appendInt32(data, kDataSize);
  appendInt16Array(data, kDataSize, 0);
  appendInt16Array(data, kDataSize, 0);

  appendInt32(data, 24);
  appendInt32(data, 1);

  appendInt32(data, 4);
  appendTextureClass(data, 0, 4, 2, "TEDesert1");
  appendTextureClass(data, 4, 4, 2, "TEDesert2");
  appendTextureClass(data, 8, 8, 2, "GrassLight");
  appendTextureClass(data, 16, 4, 2, "SnowHeavy");

  int32_t actualSize = static_cast<int32_t>(data.size() - chunkSizePos - 4);
  std::memcpy(&data[chunkSizePos], &actualSize, 4);

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());

  auto result = BlendTileParser::parse(reader, header->version, kWidth, kHeight);
  ASSERT_TRUE(result.has_value());

  EXPECT_EQ(result->textureClasses.size(), 4u);
  EXPECT_EQ(result->textureClasses[0].name, "TEDesert1");
  EXPECT_EQ(result->textureClasses[0].firstTile, 0);
  EXPECT_EQ(result->textureClasses[1].name, "TEDesert2");
  EXPECT_EQ(result->textureClasses[1].firstTile, 4);
  EXPECT_EQ(result->textureClasses[2].name, "GrassLight");
  EXPECT_EQ(result->textureClasses[2].firstTile, 8);
  EXPECT_EQ(result->textureClasses[2].numTiles, 8);
  EXPECT_EQ(result->textureClasses[3].name, "SnowHeavy");
  EXPECT_EQ(result->textureClasses[3].firstTile, 16);
  EXPECT_TRUE(result->isValid());
}

TEST_F(BlendTileParserTest, ParsesZeroBlendedTiles) {
  auto data = buildTOC({
      {"BlendTileData", 1}
  });

  appendChunkHeader(data, 1, K_BLEND_TILE_VERSION_2, 0);
  size_t chunkSizePos = data.size() - 4;

  appendInt32(data, kDataSize);
  appendInt16Array(data, kDataSize, 0);
  appendInt16Array(data, kDataSize, 0);

  appendInt32(data, 4);
  appendInt32(data, 0);

  appendInt32(data, 1);
  appendTextureClass(data, 0, 4, 2, "Desert1");

  int32_t actualSize = static_cast<int32_t>(data.size() - chunkSizePos - 4);
  std::memcpy(&data[chunkSizePos], &actualSize, 4);

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());

  auto result = BlendTileParser::parse(reader, header->version, kWidth, kHeight);
  ASSERT_TRUE(result.has_value());

  EXPECT_TRUE(result->blendTileInfos.empty());
  EXPECT_TRUE(result->isValid());
}

TEST_F(BlendTileParserTest, BlendTileDataValidation) {
  BlendTileData btd;
  EXPECT_FALSE(btd.isValid());

  btd.dataSize = 4;
  btd.tileNdxes.resize(4);
  btd.blendTileNdxes.resize(4);
  EXPECT_TRUE(btd.isValid());

  btd.tileNdxes.resize(3);
  EXPECT_FALSE(btd.isValid());
}

TEST_F(BlendTileParserTest, ParsesVersion7CliffStateWidthBug) {
  constexpr int32_t w = 9;
  constexpr int32_t h = 4;
  constexpr int32_t ds = w * h;

  auto data = buildTOC({
      {"BlendTileData", 1}
  });

  appendChunkHeader(data, 1, K_BLEND_TILE_VERSION_7, 0);
  size_t chunkSizePos = data.size() - 4;

  appendInt32(data, ds);
  appendInt16Array(data, ds, 0);
  appendInt16Array(data, ds, 0);
  appendInt16Array(data, ds, 0);
  appendInt16Array(data, ds, 0);

  int32_t flipStateWidthV7 = (w + 1) / 8;
  EXPECT_EQ(flipStateWidthV7, 1);
  size_t cliffStateSize = static_cast<size_t>(h) * flipStateWidthV7;
  for (size_t i = 0; i < cliffStateSize; ++i) {
    data.push_back(0xFF);
  }

  appendInt32(data, 4);
  appendInt32(data, 1);
  appendInt32(data, 1);

  appendInt32(data, 1);
  appendTextureClass(data, 0, 4, 2, "TestTerrain");

  appendInt32(data, 0);
  appendInt32(data, 0);

  int32_t actualSize = static_cast<int32_t>(data.size() - chunkSizePos - 4);
  std::memcpy(&data[chunkSizePos], &actualSize, 4);

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());

  auto result = BlendTileParser::parse(reader, header->version, w, h);
  ASSERT_TRUE(result.has_value());

  EXPECT_EQ(result->cellCliffState.size(), cliffStateSize);

  int32_t flipStateWidthV8 = (w + 7) / 8;
  EXPECT_EQ(flipStateWidthV8, 2);
  EXPECT_NE(flipStateWidthV7, flipStateWidthV8);
}

TEST_F(BlendTileParserTest, ConstantsHaveCorrectValues) {
  EXPECT_EQ(FLAG_VAL, 0x7ADA0000);
  EXPECT_EQ(INVERTED_MASK, 0x1);
  EXPECT_EQ(FLIPPED_MASK, 0x2);
  EXPECT_EQ(TILE_PIXEL_EXTENT, 64);
}
