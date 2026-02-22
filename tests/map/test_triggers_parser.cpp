#include <vector>

#include "../../src/lib/formats/map/data_chunk_reader.hpp"
#include "../../src/lib/formats/map/triggers_parser.hpp"
#include "../../src/lib/formats/map/types.hpp"

#include <gtest/gtest.h>

using namespace map;

class TriggersParserTest : public ::testing::Test {
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

  void appendByte(std::vector<uint8_t> &data, uint8_t value) { data.push_back(value); }

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
};

TEST_F(TriggersParserTest, ParsesVersion1BasicTrigger) {
  std::vector<std::string> nameTable = {"PolygonTriggers"};
  auto data = createTOC(nameTable);

  size_t triggersStartPos = data.size();
  appendChunkHeader(data, 1, K_TRIGGERS_VERSION_1, 0);

  appendInt(data, 1);

  appendString(data, "TriggerArea1");
  appendInt(data, 100);

  appendInt(data, 4);
  appendInt(data, 0);
  appendInt(data, 0);
  appendInt(data, 0);

  appendInt(data, 100);
  appendInt(data, 0);
  appendInt(data, 0);

  appendInt(data, 100);
  appendInt(data, 100);
  appendInt(data, 0);

  appendInt(data, 0);
  appendInt(data, 100);
  appendInt(data, 0);

  int32_t triggersDataSize =
      static_cast<int32_t>(data.size() - triggersStartPos - CHUNK_HEADER_SIZE);
  int32_t *triggersSizePtr = reinterpret_cast<int32_t *>(&data[triggersStartPos + 4 + 2]);
  *triggersSizePtr = triggersDataSize;

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << "Failed to load TOC: " << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());
  EXPECT_EQ(header->version, K_TRIGGERS_VERSION_1);

  auto triggers = TriggersParser::parse(reader, header->version);
  ASSERT_TRUE(triggers.has_value()) << "Failed to parse triggers";
  ASSERT_EQ(triggers->size(), 1);

  const auto &trigger = (*triggers)[0];
  EXPECT_EQ(trigger.name, "TriggerArea1");
  EXPECT_EQ(trigger.id, 100);
  EXPECT_FALSE(trigger.isWaterArea);
  EXPECT_FALSE(trigger.isRiver);
  EXPECT_EQ(trigger.riverStart, 0);
  ASSERT_EQ(trigger.points.size(), 4);

  EXPECT_EQ(trigger.points[0].x, 0);
  EXPECT_EQ(trigger.points[0].y, 0);
  EXPECT_EQ(trigger.points[0].z, 0);

  EXPECT_EQ(trigger.points[1].x, 100);
  EXPECT_EQ(trigger.points[1].y, 0);
  EXPECT_EQ(trigger.points[1].z, 0);

  EXPECT_EQ(trigger.points[2].x, 100);
  EXPECT_EQ(trigger.points[2].y, 100);
  EXPECT_EQ(trigger.points[2].z, 0);

  EXPECT_EQ(trigger.points[3].x, 0);
  EXPECT_EQ(trigger.points[3].y, 100);
  EXPECT_EQ(trigger.points[3].z, 0);

  EXPECT_TRUE(trigger.isValid());
}

TEST_F(TriggersParserTest, ParsesVersion2WaterArea) {
  std::vector<std::string> nameTable = {"PolygonTriggers"};
  auto data = createTOC(nameTable);

  size_t triggersStartPos = data.size();
  appendChunkHeader(data, 1, K_TRIGGERS_VERSION_2, 0);

  appendInt(data, 1);

  appendString(data, "WaterArea1");
  appendInt(data, 200);
  appendByte(data, 1);

  appendInt(data, 3);
  appendInt(data, 50);
  appendInt(data, 50);
  appendInt(data, 10);

  appendInt(data, 150);
  appendInt(data, 50);
  appendInt(data, 10);

  appendInt(data, 100);
  appendInt(data, 150);
  appendInt(data, 10);

  int32_t triggersDataSize =
      static_cast<int32_t>(data.size() - triggersStartPos - CHUNK_HEADER_SIZE);
  int32_t *triggersSizePtr = reinterpret_cast<int32_t *>(&data[triggersStartPos + 4 + 2]);
  *triggersSizePtr = triggersDataSize;

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << "Failed to load TOC: " << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());
  EXPECT_EQ(header->version, K_TRIGGERS_VERSION_2);

  auto triggers = TriggersParser::parse(reader, header->version);
  ASSERT_TRUE(triggers.has_value()) << "Failed to parse triggers";
  ASSERT_EQ(triggers->size(), 1);

  const auto &trigger = (*triggers)[0];
  EXPECT_EQ(trigger.name, "WaterArea1");
  EXPECT_EQ(trigger.id, 200);
  EXPECT_TRUE(trigger.isWaterArea);
  EXPECT_FALSE(trigger.isRiver);
  EXPECT_EQ(trigger.riverStart, 0);
  ASSERT_EQ(trigger.points.size(), 3);

  EXPECT_EQ(trigger.points[0].z, 10);
  EXPECT_EQ(trigger.points[1].z, 10);
  EXPECT_EQ(trigger.points[2].z, 10);
}

TEST_F(TriggersParserTest, ParsesVersion3River) {
  std::vector<std::string> nameTable = {"PolygonTriggers"};
  auto data = createTOC(nameTable);

  size_t triggersStartPos = data.size();
  appendChunkHeader(data, 1, K_TRIGGERS_VERSION_3, 0);

  appendInt(data, 1);

  appendString(data, "River1");
  appendInt(data, 300);
  appendByte(data, 1);
  appendByte(data, 1);
  appendInt(data, 2);

  appendInt(data, 5);
  for (int i = 0; i < 5; ++i) {
    appendInt(data, i * 10);
    appendInt(data, i * 20);
    appendInt(data, 15);
  }

  int32_t triggersDataSize =
      static_cast<int32_t>(data.size() - triggersStartPos - CHUNK_HEADER_SIZE);
  int32_t *triggersSizePtr = reinterpret_cast<int32_t *>(&data[triggersStartPos + 4 + 2]);
  *triggersSizePtr = triggersDataSize;

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << "Failed to load TOC: " << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());
  EXPECT_EQ(header->version, K_TRIGGERS_VERSION_3);

  auto triggers = TriggersParser::parse(reader, header->version);
  ASSERT_TRUE(triggers.has_value()) << "Failed to parse triggers";
  ASSERT_EQ(triggers->size(), 1);

  const auto &trigger = (*triggers)[0];
  EXPECT_EQ(trigger.name, "River1");
  EXPECT_EQ(trigger.id, 300);
  EXPECT_TRUE(trigger.isWaterArea);
  EXPECT_TRUE(trigger.isRiver);
  EXPECT_EQ(trigger.riverStart, 2);
  ASSERT_EQ(trigger.points.size(), 5);

  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ(trigger.points[i].x, i * 10);
    EXPECT_EQ(trigger.points[i].y, i * 20);
    EXPECT_EQ(trigger.points[i].z, 15);
  }
}

TEST_F(TriggersParserTest, ParsesMultipleTriggers) {
  std::vector<std::string> nameTable = {"PolygonTriggers"};
  auto data = createTOC(nameTable);

  size_t triggersStartPos = data.size();
  appendChunkHeader(data, 1, K_TRIGGERS_VERSION_3, 0);

  appendInt(data, 3);

  for (int t = 0; t < 3; ++t) {
    appendString(data, "Trigger" + std::to_string(t));
    appendInt(data, 1000 + t);
    appendByte(data, t % 2);
    appendByte(data, 0);
    appendInt(data, 0);

    appendInt(data, 3);
    for (int p = 0; p < 3; ++p) {
      appendInt(data, t * 100 + p * 10);
      appendInt(data, t * 200 + p * 20);
      appendInt(data, t * 5);
    }
  }

  int32_t triggersDataSize =
      static_cast<int32_t>(data.size() - triggersStartPos - CHUNK_HEADER_SIZE);
  int32_t *triggersSizePtr = reinterpret_cast<int32_t *>(&data[triggersStartPos + 4 + 2]);
  *triggersSizePtr = triggersDataSize;

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << "Failed to load TOC: " << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());

  auto triggers = TriggersParser::parse(reader, header->version);
  ASSERT_TRUE(triggers.has_value()) << "Failed to parse triggers";
  ASSERT_EQ(triggers->size(), 3);

  for (int t = 0; t < 3; ++t) {
    const auto &trigger = (*triggers)[t];
    EXPECT_EQ(trigger.name, "Trigger" + std::to_string(t));
    EXPECT_EQ(trigger.id, 1000 + t);
    EXPECT_EQ(trigger.isWaterArea, (t % 2) != 0);
    ASSERT_EQ(trigger.points.size(), 3);
  }
}

TEST_F(TriggersParserTest, ParsesVersion4Trigger) {
  std::vector<std::string> nameTable = {"PolygonTriggers"};
  auto data = createTOC(nameTable);

  size_t triggersStartPos = data.size();
  appendChunkHeader(data, 1, K_TRIGGERS_VERSION_4, 0);

  appendInt(data, 1);

  appendString(data, "V4Trigger");
  appendInt(data, 400);
  appendByte(data, 0);
  appendByte(data, 0);
  appendInt(data, 0);

  appendInt(data, 3);
  appendInt(data, 10);
  appendInt(data, 20);
  appendInt(data, 30);

  appendInt(data, 40);
  appendInt(data, 50);
  appendInt(data, 60);

  appendInt(data, 70);
  appendInt(data, 80);
  appendInt(data, 90);

  int32_t triggersDataSize =
      static_cast<int32_t>(data.size() - triggersStartPos - CHUNK_HEADER_SIZE);
  int32_t *triggersSizePtr = reinterpret_cast<int32_t *>(&data[triggersStartPos + 4 + 2]);
  *triggersSizePtr = triggersDataSize;

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << "Failed to load TOC: " << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());
  EXPECT_EQ(header->version, K_TRIGGERS_VERSION_4);

  auto triggers = TriggersParser::parse(reader, header->version);
  ASSERT_TRUE(triggers.has_value()) << "Failed to parse triggers";
  ASSERT_EQ(triggers->size(), 1);

  const auto &trigger = (*triggers)[0];
  EXPECT_EQ(trigger.name, "V4Trigger");
  EXPECT_EQ(trigger.id, 400);
}

TEST_F(TriggersParserTest, HandlesInvalidVersion) {
  std::vector<std::string> nameTable = {"PolygonTriggers"};
  auto data = createTOC(nameTable);

  size_t triggersStartPos = data.size();
  appendChunkHeader(data, 1, 99, 0);

  appendInt(data, 0);

  int32_t triggersDataSize =
      static_cast<int32_t>(data.size() - triggersStartPos - CHUNK_HEADER_SIZE);
  int32_t *triggersSizePtr = reinterpret_cast<int32_t *>(&data[triggersStartPos + 4 + 2]);
  *triggersSizePtr = triggersDataSize;

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << "Failed to load TOC: " << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());

  std::string parseError;
  auto triggers = TriggersParser::parse(reader, header->version, &parseError);
  EXPECT_FALSE(triggers.has_value());
  EXPECT_FALSE(parseError.empty());
}

TEST_F(TriggersParserTest, HandlesEmptyTriggerList) {
  std::vector<std::string> nameTable = {"PolygonTriggers"};
  auto data = createTOC(nameTable);

  size_t triggersStartPos = data.size();
  appendChunkHeader(data, 1, K_TRIGGERS_VERSION_3, 0);

  appendInt(data, 0);

  int32_t triggersDataSize =
      static_cast<int32_t>(data.size() - triggersStartPos - CHUNK_HEADER_SIZE);
  int32_t *triggersSizePtr = reinterpret_cast<int32_t *>(&data[triggersStartPos + 4 + 2]);
  *triggersSizePtr = triggersDataSize;

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << "Failed to load TOC: " << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());

  auto triggers = TriggersParser::parse(reader, header->version);
  ASSERT_TRUE(triggers.has_value());
  EXPECT_TRUE(triggers->empty());
}

TEST_F(TriggersParserTest, TestsPolygonTriggerValidation) {
  PolygonTrigger trigger;

  EXPECT_FALSE(trigger.isValid());

  trigger.name = "Test";
  EXPECT_FALSE(trigger.isValid());

  trigger.points.push_back({0, 0, 0});
  EXPECT_TRUE(trigger.isValid());

  trigger.points.push_back({100, 0, 0});
  trigger.points.push_back({100, 100, 0});
  EXPECT_TRUE(trigger.isValid());
}
