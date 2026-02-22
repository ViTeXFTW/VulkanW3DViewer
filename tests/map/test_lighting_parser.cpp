#include <vector>

#include "../../src/lib/formats/map/data_chunk_reader.hpp"
#include "../../src/lib/formats/map/lighting_parser.hpp"
#include "../../src/lib/formats/map/types.hpp"

#include <gtest/gtest.h>

using namespace map;

class LightingParserTest : public ::testing::Test {
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

  void appendReal(std::vector<uint8_t> &data, float value) {
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(float));
    data.push_back(bits & 0xFF);
    data.push_back((bits >> 8) & 0xFF);
    data.push_back((bits >> 16) & 0xFF);
    data.push_back((bits >> 24) & 0xFF);
  }

  void appendShort(std::vector<uint8_t> &data, uint16_t value) {
    data.push_back(value & 0xFF);
    data.push_back((value >> 8) & 0xFF);
  }

  void appendLight(std::vector<uint8_t> &data, float ar, float ag, float ab, float dr, float dg,
                   float db, float lx, float ly, float lz) {
    appendReal(data, ar);
    appendReal(data, ag);
    appendReal(data, ab);
    appendReal(data, dr);
    appendReal(data, dg);
    appendReal(data, db);
    appendReal(data, lx);
    appendReal(data, ly);
    appendReal(data, lz);
  }

  void appendChunkHeader(std::vector<uint8_t> &data, uint32_t id, uint16_t version,
                         int32_t dataSize) {
    appendInt(data, id);
    appendShort(data, version);
    appendInt(data, dataSize);
  }
};

TEST_F(LightingParserTest, ParsesVersion1Lighting) {
  std::vector<std::string> nameTable = {"GlobalLighting"};
  auto data = createTOC(nameTable);

  size_t lightingStartPos = data.size();
  appendChunkHeader(data, 1, K_LIGHTING_VERSION_1, 0);

  appendInt(data, static_cast<int32_t>(TimeOfDay::Afternoon));

  for (int i = 0; i < NUM_TIME_OF_DAY_SLOTS; ++i) {
    appendLight(data, 0.3f, 0.3f, 0.3f, 0.8f, 0.8f, 0.8f, 0.0f, 0.0f, -1.0f);

    appendLight(data, 0.2f, 0.2f, 0.2f, 0.7f, 0.7f, 0.7f, 0.5f, 0.5f, -0.5f);
  }

  int32_t lightingDataSize =
      static_cast<int32_t>(data.size() - lightingStartPos - CHUNK_HEADER_SIZE);
  int32_t *lightingSizePtr = reinterpret_cast<int32_t *>(&data[lightingStartPos + 4 + 2]);
  *lightingSizePtr = lightingDataSize;

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << "Failed to load TOC: " << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());
  EXPECT_EQ(header->version, K_LIGHTING_VERSION_1);

  auto lighting = LightingParser::parse(reader, header->version);
  ASSERT_TRUE(lighting.has_value()) << "Failed to parse lighting";

  EXPECT_EQ(lighting->currentTimeOfDay, TimeOfDay::Afternoon);
  EXPECT_TRUE(lighting->isValid());

  for (int i = 0; i < NUM_TIME_OF_DAY_SLOTS; ++i) {
    const auto &slot = lighting->timeOfDaySlots[i];
    EXPECT_FLOAT_EQ(slot.terrainLights[0].ambient.r, 0.3f);
    EXPECT_FLOAT_EQ(slot.terrainLights[0].ambient.g, 0.3f);
    EXPECT_FLOAT_EQ(slot.terrainLights[0].ambient.b, 0.3f);
    EXPECT_FLOAT_EQ(slot.terrainLights[0].diffuse.r, 0.8f);
    EXPECT_FLOAT_EQ(slot.terrainLights[0].lightPos.z, -1.0f);

    EXPECT_FLOAT_EQ(slot.objectLights[0].ambient.r, 0.2f);
    EXPECT_FLOAT_EQ(slot.objectLights[0].diffuse.r, 0.7f);
  }
}

TEST_F(LightingParserTest, ParsesVersion2LightingWithAdditionalObjectLights) {
  std::vector<std::string> nameTable = {"GlobalLighting"};
  auto data = createTOC(nameTable);

  size_t lightingStartPos = data.size();
  appendChunkHeader(data, 1, K_LIGHTING_VERSION_2, 0);

  appendInt(data, static_cast<int32_t>(TimeOfDay::Morning));

  for (int i = 0; i < NUM_TIME_OF_DAY_SLOTS; ++i) {
    appendLight(data, 0.3f, 0.3f, 0.3f, 0.8f, 0.8f, 0.8f, 0.0f, 0.0f, -1.0f);

    appendLight(data, 0.2f, 0.2f, 0.2f, 0.7f, 0.7f, 0.7f, 0.5f, 0.5f, -0.5f);

    appendLight(data, 0.1f, 0.1f, 0.1f, 0.6f, 0.6f, 0.6f, 1.0f, 0.0f, 0.0f);
    appendLight(data, 0.15f, 0.15f, 0.15f, 0.65f, 0.65f, 0.65f, -1.0f, 0.0f, 0.0f);
  }

  int32_t lightingDataSize =
      static_cast<int32_t>(data.size() - lightingStartPos - CHUNK_HEADER_SIZE);
  int32_t *lightingSizePtr = reinterpret_cast<int32_t *>(&data[lightingStartPos + 4 + 2]);
  *lightingSizePtr = lightingDataSize;

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << "Failed to load TOC: " << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());
  EXPECT_EQ(header->version, K_LIGHTING_VERSION_2);

  auto lighting = LightingParser::parse(reader, header->version);
  ASSERT_TRUE(lighting.has_value()) << "Failed to parse lighting";

  EXPECT_EQ(lighting->currentTimeOfDay, TimeOfDay::Morning);

  for (int i = 0; i < NUM_TIME_OF_DAY_SLOTS; ++i) {
    const auto &slot = lighting->timeOfDaySlots[i];

    EXPECT_FLOAT_EQ(slot.objectLights[1].ambient.r, 0.1f);
    EXPECT_FLOAT_EQ(slot.objectLights[1].diffuse.r, 0.6f);
    EXPECT_FLOAT_EQ(slot.objectLights[1].lightPos.x, 1.0f);

    EXPECT_FLOAT_EQ(slot.objectLights[2].ambient.r, 0.15f);
    EXPECT_FLOAT_EQ(slot.objectLights[2].diffuse.r, 0.65f);
    EXPECT_FLOAT_EQ(slot.objectLights[2].lightPos.x, -1.0f);
  }
}

TEST_F(LightingParserTest, ParsesVersion3LightingWithAllLights) {
  std::vector<std::string> nameTable = {"GlobalLighting"};
  auto data = createTOC(nameTable);

  size_t lightingStartPos = data.size();
  appendChunkHeader(data, 1, K_LIGHTING_VERSION_3, 0);

  appendInt(data, static_cast<int32_t>(TimeOfDay::Night));

  for (int i = 0; i < NUM_TIME_OF_DAY_SLOTS; ++i) {
    appendLight(data, 0.3f, 0.3f, 0.3f, 0.8f, 0.8f, 0.8f, 0.0f, 0.0f, -1.0f);

    appendLight(data, 0.2f, 0.2f, 0.2f, 0.7f, 0.7f, 0.7f, 0.5f, 0.5f, -0.5f);

    appendLight(data, 0.1f, 0.1f, 0.1f, 0.6f, 0.6f, 0.6f, 1.0f, 0.0f, 0.0f);
    appendLight(data, 0.15f, 0.15f, 0.15f, 0.65f, 0.65f, 0.65f, -1.0f, 0.0f, 0.0f);

    appendLight(data, 0.25f, 0.25f, 0.25f, 0.75f, 0.75f, 0.75f, 0.0f, 1.0f, 0.0f);
    appendLight(data, 0.35f, 0.35f, 0.35f, 0.85f, 0.85f, 0.85f, 0.0f, -1.0f, 0.0f);
  }

  appendInt(data, 0xFF808080);

  int32_t lightingDataSize =
      static_cast<int32_t>(data.size() - lightingStartPos - CHUNK_HEADER_SIZE);
  int32_t *lightingSizePtr = reinterpret_cast<int32_t *>(&data[lightingStartPos + 4 + 2]);
  *lightingSizePtr = lightingDataSize;

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << "Failed to load TOC: " << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());
  EXPECT_EQ(header->version, K_LIGHTING_VERSION_3);

  auto lighting = LightingParser::parse(reader, header->version);
  ASSERT_TRUE(lighting.has_value()) << "Failed to parse lighting";

  EXPECT_EQ(lighting->currentTimeOfDay, TimeOfDay::Night);
  EXPECT_EQ(lighting->shadowColor, 0xFF808080u);

  for (int i = 0; i < NUM_TIME_OF_DAY_SLOTS; ++i) {
    const auto &slot = lighting->timeOfDaySlots[i];

    EXPECT_FLOAT_EQ(slot.terrainLights[1].ambient.r, 0.25f);
    EXPECT_FLOAT_EQ(slot.terrainLights[1].diffuse.r, 0.75f);
    EXPECT_FLOAT_EQ(slot.terrainLights[1].lightPos.y, 1.0f);

    EXPECT_FLOAT_EQ(slot.terrainLights[2].ambient.r, 0.35f);
    EXPECT_FLOAT_EQ(slot.terrainLights[2].diffuse.r, 0.85f);
    EXPECT_FLOAT_EQ(slot.terrainLights[2].lightPos.y, -1.0f);
  }
}

TEST_F(LightingParserTest, ParsesVersion3WithoutShadowColor) {
  std::vector<std::string> nameTable = {"GlobalLighting"};
  auto data = createTOC(nameTable);

  size_t lightingStartPos = data.size();
  appendChunkHeader(data, 1, K_LIGHTING_VERSION_3, 0);

  appendInt(data, static_cast<int32_t>(TimeOfDay::Evening));

  for (int i = 0; i < NUM_TIME_OF_DAY_SLOTS; ++i) {
    appendLight(data, 0.3f, 0.3f, 0.3f, 0.8f, 0.8f, 0.8f, 0.0f, 0.0f, -1.0f);
    appendLight(data, 0.2f, 0.2f, 0.2f, 0.7f, 0.7f, 0.7f, 0.5f, 0.5f, -0.5f);
    appendLight(data, 0.1f, 0.1f, 0.1f, 0.6f, 0.6f, 0.6f, 1.0f, 0.0f, 0.0f);
    appendLight(data, 0.15f, 0.15f, 0.15f, 0.65f, 0.65f, 0.65f, -1.0f, 0.0f, 0.0f);
    appendLight(data, 0.25f, 0.25f, 0.25f, 0.75f, 0.75f, 0.75f, 0.0f, 1.0f, 0.0f);
    appendLight(data, 0.35f, 0.35f, 0.35f, 0.85f, 0.85f, 0.85f, 0.0f, -1.0f, 0.0f);
  }

  int32_t lightingDataSize =
      static_cast<int32_t>(data.size() - lightingStartPos - CHUNK_HEADER_SIZE);
  int32_t *lightingSizePtr = reinterpret_cast<int32_t *>(&data[lightingStartPos + 4 + 2]);
  *lightingSizePtr = lightingDataSize;

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << "Failed to load TOC: " << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());

  auto lighting = LightingParser::parse(reader, header->version);
  ASSERT_TRUE(lighting.has_value()) << "Failed to parse lighting";

  EXPECT_EQ(lighting->currentTimeOfDay, TimeOfDay::Evening);
  EXPECT_EQ(lighting->shadowColor, 0u);
}

TEST_F(LightingParserTest, HandlesInvalidVersion) {
  std::vector<std::string> nameTable = {"GlobalLighting"};
  auto data = createTOC(nameTable);

  size_t lightingStartPos = data.size();
  appendChunkHeader(data, 1, 99, 0);

  appendInt(data, static_cast<int32_t>(TimeOfDay::Morning));

  int32_t lightingDataSize =
      static_cast<int32_t>(data.size() - lightingStartPos - CHUNK_HEADER_SIZE);
  int32_t *lightingSizePtr = reinterpret_cast<int32_t *>(&data[lightingStartPos + 4 + 2]);
  *lightingSizePtr = lightingDataSize;

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value()) << "Failed to load TOC: " << *error;

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());

  std::string parseError;
  auto lighting = LightingParser::parse(reader, header->version, &parseError);
  EXPECT_FALSE(lighting.has_value());
  EXPECT_FALSE(parseError.empty());
}

TEST_F(LightingParserTest, TestsGetCurrentLighting) {
  GlobalLighting lighting;
  lighting.currentTimeOfDay = TimeOfDay::Morning;

  lighting.timeOfDaySlots[0].terrainLights[0].ambient = glm::vec3(1.0f, 0.0f, 0.0f);
  lighting.timeOfDaySlots[1].terrainLights[0].ambient = glm::vec3(0.0f, 1.0f, 0.0f);
  lighting.timeOfDaySlots[2].terrainLights[0].ambient = glm::vec3(0.0f, 0.0f, 1.0f);
  lighting.timeOfDaySlots[3].terrainLights[0].ambient = glm::vec3(1.0f, 1.0f, 1.0f);

  const auto &morningLight = lighting.getCurrentLighting();
  EXPECT_FLOAT_EQ(morningLight.terrainLights[0].ambient.r, 1.0f);
  EXPECT_FLOAT_EQ(morningLight.terrainLights[0].ambient.g, 0.0f);

  lighting.currentTimeOfDay = TimeOfDay::Afternoon;
  const auto &afternoonLight = lighting.getCurrentLighting();
  EXPECT_FLOAT_EQ(afternoonLight.terrainLights[0].ambient.r, 0.0f);
  EXPECT_FLOAT_EQ(afternoonLight.terrainLights[0].ambient.g, 1.0f);

  lighting.currentTimeOfDay = TimeOfDay::Evening;
  const auto &eveningLight = lighting.getCurrentLighting();
  EXPECT_FLOAT_EQ(eveningLight.terrainLights[0].ambient.b, 1.0f);

  lighting.currentTimeOfDay = TimeOfDay::Night;
  const auto &nightLight = lighting.getCurrentLighting();
  EXPECT_FLOAT_EQ(nightLight.terrainLights[0].ambient.r, 1.0f);
  EXPECT_FLOAT_EQ(nightLight.terrainLights[0].ambient.g, 1.0f);
  EXPECT_FLOAT_EQ(nightLight.terrainLights[0].ambient.b, 1.0f);

  lighting.currentTimeOfDay = TimeOfDay::Invalid;
  const auto &defaultLight = lighting.getCurrentLighting();
  EXPECT_FLOAT_EQ(defaultLight.terrainLights[0].ambient.r, 1.0f);
}

TEST_F(LightingParserTest, TestsGlobalLightingValidation) {
  GlobalLighting lighting;

  lighting.currentTimeOfDay = TimeOfDay::Invalid;
  EXPECT_FALSE(lighting.isValid());

  lighting.currentTimeOfDay = TimeOfDay::Morning;
  EXPECT_TRUE(lighting.isValid());

  lighting.currentTimeOfDay = TimeOfDay::Night;
  EXPECT_TRUE(lighting.isValid());
}
