#include <fstream>
#include <vector>

#include "../../src/lib/formats/map/data_chunk_reader.hpp"
#include "../../src/lib/formats/map/heightmap_parser.hpp"
#include "../../src/lib/formats/map/types.hpp"

#include <gtest/gtest.h>

using namespace map;

class RealMapLoadingTest : public ::testing::Test {
protected:
  std::vector<uint8_t> loadFile(const std::string &path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
      return {};
    }
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(size);
    file.read(reinterpret_cast<char *>(data.data()), size);
    return data;
  }
};

TEST_F(RealMapLoadingTest, LoadsTansooMapHeightData) {
  const char *mapPath =
      "lib/GeneralsGameCode/GeneralsReplays/GeneralsZH/1.04/Maps/tansooo/tansooo.map";
  auto data = loadFile(mapPath);

  if (data.empty()) {
    GTEST_SKIP() << "Map file not found: " << mapPath;
  }

  GTEST_SKIP() << "Map files from GeneralsGameCode are compressed (EAR header). "
               << "Decompression support will be added in a future phase. "
               << "This test is kept to document the compression format.";

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << "Failed to load TOC: " << *error;

  while (!reader.atEnd()) {
    auto header = reader.openChunk();
    ASSERT_TRUE(header.has_value()) << "Failed to read chunk header";

    auto chunkName = reader.lookupName(header->id);
    ASSERT_TRUE(chunkName.has_value());

    if (*chunkName == "HeightMapData") {
      auto heightMap = HeightMapParser::parse(reader, header->version);
      ASSERT_TRUE(heightMap.has_value()) << "Failed to parse HeightMapData";

      EXPECT_GT(heightMap->width, 0);
      EXPECT_GT(heightMap->height, 0);
      EXPECT_TRUE(heightMap->isValid());
      EXPECT_EQ(static_cast<int32_t>(heightMap->data.size()), heightMap->width * heightMap->height);

      reader.closeChunk();
      return;
    }

    reader.closeChunk();
  }

  FAIL() << "HeightMapData chunk not found in map file";
}

TEST_F(RealMapLoadingTest, LoadsArcticArenaMapHeightData) {
  const char *mapPath = "lib/GeneralsGameCode/GeneralsReplays/GeneralsZH/1.04/Maps/[RANK] Arctic "
                        "Arena ZH v1/[RANK] Arctic Arena ZH v1.map";
  auto data = loadFile(mapPath);

  if (data.empty()) {
    GTEST_SKIP() << "Map file not found: " << mapPath;
  }

  GTEST_SKIP() << "Map files from GeneralsGameCode are compressed (EAR header). "
               << "Decompression support will be added in a future phase. "
               << "This test is kept to document the compression format.";

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << "Failed to load TOC: " << *error;

  while (!reader.atEnd()) {
    auto header = reader.openChunk();
    ASSERT_TRUE(header.has_value()) << "Failed to read chunk header";

    auto chunkName = reader.lookupName(header->id);
    ASSERT_TRUE(chunkName.has_value());

    if (*chunkName == "HeightMapData") {
      auto heightMap = HeightMapParser::parse(reader, header->version);
      ASSERT_TRUE(heightMap.has_value()) << "Failed to parse HeightMapData";

      EXPECT_GT(heightMap->width, 0);
      EXPECT_GT(heightMap->height, 0);
      EXPECT_TRUE(heightMap->isValid());
      EXPECT_EQ(static_cast<int32_t>(heightMap->data.size()), heightMap->width * heightMap->height);

      uint8_t minHeight = 255;
      uint8_t maxHeight = 0;
      for (uint8_t h : heightMap->data) {
        if (h < minHeight)
          minHeight = h;
        if (h > maxHeight)
          maxHeight = h;
      }

      EXPECT_LT(minHeight, maxHeight) << "Map should have terrain variation";

      reader.closeChunk();
      return;
    }

    reader.closeChunk();
  }

  FAIL() << "HeightMapData chunk not found in map file";
}
