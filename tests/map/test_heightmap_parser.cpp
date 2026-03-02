#include <cstring>

#include "../../src/lib/formats/map/data_chunk_reader.hpp"
#include "../../src/lib/formats/map/heightmap_parser.hpp"
#include "../../src/lib/formats/map/types.hpp"

#include <gtest/gtest.h>

using namespace map;

class HeightMapParserTest : public ::testing::Test {
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
};

TEST_F(HeightMapParserTest, ParsesVersion1) {
  auto data = buildTOC({
      {"HeightMapData", 1}
  });

  appendChunkHeader(data, 1, K_HEIGHT_MAP_VERSION_1, 0);
  size_t chunkSizePos = data.size() - 4;

  int32_t width = 20;
  int32_t height = 20;
  appendInt32(data, width);
  appendInt32(data, height);

  int32_t dataSize = width * height;
  appendInt32(data, dataSize);

  for (int32_t i = 0; i < dataSize; ++i) {
    data.push_back(static_cast<uint8_t>(i % 256));
  }

  int32_t actualSize = static_cast<int32_t>(data.size() - chunkSizePos - 4);
  std::memcpy(&data[chunkSizePos], &actualSize, 4);

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value());

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());

  auto heightMap = HeightMapParser::parse(reader, header->version);
  ASSERT_TRUE(heightMap.has_value());

  EXPECT_EQ(heightMap->width, width / 2);
  EXPECT_EQ(heightMap->height, height / 2);
  EXPECT_EQ(heightMap->borderSize, 0);
  EXPECT_EQ(heightMap->boundaries.size(), 1u);
  EXPECT_EQ(heightMap->boundaries[0].x, width / 2);
  EXPECT_EQ(heightMap->boundaries[0].y, height / 2);
  EXPECT_EQ(static_cast<int32_t>(heightMap->data.size()), (width / 2) * (height / 2));
  EXPECT_TRUE(heightMap->isValid());
}

TEST_F(HeightMapParserTest, ParsesVersion2) {
  auto data = buildTOC({
      {"HeightMapData", 1}
  });

  appendChunkHeader(data, 1, K_HEIGHT_MAP_VERSION_2, 0);
  size_t chunkSizePos = data.size() - 4;

  int32_t width = 64;
  int32_t height = 64;
  appendInt32(data, width);
  appendInt32(data, height);

  int32_t dataSize = width * height;
  appendInt32(data, dataSize);

  for (int32_t i = 0; i < dataSize; ++i) {
    data.push_back(static_cast<uint8_t>(i % 128));
  }

  int32_t actualSize = static_cast<int32_t>(data.size() - chunkSizePos - 4);
  std::memcpy(&data[chunkSizePos], &actualSize, 4);

  DataChunkReader reader;
  reader.loadFromMemory(data);

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());

  auto heightMap = HeightMapParser::parse(reader, header->version);
  ASSERT_TRUE(heightMap.has_value());

  EXPECT_EQ(heightMap->width, width);
  EXPECT_EQ(heightMap->height, height);
  EXPECT_EQ(heightMap->borderSize, 0);
  EXPECT_EQ(heightMap->boundaries.size(), 1u);
  EXPECT_EQ(heightMap->boundaries[0].x, width);
  EXPECT_EQ(heightMap->boundaries[0].y, height);
  EXPECT_EQ(static_cast<int32_t>(heightMap->data.size()), dataSize);
  EXPECT_TRUE(heightMap->isValid());
}

TEST_F(HeightMapParserTest, ParsesVersion3) {
  auto data = buildTOC({
      {"HeightMapData", 1}
  });

  appendChunkHeader(data, 1, K_HEIGHT_MAP_VERSION_3, 0);
  size_t chunkSizePos = data.size() - 4;

  int32_t width = 128;
  int32_t height = 128;
  int32_t borderSize = 8;
  appendInt32(data, width);
  appendInt32(data, height);
  appendInt32(data, borderSize);

  int32_t dataSize = width * height;
  appendInt32(data, dataSize);

  for (int32_t i = 0; i < dataSize; ++i) {
    data.push_back(static_cast<uint8_t>((i * 7) % 256));
  }

  int32_t actualSize = static_cast<int32_t>(data.size() - chunkSizePos - 4);
  std::memcpy(&data[chunkSizePos], &actualSize, 4);

  DataChunkReader reader;
  reader.loadFromMemory(data);

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());

  auto heightMap = HeightMapParser::parse(reader, header->version);
  ASSERT_TRUE(heightMap.has_value());

  EXPECT_EQ(heightMap->width, width);
  EXPECT_EQ(heightMap->height, height);
  EXPECT_EQ(heightMap->borderSize, borderSize);
  EXPECT_EQ(heightMap->boundaries.size(), 1u);
  EXPECT_EQ(heightMap->boundaries[0].x, width - 2 * borderSize);
  EXPECT_EQ(heightMap->boundaries[0].y, height - 2 * borderSize);
  EXPECT_EQ(static_cast<int32_t>(heightMap->data.size()), dataSize);
  EXPECT_TRUE(heightMap->isValid());
}

TEST_F(HeightMapParserTest, ParsesVersion4) {
  auto data = buildTOC({
      {"HeightMapData", 1}
  });

  appendChunkHeader(data, 1, K_HEIGHT_MAP_VERSION_4, 0);
  size_t chunkSizePos = data.size() - 4;

  int32_t width = 256;
  int32_t height = 256;
  int32_t borderSize = 16;
  int32_t numBoundaries = 2;

  appendInt32(data, width);
  appendInt32(data, height);
  appendInt32(data, borderSize);
  appendInt32(data, numBoundaries);

  appendInt32(data, 200);
  appendInt32(data, 200);
  appendInt32(data, 100);
  appendInt32(data, 100);

  int32_t dataSize = width * height;
  appendInt32(data, dataSize);

  for (int32_t i = 0; i < dataSize; ++i) {
    data.push_back(static_cast<uint8_t>((i * 13) % 256));
  }

  int32_t actualSize = static_cast<int32_t>(data.size() - chunkSizePos - 4);
  std::memcpy(&data[chunkSizePos], &actualSize, 4);

  DataChunkReader reader;
  reader.loadFromMemory(data);

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());

  auto heightMap = HeightMapParser::parse(reader, header->version);
  ASSERT_TRUE(heightMap.has_value());

  EXPECT_EQ(heightMap->width, width);
  EXPECT_EQ(heightMap->height, height);
  EXPECT_EQ(heightMap->borderSize, borderSize);
  EXPECT_EQ(heightMap->boundaries.size(), 2u);
  EXPECT_EQ(heightMap->boundaries[0].x, 200);
  EXPECT_EQ(heightMap->boundaries[0].y, 200);
  EXPECT_EQ(heightMap->boundaries[1].x, 100);
  EXPECT_EQ(heightMap->boundaries[1].y, 100);
  EXPECT_EQ(static_cast<int32_t>(heightMap->data.size()), dataSize);
  EXPECT_TRUE(heightMap->isValid());
}

TEST_F(HeightMapParserTest, RejectsUnsupportedVersion) {
  auto data = buildTOC({
      {"HeightMapData", 1}
  });
  appendChunkHeader(data, 1, 99, 0);

  DataChunkReader reader;
  reader.loadFromMemory(data);

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());

  std::string error;
  auto heightMap = HeightMapParser::parse(reader, header->version, &error);
  EXPECT_FALSE(heightMap.has_value());
  EXPECT_NE(error.find("Unsupported"), std::string::npos);
}

TEST_F(HeightMapParserTest, RejectsSizeMismatch) {
  auto data = buildTOC({
      {"HeightMapData", 1}
  });

  appendChunkHeader(data, 1, K_HEIGHT_MAP_VERSION_2, 0);
  size_t chunkSizePos = data.size() - 4;

  int32_t width = 64;
  int32_t height = 64;
  appendInt32(data, width);
  appendInt32(data, height);

  int32_t wrongSize = width * height - 100;
  appendInt32(data, wrongSize);

  for (int32_t i = 0; i < wrongSize; ++i) {
    data.push_back(static_cast<uint8_t>(i));
  }

  int32_t actualSize = static_cast<int32_t>(data.size() - chunkSizePos - 4);
  std::memcpy(&data[chunkSizePos], &actualSize, 4);

  DataChunkReader reader;
  reader.loadFromMemory(data);

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());

  std::string error;
  auto heightMap = HeightMapParser::parse(reader, header->version, &error);
  EXPECT_FALSE(heightMap.has_value());
  EXPECT_NE(error.find("mismatch"), std::string::npos);
}

TEST_F(HeightMapParserTest, GetWorldHeightReturnsCorrectValue) {
  HeightMap heightMap;
  heightMap.width = 4;
  heightMap.height = 4;
  heightMap.data.resize(16);

  heightMap.data[0 * 4 + 0] = 0;
  heightMap.data[1 * 4 + 2] = 16;
  heightMap.data[3 * 4 + 3] = 255;

  EXPECT_FLOAT_EQ(heightMap.getWorldHeight(0, 0), 0.0f);
  EXPECT_FLOAT_EQ(heightMap.getWorldHeight(2, 1), 16.0f * MAP_HEIGHT_SCALE);
  EXPECT_FLOAT_EQ(heightMap.getWorldHeight(3, 3), 255.0f * MAP_HEIGHT_SCALE);
  EXPECT_FLOAT_EQ(heightMap.getWorldHeight(-1, 0), 0.0f);
  EXPECT_FLOAT_EQ(heightMap.getWorldHeight(0, -1), 0.0f);
  EXPECT_FLOAT_EQ(heightMap.getWorldHeight(4, 0), 0.0f);
  EXPECT_FLOAT_EQ(heightMap.getWorldHeight(0, 4), 0.0f);
}

TEST_F(HeightMapParserTest, SetHeightModifiesData) {
  HeightMap heightMap;
  heightMap.width = 4;
  heightMap.height = 4;
  heightMap.data.resize(16, 0);

  heightMap.setHeight(2, 1, 100);
  EXPECT_EQ(heightMap.data[1 * 4 + 2], 100);

  heightMap.setHeight(-1, 0, 50);
  heightMap.setHeight(4, 0, 50);
  EXPECT_EQ(heightMap.data[0], 0);
}

TEST_F(HeightMapParserTest, GetHeightReturnsCorrectValue) {
  HeightMap heightMap;
  heightMap.width = 4;
  heightMap.height = 4;
  heightMap.data.resize(16);

  heightMap.data[1 * 4 + 2] = 123;

  EXPECT_EQ(heightMap.getHeight(2, 1), 123);
  EXPECT_EQ(heightMap.getHeight(0, 0), 0);
  EXPECT_EQ(heightMap.getHeight(-1, 0), 0);
  EXPECT_EQ(heightMap.getHeight(4, 0), 0);
}

TEST_F(HeightMapParserTest, IsValidReturnsTrueForValidHeightMap) {
  HeightMap heightMap;
  heightMap.width = 64;
  heightMap.height = 64;
  heightMap.data.resize(64 * 64);

  EXPECT_TRUE(heightMap.isValid());
}

TEST_F(HeightMapParserTest, IsValidReturnsFalseForInvalidHeightMap) {
  HeightMap heightMap1;
  heightMap1.width = 0;
  heightMap1.height = 64;
  heightMap1.data.resize(64);
  EXPECT_FALSE(heightMap1.isValid());

  HeightMap heightMap2;
  heightMap2.width = 64;
  heightMap2.height = 0;
  heightMap2.data.resize(64);
  EXPECT_FALSE(heightMap2.isValid());

  HeightMap heightMap3;
  heightMap3.width = 64;
  heightMap3.height = 64;
  heightMap3.data.resize(100);
  EXPECT_FALSE(heightMap3.isValid());
}
