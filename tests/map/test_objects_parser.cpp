#include <vector>

#include "../../src/lib/formats/map/data_chunk_reader.hpp"
#include "../../src/lib/formats/map/objects_parser.hpp"
#include "../../src/lib/formats/map/types.hpp"

#include <gtest/gtest.h>

using namespace map;

class ObjectsParserTest : public ::testing::Test {
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

TEST_F(ObjectsParserTest, ParsesVersion1Object) {
  std::vector<std::string> nameTable = {"ObjectsList", "Object"};
  auto data = createTOC(nameTable);

  size_t objectsListStartPos = data.size();
  appendChunkHeader(data, 1, K_OBJECTS_VERSION_1, 0);

  size_t objectStartPos = data.size();
  appendChunkHeader(data, 2, K_OBJECTS_VERSION_1, 0);

  appendFloat(data, 100.0f);
  appendFloat(data, 200.0f);
  appendFloat(data, 0.5f);
  appendInt(data, 0x001);
  appendString(data, "TestObject");

  int32_t objectDataSize = static_cast<int32_t>(data.size() - objectStartPos - CHUNK_HEADER_SIZE);
  int32_t *objectSizePtr = reinterpret_cast<int32_t *>(&data[objectStartPos + 4 + 2]);
  *objectSizePtr = objectDataSize;

  int32_t objectsListDataSize =
      static_cast<int32_t>(data.size() - objectsListStartPos - CHUNK_HEADER_SIZE);
  int32_t *objectsListSizePtr = reinterpret_cast<int32_t *>(&data[objectsListStartPos + 4 + 2]);
  *objectsListSizePtr = objectsListDataSize;

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << "Failed to load TOC: " << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());
  EXPECT_EQ(header->version, K_OBJECTS_VERSION_1);

  auto objects = ObjectsParser::parse(reader, header->version);
  ASSERT_TRUE(objects.has_value()) << "Failed to parse objects";
  ASSERT_EQ(objects->size(), 1);

  const auto &obj = (*objects)[0];
  EXPECT_FLOAT_EQ(obj.position.x, 100.0f);
  EXPECT_FLOAT_EQ(obj.position.y, 200.0f);
  EXPECT_FLOAT_EQ(obj.position.z, 0.0f);
  EXPECT_FLOAT_EQ(obj.angle, 0.5f);
  EXPECT_EQ(obj.flags, 0x001u);
  EXPECT_EQ(obj.templateName, "TestObject");
  EXPECT_TRUE(obj.properties.empty());
}

TEST_F(ObjectsParserTest, ParsesVersion2ObjectWithDict) {
  std::vector<std::string> nameTable = {"ObjectsList", "Object", "originalOwner", "uniqueID"};
  auto data = createTOC(nameTable);

  size_t objectsListStartPos = data.size();
  appendChunkHeader(data, 1, K_OBJECTS_VERSION_2, 0);

  size_t objectStartPos = data.size();
  appendChunkHeader(data, 2, K_OBJECTS_VERSION_2, 0);

  appendFloat(data, 150.0f);
  appendFloat(data, 250.0f);
  appendFloat(data, 1.0f);
  appendInt(data, 0x002);
  appendString(data, "Building");

  std::vector<std::pair<std::string, DictValue>> dictPairs;
  dictPairs.push_back({"originalOwner", DictValue::makeString("Player1")});
  dictPairs.push_back({"uniqueID", DictValue::makeString("Building 1")});
  appendDict(data, nameTable, dictPairs);

  int32_t objectDataSize = static_cast<int32_t>(data.size() - objectStartPos - CHUNK_HEADER_SIZE);
  int32_t *objectSizePtr = reinterpret_cast<int32_t *>(&data[objectStartPos + 4 + 2]);
  *objectSizePtr = objectDataSize;

  int32_t objectsListDataSize =
      static_cast<int32_t>(data.size() - objectsListStartPos - CHUNK_HEADER_SIZE);
  int32_t *objectsListSizePtr = reinterpret_cast<int32_t *>(&data[objectsListStartPos + 4 + 2]);
  *objectsListSizePtr = objectsListDataSize;

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << "Failed to load TOC: " << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());
  EXPECT_EQ(header->version, K_OBJECTS_VERSION_2);

  auto objects = ObjectsParser::parse(reader, header->version);
  ASSERT_TRUE(objects.has_value()) << "Failed to parse objects";
  ASSERT_EQ(objects->size(), 1);

  const auto &obj = (*objects)[0];
  EXPECT_FLOAT_EQ(obj.position.x, 150.0f);
  EXPECT_FLOAT_EQ(obj.position.y, 250.0f);
  EXPECT_FLOAT_EQ(obj.position.z, 0.0f);
  EXPECT_FLOAT_EQ(obj.angle, 1.0f);
  EXPECT_EQ(obj.flags, 0x002u);
  EXPECT_EQ(obj.templateName, "Building");
  EXPECT_EQ(obj.properties.size(), 2);

  auto ownerIt = obj.properties.find("originalOwner");
  ASSERT_NE(ownerIt, obj.properties.end());
  EXPECT_EQ(ownerIt->second.type, DataType::AsciiString);
  EXPECT_EQ(ownerIt->second.stringValue, "Player1");

  auto idIt = obj.properties.find("uniqueID");
  ASSERT_NE(idIt, obj.properties.end());
  EXPECT_EQ(idIt->second.type, DataType::AsciiString);
  EXPECT_EQ(idIt->second.stringValue, "Building 1");
}

TEST_F(ObjectsParserTest, ParsesVersion3ObjectWithZ) {
  std::vector<std::string> nameTable = {"ObjectsList", "Object", "objectInitialHealth"};
  auto data = createTOC(nameTable);

  size_t objectsListStartPos = data.size();
  appendChunkHeader(data, 1, K_OBJECTS_VERSION_3, 0);

  size_t objectStartPos = data.size();
  appendChunkHeader(data, 2, K_OBJECTS_VERSION_3, 0);

  appendFloat(data, 300.0f);
  appendFloat(data, 400.0f);
  appendFloat(data, 50.0f);
  appendFloat(data, 3.14f);
  appendInt(data, FLAG_DONT_RENDER);
  appendString(data, "Vehicle");

  std::vector<std::pair<std::string, DictValue>> dictPairs;
  dictPairs.push_back({"objectInitialHealth", DictValue::makeInt(75)});
  appendDict(data, nameTable, dictPairs);

  int32_t objectDataSize = static_cast<int32_t>(data.size() - objectStartPos - CHUNK_HEADER_SIZE);
  int32_t *objectSizePtr = reinterpret_cast<int32_t *>(&data[objectStartPos + 4 + 2]);
  *objectSizePtr = objectDataSize;

  int32_t objectsListDataSize =
      static_cast<int32_t>(data.size() - objectsListStartPos - CHUNK_HEADER_SIZE);
  int32_t *objectsListSizePtr = reinterpret_cast<int32_t *>(&data[objectsListStartPos + 4 + 2]);
  *objectsListSizePtr = objectsListDataSize;

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << "Failed to load TOC: " << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());
  EXPECT_EQ(header->version, K_OBJECTS_VERSION_3);

  auto objects = ObjectsParser::parse(reader, header->version);
  ASSERT_TRUE(objects.has_value()) << "Failed to parse objects";
  ASSERT_EQ(objects->size(), 1);

  const auto &obj = (*objects)[0];
  EXPECT_FLOAT_EQ(obj.position.x, 300.0f);
  EXPECT_FLOAT_EQ(obj.position.y, 400.0f);
  EXPECT_FLOAT_EQ(obj.position.z, 50.0f);
  EXPECT_FLOAT_EQ(obj.angle, 3.14f);
  EXPECT_EQ(obj.flags, FLAG_DONT_RENDER);
  EXPECT_EQ(obj.templateName, "Vehicle");
  EXPECT_FALSE(obj.shouldRender());
  EXPECT_EQ(obj.properties.size(), 1);

  auto healthIt = obj.properties.find("objectInitialHealth");
  ASSERT_NE(healthIt, obj.properties.end());
  EXPECT_EQ(healthIt->second.type, DataType::Int);
  EXPECT_EQ(healthIt->second.intValue, 75);
}

TEST_F(ObjectsParserTest, ParsesMultipleObjects) {
  std::vector<std::string> nameTable = {"ObjectsList", "Object"};
  auto data = createTOC(nameTable);

  size_t objectsListStartPos = data.size();
  appendChunkHeader(data, 1, K_OBJECTS_VERSION_1, 0);

  for (int i = 0; i < 3; ++i) {
    size_t objectStartPos = data.size();
    appendChunkHeader(data, 2, K_OBJECTS_VERSION_1, 0);

    appendFloat(data, 100.0f * (i + 1));
    appendFloat(data, 200.0f * (i + 1));
    appendFloat(data, 0.5f * (i + 1));
    appendInt(data, i);
    appendString(data, "Object" + std::to_string(i));

    int32_t objectDataSize = static_cast<int32_t>(data.size() - objectStartPos - CHUNK_HEADER_SIZE);
    int32_t *objectSizePtr = reinterpret_cast<int32_t *>(&data[objectStartPos + 4 + 2]);
    *objectSizePtr = objectDataSize;
  }

  int32_t objectsListDataSize =
      static_cast<int32_t>(data.size() - objectsListStartPos - CHUNK_HEADER_SIZE);
  int32_t *objectsListSizePtr = reinterpret_cast<int32_t *>(&data[objectsListStartPos + 4 + 2]);
  *objectsListSizePtr = objectsListDataSize;

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << "Failed to load TOC: " << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());

  auto objects = ObjectsParser::parse(reader, header->version);
  ASSERT_TRUE(objects.has_value()) << "Failed to parse objects";
  ASSERT_EQ(objects->size(), 3);

  for (int i = 0; i < 3; ++i) {
    const auto &obj = (*objects)[i];
    EXPECT_FLOAT_EQ(obj.position.x, 100.0f * (i + 1));
    EXPECT_FLOAT_EQ(obj.position.y, 200.0f * (i + 1));
    EXPECT_FLOAT_EQ(obj.angle, 0.5f * (i + 1));
    EXPECT_EQ(obj.flags, static_cast<uint32_t>(i));
    EXPECT_EQ(obj.templateName, "Object" + std::to_string(i));
  }
}

TEST_F(ObjectsParserTest, HandlesInvalidVersion) {
  std::vector<std::string> nameTable = {"ObjectsList"};
  auto data = createTOC(nameTable);

  size_t objectsListStartPos = data.size();
  appendChunkHeader(data, 1, 99, 0);

  int32_t objectsListDataSize =
      static_cast<int32_t>(data.size() - objectsListStartPos - CHUNK_HEADER_SIZE);
  int32_t *objectsListSizePtr = reinterpret_cast<int32_t *>(&data[objectsListStartPos + 4 + 2]);
  *objectsListSizePtr = objectsListDataSize;

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << "Failed to load TOC: " << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());

  std::string parseError;
  auto objects = ObjectsParser::parse(reader, header->version, &parseError);
  EXPECT_FALSE(objects.has_value());
  EXPECT_FALSE(parseError.empty());
}

TEST_F(ObjectsParserTest, TestsObjectFlagHelpers) {
  MapObject obj;

  obj.flags = FLAG_ROAD_POINT1;
  EXPECT_TRUE(obj.isRoadPoint());
  EXPECT_FALSE(obj.isBridgePoint());
  EXPECT_TRUE(obj.shouldRender());

  obj.flags = FLAG_BRIDGE_POINT2;
  EXPECT_FALSE(obj.isRoadPoint());
  EXPECT_TRUE(obj.isBridgePoint());
  EXPECT_TRUE(obj.shouldRender());

  obj.flags = FLAG_DONT_RENDER;
  EXPECT_FALSE(obj.isRoadPoint());
  EXPECT_FALSE(obj.isBridgePoint());
  EXPECT_FALSE(obj.shouldRender());

  obj.flags = FLAG_ROAD_POINT1 | FLAG_DONT_RENDER;
  EXPECT_TRUE(obj.isRoadPoint());
  EXPECT_FALSE(obj.shouldRender());
}
