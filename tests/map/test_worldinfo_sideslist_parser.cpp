#include <vector>

#include "../../src/lib/formats/map/data_chunk_reader.hpp"
#include "../../src/lib/formats/map/sideslist_parser.hpp"
#include "../../src/lib/formats/map/types.hpp"
#include "../../src/lib/formats/map/worldinfo_parser.hpp"

#include <gtest/gtest.h>

using namespace map;

class WorldInfoSidesListTest : public ::testing::Test {
protected:
  std::vector<uint8_t> createTOC(const std::vector<std::string> &names) {
    std::vector<uint8_t> data;

    data.push_back('C');
    data.push_back('k');
    data.push_back('M');
    data.push_back('p');

    int32_t count = static_cast<int32_t>(names.size());
    data.push_back(count & 0xFF);
    data.push_back((count >> 8) & 0xFF);
    data.push_back((count >> 16) & 0xFF);
    data.push_back((count >> 24) & 0xFF);

    for (uint32_t i = 0; i < names.size(); ++i) {
      const auto &name = names[i];
      uint8_t len = static_cast<uint8_t>(name.size());
      data.push_back(len);
      for (char c : name) {
        data.push_back(static_cast<uint8_t>(c));
      }

      uint32_t id = i + 1;
      data.push_back(id & 0xFF);
      data.push_back((id >> 8) & 0xFF);
      data.push_back((id >> 16) & 0xFF);
      data.push_back((id >> 24) & 0xFF);
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
    memcpy(&bits, &value, sizeof(float));
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

  void appendChunkHeader(std::vector<uint8_t> &data, uint32_t id, uint16_t version,
                         int32_t dataSize) {
    appendInt(data, id);
    appendShort(data, version);
    appendInt(data, dataSize);
  }

  void appendDict(std::vector<uint8_t> &data, const std::vector<std::string> &nameTable,
                  const std::vector<std::pair<std::string, DictValue>> &pairs) {
    uint16_t pairCount = static_cast<uint16_t>(pairs.size());
    appendShort(data, pairCount);

    for (const auto &[key, value] : pairs) {
      uint32_t keyId = 0;
      for (size_t i = 0; i < nameTable.size(); ++i) {
        if (nameTable[i] == key) {
          keyId = i + 1;
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
};

TEST_F(WorldInfoSidesListTest, ParsesWorldInfoVersion1) {
  std::vector<std::string> nameTable = {"WorldInfo", "weather", "mapName"};
  auto data = createTOC(nameTable);

  size_t chunkStartPos = data.size();
  appendChunkHeader(data, 1, K_WORLDDICT_VERSION_1, 0);

  std::vector<std::pair<std::string, DictValue>> dictPairs;
  dictPairs.push_back({"weather", DictValue::makeInt(1)});
  dictPairs.push_back({"mapName", DictValue::makeString("TestMap")});
  appendDict(data, nameTable, dictPairs);

  int32_t chunkDataSize = static_cast<int32_t>(data.size() - chunkStartPos - CHUNK_HEADER_SIZE);
  int32_t *sizePtr = reinterpret_cast<int32_t *>(&data[chunkStartPos + 4 + 2]);
  *sizePtr = chunkDataSize;

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << "Failed to load TOC: " << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());
  EXPECT_EQ(header->version, K_WORLDDICT_VERSION_1);

  auto worldInfo = WorldInfoParser::parse(reader, header->version);
  ASSERT_TRUE(worldInfo.has_value()) << "Failed to parse WorldInfo";
  EXPECT_TRUE(worldInfo->isValid());
  EXPECT_EQ(worldInfo->weather, Weather::Snowy);
  EXPECT_EQ(worldInfo->properties.size(), 2);

  auto weatherIt = worldInfo->properties.find("weather");
  ASSERT_NE(weatherIt, worldInfo->properties.end());
  EXPECT_EQ(weatherIt->second.type, DataType::Int);
  EXPECT_EQ(weatherIt->second.intValue, 1);

  auto nameIt = worldInfo->properties.find("mapName");
  ASSERT_NE(nameIt, worldInfo->properties.end());
  EXPECT_EQ(nameIt->second.type, DataType::AsciiString);
  EXPECT_EQ(nameIt->second.stringValue, "TestMap");
}

TEST_F(WorldInfoSidesListTest, ParsesEmptyWorldInfo) {
  std::vector<std::string> nameTable = {"WorldInfo"};
  auto data = createTOC(nameTable);

  size_t chunkStartPos = data.size();
  appendChunkHeader(data, 1, K_WORLDDICT_VERSION_1, 0);

  std::vector<std::pair<std::string, DictValue>> dictPairs;
  appendDict(data, nameTable, dictPairs);

  int32_t chunkDataSize = static_cast<int32_t>(data.size() - chunkStartPos - CHUNK_HEADER_SIZE);
  int32_t *sizePtr = reinterpret_cast<int32_t *>(&data[chunkStartPos + 4 + 2]);
  *sizePtr = chunkDataSize;

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << "Failed to load TOC: " << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());

  auto worldInfo = WorldInfoParser::parse(reader, header->version);
  ASSERT_TRUE(worldInfo.has_value()) << "Failed to parse WorldInfo";
  EXPECT_TRUE(worldInfo->isValid());
  EXPECT_EQ(worldInfo->weather, Weather::Normal);
  EXPECT_TRUE(worldInfo->properties.empty());
}

TEST_F(WorldInfoSidesListTest, ParsesSidesListVersion3) {
  std::vector<std::string> nameTable = {"SidesList",         "playerName",   "teamName",
                                        "PlayerScriptsList", "playerAllies", "playerEnemies"};
  auto data = createTOC(nameTable);

  size_t chunkStartPos = data.size();
  appendChunkHeader(data, 1, K_SIDES_DATA_VERSION_3, 0);

  appendInt(data, 2);

  std::vector<std::pair<std::string, DictValue>> side1Pairs;
  side1Pairs.push_back({"playerName", DictValue::makeString("Player1")});
  side1Pairs.push_back({"playerAllies", DictValue::makeString("skirmishTeam0")});
  appendDict(data, nameTable, side1Pairs);

  appendInt(data, 1);
  appendString(data, "Command Center 1");
  appendString(data, "AmericaCommandCenter");
  appendFloat(data, 100.0f);
  appendFloat(data, 200.0f);
  appendFloat(data, 0.0f);
  appendFloat(data, 0.0f);
  data.push_back(1);
  appendInt(data, 0);
  appendString(data, "");
  appendInt(data, 100);
  data.push_back(0);
  data.push_back(0);
  data.push_back(1);

  std::vector<std::pair<std::string, DictValue>> side2Pairs;
  side2Pairs.push_back({"playerName", DictValue::makeString("Player2")});
  side2Pairs.push_back({"playerEnemies", DictValue::makeString("skirmishTeam0")});
  appendDict(data, nameTable, side2Pairs);

  appendInt(data, 0);

  appendInt(data, 1);
  std::vector<std::pair<std::string, DictValue>> team1Pairs;
  team1Pairs.push_back({"teamName", DictValue::makeString("skirmishTeam0")});
  appendDict(data, nameTable, team1Pairs);

  size_t playerScriptsStartPos = data.size();
  appendChunkHeader(data, 4, 1, 0);

  appendInt(data, 2);
  appendInt(data, 1);
  appendString(data, "InitialCameraPosition");
  appendString(data, "CameraPosition 100 200 300");

  appendInt(data, 0);

  int32_t playerScriptsDataSize =
      static_cast<int32_t>(data.size() - playerScriptsStartPos - CHUNK_HEADER_SIZE);
  int32_t *playerScriptsSizePtr = reinterpret_cast<int32_t *>(&data[playerScriptsStartPos + 4 + 2]);
  *playerScriptsSizePtr = playerScriptsDataSize;

  int32_t chunkDataSize = static_cast<int32_t>(data.size() - chunkStartPos - CHUNK_HEADER_SIZE);
  int32_t *sizePtr = reinterpret_cast<int32_t *>(&data[chunkStartPos + 4 + 2]);
  *sizePtr = chunkDataSize;

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << "Failed to load TOC: " << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());
  EXPECT_EQ(header->version, K_SIDES_DATA_VERSION_3);

  auto sidesList = SidesListParser::parse(reader, header->version);
  ASSERT_TRUE(sidesList.has_value()) << "Failed to parse SidesList";
  EXPECT_TRUE(sidesList->isValid());

  ASSERT_EQ(sidesList->sides.size(), 2);
  EXPECT_EQ(sidesList->sides[0].name, "Player1");
  EXPECT_EQ(sidesList->sides[0].buildList.size(), 1);
  EXPECT_EQ(sidesList->sides[0].buildList[0].buildingName, "Command Center 1");
  EXPECT_EQ(sidesList->sides[0].buildList[0].templateName, "AmericaCommandCenter");
  EXPECT_FLOAT_EQ(sidesList->sides[0].buildList[0].location.x, 100.0f);
  EXPECT_FLOAT_EQ(sidesList->sides[0].buildList[0].location.y, 200.0f);
  EXPECT_FLOAT_EQ(sidesList->sides[0].buildList[0].location.z, 0.0f);
  EXPECT_TRUE(sidesList->sides[0].buildList[0].initiallyBuilt);
  EXPECT_EQ(sidesList->sides[0].buildList[0].health, 100);
  EXPECT_TRUE(sidesList->sides[0].buildList[0].isRepairable);

  EXPECT_EQ(sidesList->sides[1].name, "Player2");
  EXPECT_EQ(sidesList->sides[1].buildList.size(), 0);

  ASSERT_EQ(sidesList->teams.size(), 1);
  EXPECT_EQ(sidesList->teams[0].name, "skirmishTeam0");

  ASSERT_EQ(sidesList->playerScripts.size(), 1);
  EXPECT_EQ(sidesList->playerScripts[0].name, "InitialCameraPosition");
  EXPECT_EQ(sidesList->playerScripts[0].script, "CameraPosition 100 200 300");
}

TEST_F(WorldInfoSidesListTest, ParsesEmptySidesList) {
  std::vector<std::string> nameTable = {"SidesList", "PlayerScriptsList"};
  auto data = createTOC(nameTable);

  size_t chunkStartPos = data.size();
  appendChunkHeader(data, 1, K_SIDES_DATA_VERSION_3, 0);

  appendInt(data, 0);
  appendInt(data, 0);

  size_t playerScriptsStartPos = data.size();
  appendChunkHeader(data, 2, 1, 0);
  appendInt(data, 0);

  int32_t playerScriptsDataSize =
      static_cast<int32_t>(data.size() - playerScriptsStartPos - CHUNK_HEADER_SIZE);
  int32_t *playerScriptsSizePtr = reinterpret_cast<int32_t *>(&data[playerScriptsStartPos + 4 + 2]);
  *playerScriptsSizePtr = playerScriptsDataSize;

  int32_t chunkDataSize = static_cast<int32_t>(data.size() - chunkStartPos - CHUNK_HEADER_SIZE);
  int32_t *sizePtr = reinterpret_cast<int32_t *>(&data[chunkStartPos + 4 + 2]);
  *sizePtr = chunkDataSize;

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << "Failed to load TOC: " << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());

  auto sidesList = SidesListParser::parse(reader, header->version);
  ASSERT_TRUE(sidesList.has_value()) << "Failed to parse SidesList";
  EXPECT_TRUE(sidesList->isValid());
  EXPECT_TRUE(sidesList->sides.empty());
  EXPECT_TRUE(sidesList->teams.empty());
  EXPECT_TRUE(sidesList->playerScripts.empty());
}
