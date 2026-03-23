#include <cstring>
#include <vector>

#include "../../src/lib/formats/map/data_chunk_reader.hpp"
#include "../../src/lib/formats/map/map_loader.hpp"
#include "../../src/lib/formats/map/types.hpp"

#include <gtest/gtest.h>

using namespace map;

class MapLoaderTest : public ::testing::Test {
protected:
  std::vector<uint8_t> createTOC(const std::vector<std::string> &names) {
    std::vector<uint8_t> data;

    data.push_back('C');
    data.push_back('k');
    data.push_back('M');
    data.push_back('p');

    int32_t count = static_cast<int32_t>(names.size());
    appendInt(data, count);

    for (uint32_t i = 0; i < names.size(); ++i) {
      const auto &name = names[i];
      uint8_t len = static_cast<uint8_t>(name.size());
      data.push_back(len);
      for (char c : name) {
        data.push_back(static_cast<uint8_t>(c));
      }

      uint32_t id = i + 1;
      appendInt(data, static_cast<int32_t>(id));
    }

    return data;
  }

  void appendInt(std::vector<uint8_t> &data, int32_t value) {
    data.push_back(value & 0xFF);
    data.push_back((value >> 8) & 0xFF);
    data.push_back((value >> 16) & 0xFF);
    data.push_back((value >> 24) & 0xFF);
  }

  void appendFloat(std::vector<uint8_t> &data, float value) {
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(float));
    appendInt(data, static_cast<int32_t>(bits));
  }

  void appendShort(std::vector<uint8_t> &data, uint16_t value) {
    data.push_back(value & 0xFF);
    data.push_back((value >> 8) & 0xFF);
  }

  void appendString(std::vector<uint8_t> &data, const std::string &str) {
    uint16_t len = static_cast<uint16_t>(str.size());
    appendShort(data, len);
    for (char c : str) {
      data.push_back(static_cast<uint8_t>(c));
    }
  }

  void appendByte(std::vector<uint8_t> &data, int8_t value) {
    data.push_back(static_cast<uint8_t>(value));
  }

  size_t appendChunkHeader(std::vector<uint8_t> &data, uint32_t id, uint16_t version) {
    size_t startPos = data.size();
    appendInt(data, static_cast<int32_t>(id));
    appendShort(data, version);
    appendInt(data, 0);
    return startPos;
  }

  void patchChunkSize(std::vector<uint8_t> &data, size_t headerStartPos) {
    int32_t dataSize = static_cast<int32_t>(data.size() - headerStartPos - CHUNK_HEADER_SIZE);
    std::memcpy(&data[headerStartPos + 6], &dataSize, 4);
  }

  void appendDict(std::vector<uint8_t> &data, const std::vector<std::string> &nameTable,
                  const std::vector<std::pair<std::string, DictValue>> &pairs) {
    uint16_t pairCount = static_cast<uint16_t>(pairs.size());
    appendShort(data, pairCount);

    for (const auto &[key, value] : pairs) {
      uint32_t keyId = 0;
      for (size_t i = 0; i < nameTable.size(); ++i) {
        if (nameTable[i] == key) {
          keyId = static_cast<uint32_t>(i + 1);
          break;
        }
      }

      uint32_t keyAndType = (keyId << 8) | static_cast<uint8_t>(value.type);
      appendInt(data, static_cast<int32_t>(keyAndType));

      switch (value.type) {
      case DataType::Bool:
        data.push_back(value.boolValue ? 1 : 0);
        break;
      case DataType::Int:
        appendInt(data, value.intValue);
        break;
      case DataType::Real:
        appendFloat(data, value.realValue);
        break;
      case DataType::AsciiString:
        appendString(data, value.stringValue);
        break;
      case DataType::UnicodeString:
        break;
      }
    }
  }

  uint32_t findTOCId(const std::vector<std::string> &nameTable, const std::string &name) {
    for (size_t i = 0; i < nameTable.size(); ++i) {
      if (nameTable[i] == name) {
        return static_cast<uint32_t>(i + 1);
      }
    }
    return 0;
  }

  void appendHeightMapChunk(std::vector<uint8_t> &data, const std::vector<std::string> &nameTable,
                            int32_t width, int32_t height, int32_t borderSize,
                            uint8_t fillValue = 128) {
    uint32_t chunkId = findTOCId(nameTable, "HeightMapData");
    size_t headerPos = appendChunkHeader(data, chunkId, 4);

    appendInt(data, width);
    appendInt(data, height);
    appendInt(data, borderSize);

    int32_t numBoundaries = 1;
    appendInt(data, numBoundaries);
    appendInt(data, width - 2 * borderSize);
    appendInt(data, height - 2 * borderSize);

    int32_t dataSize = width * height;
    appendInt(data, dataSize);

    for (int32_t i = 0; i < dataSize; ++i) {
      data.push_back(fillValue);
    }

    patchChunkSize(data, headerPos);
  }

  void appendBlendTileChunk(std::vector<uint8_t> &data, const std::vector<std::string> &nameTable,
                            int32_t hmWidth, int32_t hmHeight) {
    uint32_t chunkId = findTOCId(nameTable, "BlendTileData");
    size_t headerPos = appendChunkHeader(data, chunkId, 8);

    int32_t dataSize = hmWidth * hmHeight;
    appendInt(data, dataSize);

    for (int32_t i = 0; i < dataSize; ++i) {
      appendShort(data, 0);
    }
    for (int32_t i = 0; i < dataSize; ++i) {
      appendShort(data, 0);
    }
    for (int32_t i = 0; i < dataSize; ++i) {
      appendShort(data, 0);
    }
    for (int32_t i = 0; i < dataSize; ++i) {
      appendShort(data, 0);
    }

    int32_t flipStateWidth = (hmWidth + 7) / 8;
    int32_t cliffStateSize = hmHeight * flipStateWidth;
    for (int32_t i = 0; i < cliffStateSize; ++i) {
      data.push_back(0);
    }

    appendInt(data, 4);
    appendInt(data, 1);
    appendInt(data, 0);

    int32_t numTextureClasses = 1;
    appendInt(data, numTextureClasses);

    appendInt(data, 0);
    appendInt(data, 4);
    appendInt(data, 2);
    appendInt(data, 0);
    appendString(data, "TEDesert1");

    appendInt(data, 0);
    appendInt(data, 0);

    patchChunkSize(data, headerPos);
  }

  void appendObjectsListChunk(std::vector<uint8_t> &data, const std::vector<std::string> &nameTable,
                              int objectCount) {
    uint32_t listId = findTOCId(nameTable, "ObjectsList");
    uint32_t objId = findTOCId(nameTable, "Object");
    size_t listHeaderPos = appendChunkHeader(data, listId, 3);

    for (int i = 0; i < objectCount; ++i) {
      size_t objHeaderPos = appendChunkHeader(data, objId, 3);

      appendFloat(data, 100.0f * (i + 1));
      appendFloat(data, 200.0f * (i + 1));
      appendFloat(data, 10.0f * (i + 1));
      appendFloat(data, 0.5f * (i + 1));
      appendInt(data, 0);
      appendString(data, "Object" + std::to_string(i));

      appendShort(data, 0);

      patchChunkSize(data, objHeaderPos);
    }

    patchChunkSize(data, listHeaderPos);
  }

  void appendPolygonTriggersChunk(std::vector<uint8_t> &data,
                                  const std::vector<std::string> &nameTable) {
    uint32_t chunkId = findTOCId(nameTable, "PolygonTriggers");
    size_t headerPos = appendChunkHeader(data, chunkId, 3);

    int32_t count = 2;
    appendInt(data, count);

    appendString(data, "WaterArea1");
    appendInt(data, 1);
    appendByte(data, 1);
    appendByte(data, 0);
    appendInt(data, 0);
    int32_t numPoints1 = 4;
    appendInt(data, numPoints1);
    for (int32_t j = 0; j < numPoints1; ++j) {
      appendInt(data, j * 100);
      appendInt(data, j * 100);
      appendInt(data, 50);
    }

    appendString(data, "TriggerZone1");
    appendInt(data, 2);
    appendByte(data, 0);
    appendByte(data, 0);
    appendInt(data, 0);
    int32_t numPoints2 = 3;
    appendInt(data, numPoints2);
    for (int32_t j = 0; j < numPoints2; ++j) {
      appendInt(data, j * 50);
      appendInt(data, j * 50);
      appendInt(data, 0);
    }

    patchChunkSize(data, headerPos);
  }

  void appendGlobalLightingChunk(std::vector<uint8_t> &data,
                                 const std::vector<std::string> &nameTable) {
    uint32_t chunkId = findTOCId(nameTable, "GlobalLighting");
    size_t headerPos = appendChunkHeader(data, chunkId, 3);

    appendInt(data, static_cast<int32_t>(TimeOfDay::Afternoon));

    for (int slot = 0; slot < 4; ++slot) {
      appendFloat(data, 0.3f);
      appendFloat(data, 0.3f);
      appendFloat(data, 0.3f);
      appendFloat(data, 0.8f);
      appendFloat(data, 0.8f);
      appendFloat(data, 0.8f);
      appendFloat(data, 0.0f);
      appendFloat(data, 0.0f);
      appendFloat(data, -1.0f);

      appendFloat(data, 0.2f);
      appendFloat(data, 0.2f);
      appendFloat(data, 0.2f);
      appendFloat(data, 0.6f);
      appendFloat(data, 0.6f);
      appendFloat(data, 0.6f);
      appendFloat(data, 1.0f);
      appendFloat(data, 0.0f);
      appendFloat(data, 0.0f);

      for (int j = 0; j < 2; ++j) {
        for (int k = 0; k < 9; ++k) {
          appendFloat(data, 0.0f);
        }
      }

      for (int j = 0; j < 2; ++j) {
        for (int k = 0; k < 9; ++k) {
          appendFloat(data, 0.0f);
        }
      }
    }

    appendInt(data, static_cast<int32_t>(0xFF404040));

    patchChunkSize(data, headerPos);
  }

  void appendWorldInfoChunk(std::vector<uint8_t> &data, const std::vector<std::string> &nameTable) {
    uint32_t chunkId = findTOCId(nameTable, "WorldInfo");
    size_t headerPos = appendChunkHeader(data, chunkId, 1);

    std::vector<std::pair<std::string, DictValue>> pairs;
    pairs.push_back({"weather", DictValue::makeInt(0)});
    appendDict(data, nameTable, pairs);

    patchChunkSize(data, headerPos);
  }
  void appendSidesListChunk(std::vector<uint8_t> &data, const std::vector<std::string> &nameTable) {
    uint32_t sidesId = findTOCId(nameTable, "SidesList");
    uint32_t scriptsId = findTOCId(nameTable, "PlayerScriptsList");
    size_t headerPos = appendChunkHeader(data, sidesId, 3);

    int32_t numSides = 1;
    appendInt(data, numSides);

    std::vector<std::pair<std::string, DictValue>> sidePairs;
    sidePairs.push_back({"playerName", DictValue::makeString("TestPlayer")});
    appendDict(data, nameTable, sidePairs);

    int32_t buildListCount = 0;
    appendInt(data, buildListCount);

    int32_t numTeams = 0;
    appendInt(data, numTeams);

    size_t scriptsHeaderPos = appendChunkHeader(data, scriptsId, 1);

    int32_t numPlayers = 1;
    appendInt(data, numPlayers);

    int32_t numScripts = 0;
    appendInt(data, numScripts);

    patchChunkSize(data, scriptsHeaderPos);
    patchChunkSize(data, headerPos);
  }

  std::vector<std::string> fullNameTable() {
    return {"HeightMapData",   "BlendTileData",  "ObjectsList",      "Object",
            "PolygonTriggers", "GlobalLighting", "WorldInfo",        "SidesList",
            "weather",         "playerName",     "PlayerScriptsList"};
  }
};

TEST_F(MapLoaderTest, LoadsEmptyMapWithOnlyTOC) {
  std::vector<std::string> nameTable = {"HeightMapData"};
  auto data = createTOC(nameTable);

  auto mapFile = MapLoader::loadFromMemory(data.data(), data.size());
  ASSERT_TRUE(mapFile.has_value());
  EXPECT_FALSE(mapFile->hasHeightMap());
  EXPECT_FALSE(mapFile->hasBlendTiles());
  EXPECT_FALSE(mapFile->hasObjects());
  EXPECT_FALSE(mapFile->hasTriggers());
}

TEST_F(MapLoaderTest, LoadsHeightMapOnly) {
  auto nameTable = fullNameTable();
  auto data = createTOC(nameTable);

  appendHeightMapChunk(data, nameTable, 10, 10, 2, 100);

  auto mapFile = MapLoader::loadFromMemory(data.data(), data.size());
  ASSERT_TRUE(mapFile.has_value());

  EXPECT_TRUE(mapFile->hasHeightMap());
  EXPECT_EQ(mapFile->heightMap.width, 10);
  EXPECT_EQ(mapFile->heightMap.height, 10);
  EXPECT_EQ(mapFile->heightMap.borderSize, 2);
  EXPECT_EQ(mapFile->heightMap.data.size(), 100u);
  EXPECT_EQ(mapFile->heightMap.boundaries.size(), 1u);
  EXPECT_EQ(mapFile->heightMap.boundaries[0].x, 6);
  EXPECT_EQ(mapFile->heightMap.boundaries[0].y, 6);

  for (uint8_t h : mapFile->heightMap.data) {
    EXPECT_EQ(h, 100);
  }
}

TEST_F(MapLoaderTest, LoadsHeightMapAndBlendTiles) {
  auto nameTable = fullNameTable();
  auto data = createTOC(nameTable);

  int32_t w = 8, h = 8;
  appendHeightMapChunk(data, nameTable, w, h, 1);
  appendBlendTileChunk(data, nameTable, w, h);

  auto mapFile = MapLoader::loadFromMemory(data.data(), data.size());
  ASSERT_TRUE(mapFile.has_value());

  EXPECT_TRUE(mapFile->hasHeightMap());
  EXPECT_TRUE(mapFile->hasBlendTiles());
  EXPECT_EQ(mapFile->blendTiles.dataSize, w * h);
  EXPECT_EQ(mapFile->blendTiles.textureClasses.size(), 1u);
  EXPECT_EQ(mapFile->blendTiles.textureClasses[0].name, "TEDesert1");
}

TEST_F(MapLoaderTest, LoadsObjectsList) {
  auto nameTable = fullNameTable();
  auto data = createTOC(nameTable);

  appendHeightMapChunk(data, nameTable, 5, 5, 0);
  appendObjectsListChunk(data, nameTable, 3);

  auto mapFile = MapLoader::loadFromMemory(data.data(), data.size());
  ASSERT_TRUE(mapFile.has_value());

  EXPECT_TRUE(mapFile->hasObjects());
  ASSERT_EQ(mapFile->objects.size(), 3u);

  EXPECT_FLOAT_EQ(mapFile->objects[0].position.x, 100.0f);
  EXPECT_FLOAT_EQ(mapFile->objects[0].position.y, 200.0f);
  EXPECT_FLOAT_EQ(mapFile->objects[0].position.z, 10.0f);
  EXPECT_EQ(mapFile->objects[0].templateName, "Object0");

  EXPECT_FLOAT_EQ(mapFile->objects[1].position.x, 200.0f);
  EXPECT_FLOAT_EQ(mapFile->objects[1].position.y, 400.0f);
  EXPECT_EQ(mapFile->objects[1].templateName, "Object1");

  EXPECT_FLOAT_EQ(mapFile->objects[2].position.x, 300.0f);
  EXPECT_EQ(mapFile->objects[2].templateName, "Object2");
}

TEST_F(MapLoaderTest, LoadsPolygonTriggers) {
  auto nameTable = fullNameTable();
  auto data = createTOC(nameTable);

  appendHeightMapChunk(data, nameTable, 5, 5, 0);
  appendPolygonTriggersChunk(data, nameTable);

  auto mapFile = MapLoader::loadFromMemory(data.data(), data.size());
  ASSERT_TRUE(mapFile.has_value());

  EXPECT_TRUE(mapFile->hasTriggers());
  ASSERT_EQ(mapFile->triggers.size(), 2u);

  EXPECT_EQ(mapFile->triggers[0].name, "WaterArea1");
  EXPECT_EQ(mapFile->triggers[0].id, 1);
  EXPECT_TRUE(mapFile->triggers[0].isWaterArea);
  EXPECT_FALSE(mapFile->triggers[0].isRiver);
  EXPECT_EQ(mapFile->triggers[0].points.size(), 4u);

  EXPECT_EQ(mapFile->triggers[1].name, "TriggerZone1");
  EXPECT_EQ(mapFile->triggers[1].id, 2);
  EXPECT_FALSE(mapFile->triggers[1].isWaterArea);
  EXPECT_EQ(mapFile->triggers[1].points.size(), 3u);
}

TEST_F(MapLoaderTest, LoadsGlobalLighting) {
  auto nameTable = fullNameTable();
  auto data = createTOC(nameTable);

  appendHeightMapChunk(data, nameTable, 5, 5, 0);
  appendGlobalLightingChunk(data, nameTable);

  auto mapFile = MapLoader::loadFromMemory(data.data(), data.size());
  ASSERT_TRUE(mapFile.has_value());

  EXPECT_TRUE(mapFile->hasLighting());
  EXPECT_EQ(mapFile->lighting.currentTimeOfDay, TimeOfDay::Afternoon);
  EXPECT_EQ(mapFile->lighting.shadowColor, 0xFF404040u);

  const auto &afternoon = mapFile->lighting.getCurrentLighting();
  EXPECT_FLOAT_EQ(afternoon.terrainLights[0].ambient.x, 0.3f);
  EXPECT_FLOAT_EQ(afternoon.terrainLights[0].diffuse.x, 0.8f);
}

TEST_F(MapLoaderTest, LoadsWorldInfo) {
  auto nameTable = fullNameTable();
  auto data = createTOC(nameTable);

  appendHeightMapChunk(data, nameTable, 5, 5, 0);
  appendWorldInfoChunk(data, nameTable);

  auto mapFile = MapLoader::loadFromMemory(data.data(), data.size());
  ASSERT_TRUE(mapFile.has_value());

  EXPECT_TRUE(mapFile->worldInfo.isValid());
  EXPECT_EQ(mapFile->worldInfo.weather, Weather::Normal);
}

TEST_F(MapLoaderTest, LoadsSidesList) {
  auto nameTable = fullNameTable();
  auto data = createTOC(nameTable);

  appendHeightMapChunk(data, nameTable, 5, 5, 0);
  appendSidesListChunk(data, nameTable);

  auto mapFile = MapLoader::loadFromMemory(data.data(), data.size());
  ASSERT_TRUE(mapFile.has_value());

  EXPECT_TRUE(mapFile->sides.isValid());
  ASSERT_EQ(mapFile->sides.sides.size(), 1u);
  EXPECT_EQ(mapFile->sides.sides[0].name, "TestPlayer");
}

TEST_F(MapLoaderTest, LoadsFullMapFile) {
  auto nameTable = fullNameTable();
  auto data = createTOC(nameTable);

  int32_t w = 16, h = 16;
  appendHeightMapChunk(data, nameTable, w, h, 2, 64);
  appendBlendTileChunk(data, nameTable, w, h);
  appendWorldInfoChunk(data, nameTable);
  appendSidesListChunk(data, nameTable);
  appendObjectsListChunk(data, nameTable, 5);
  appendPolygonTriggersChunk(data, nameTable);
  appendGlobalLightingChunk(data, nameTable);

  auto mapFile = MapLoader::loadFromMemory(data.data(), data.size());
  ASSERT_TRUE(mapFile.has_value());

  EXPECT_TRUE(mapFile->hasHeightMap());
  EXPECT_EQ(mapFile->heightMap.width, w);
  EXPECT_EQ(mapFile->heightMap.height, h);

  EXPECT_TRUE(mapFile->hasBlendTiles());
  EXPECT_EQ(mapFile->blendTiles.dataSize, w * h);

  EXPECT_TRUE(mapFile->worldInfo.isValid());
  EXPECT_TRUE(mapFile->sides.isValid());

  EXPECT_TRUE(mapFile->hasObjects());
  EXPECT_EQ(mapFile->objects.size(), 5u);

  EXPECT_TRUE(mapFile->hasTriggers());
  EXPECT_EQ(mapFile->triggers.size(), 2u);

  EXPECT_TRUE(mapFile->hasLighting());
  EXPECT_EQ(mapFile->lighting.currentTimeOfDay, TimeOfDay::Afternoon);
}

TEST_F(MapLoaderTest, FailsOnInvalidMagic) {
  std::vector<uint8_t> data = {'B', 'A', 'D', '!', 0, 0, 0, 0};

  std::string error;
  auto mapFile = MapLoader::loadFromMemory(data.data(), data.size(), &error);
  EXPECT_FALSE(mapFile.has_value());
  EXPECT_FALSE(error.empty());
}

TEST_F(MapLoaderTest, FailsOnEmptyData) {
  std::string error;
  auto mapFile = MapLoader::loadFromMemory(nullptr, 0, &error);
  EXPECT_FALSE(mapFile.has_value());
  EXPECT_FALSE(error.empty());
}

TEST_F(MapLoaderTest, FailsOnTruncatedTOC) {
  std::vector<uint8_t> data = {'C', 'k', 'M', 'p'};

  std::string error;
  auto mapFile = MapLoader::loadFromMemory(data.data(), data.size(), &error);
  EXPECT_FALSE(mapFile.has_value());
  EXPECT_FALSE(error.empty());
}

TEST_F(MapLoaderTest, FailsOnBlendTileBeforeHeightMap) {
  auto nameTable = fullNameTable();
  auto data = createTOC(nameTable);

  uint32_t chunkId = findTOCId(nameTable, "BlendTileData");
  size_t headerPos = appendChunkHeader(data, chunkId, 8);
  appendInt(data, 4);
  for (int i = 0; i < 4 * 4; ++i) {
    appendShort(data, 0);
  }
  appendInt(data, 0);
  appendInt(data, 0);
  patchChunkSize(data, headerPos);

  std::string error;
  auto mapFile = MapLoader::loadFromMemory(data.data(), data.size(), &error);
  EXPECT_FALSE(mapFile.has_value());
  EXPECT_NE(error.find("BlendTileData"), std::string::npos);
}

TEST_F(MapLoaderTest, SkipsUnknownChunks) {
  auto nameTable = fullNameTable();
  nameTable.push_back("UnknownChunk");
  auto data = createTOC(nameTable);

  appendHeightMapChunk(data, nameTable, 5, 5, 0);

  uint32_t unknownId = findTOCId(nameTable, "UnknownChunk");
  size_t unknownHeaderPos = appendChunkHeader(data, unknownId, 1);
  appendInt(data, 42);
  appendInt(data, 99);
  patchChunkSize(data, unknownHeaderPos);

  appendObjectsListChunk(data, nameTable, 1);

  auto mapFile = MapLoader::loadFromMemory(data.data(), data.size());
  ASSERT_TRUE(mapFile.has_value());

  EXPECT_TRUE(mapFile->hasHeightMap());
  EXPECT_TRUE(mapFile->hasObjects());
  EXPECT_EQ(mapFile->objects.size(), 1u);
}

TEST_F(MapLoaderTest, DescribeProducesNonEmptyOutput) {
  auto nameTable = fullNameTable();
  auto data = createTOC(nameTable);

  int32_t w = 8, h = 8;
  appendHeightMapChunk(data, nameTable, w, h, 1, 64);
  appendBlendTileChunk(data, nameTable, w, h);
  appendObjectsListChunk(data, nameTable, 2);
  appendPolygonTriggersChunk(data, nameTable);
  appendGlobalLightingChunk(data, nameTable);

  auto mapFile = MapLoader::loadFromMemory(data.data(), data.size());
  ASSERT_TRUE(mapFile.has_value());

  std::string description = mapFile->describe();
  EXPECT_FALSE(description.empty());
  EXPECT_NE(description.find("HeightMap"), std::string::npos);
  EXPECT_NE(description.find("8 x 8"), std::string::npos);
  EXPECT_NE(description.find("BlendTileData"), std::string::npos);
  EXPECT_NE(description.find("TEDesert1"), std::string::npos);
  EXPECT_NE(description.find("Objects"), std::string::npos);
  EXPECT_NE(description.find("Polygon Triggers"), std::string::npos);
  EXPECT_NE(description.find("Global Lighting"), std::string::npos);
}

TEST_F(MapLoaderTest, DescribeHandlesMinimalMap) {
  auto nameTable = fullNameTable();
  auto data = createTOC(nameTable);

  auto mapFile = MapLoader::loadFromMemory(data.data(), data.size());
  ASSERT_TRUE(mapFile.has_value());

  std::string description = mapFile->describe();
  EXPECT_FALSE(description.empty());
  EXPECT_NE(description.find("Map File Contents"), std::string::npos);
}

TEST_F(MapLoaderTest, HeightMapWorldHeightAccessors) {
  auto nameTable = fullNameTable();
  auto data = createTOC(nameTable);

  appendHeightMapChunk(data, nameTable, 4, 4, 0, 200);

  auto mapFile = MapLoader::loadFromMemory(data.data(), data.size());
  ASSERT_TRUE(mapFile.has_value());

  float expectedWorldHeight = 200.0f * MAP_HEIGHT_SCALE;
  EXPECT_FLOAT_EQ(mapFile->heightMap.getWorldHeight(0, 0), expectedWorldHeight);
  EXPECT_FLOAT_EQ(mapFile->heightMap.getWorldHeight(3, 3), expectedWorldHeight);

  EXPECT_FLOAT_EQ(mapFile->heightMap.getWorldHeight(-1, 0), 0.0f);
  EXPECT_FLOAT_EQ(mapFile->heightMap.getWorldHeight(4, 0), 0.0f);
}

TEST_F(MapLoaderTest, MapFileSourcePathNotSetForMemoryLoad) {
  auto nameTable = fullNameTable();
  auto data = createTOC(nameTable);

  auto mapFile = MapLoader::loadFromMemory(data.data(), data.size());
  ASSERT_TRUE(mapFile.has_value());
  EXPECT_TRUE(mapFile->sourcePath.empty());
}

TEST_F(MapLoaderTest, LoadFromFileFailsForNonexistentFile) {
  std::string error;
  auto mapFile = MapLoader::load("nonexistent_file.map", &error);
  EXPECT_FALSE(mapFile.has_value());
  EXPECT_FALSE(error.empty());
}
