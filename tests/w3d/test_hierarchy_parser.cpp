#include "w3d/chunk_reader.hpp"
#include "w3d/hierarchy_parser.hpp"

#include <gtest/gtest.h>

using namespace w3d;

class HierarchyParserTest : public ::testing::Test {
protected:
  // Helper to create a chunk with header
  static std::vector<uint8_t> makeChunk(ChunkType type, const std::vector<uint8_t> &data,
                                        bool isContainer = false) {
    std::vector<uint8_t> result;

    // Chunk type (4 bytes, little-endian)
    uint32_t typeVal = static_cast<uint32_t>(type);
    result.push_back(typeVal & 0xFF);
    result.push_back((typeVal >> 8) & 0xFF);
    result.push_back((typeVal >> 16) & 0xFF);
    result.push_back((typeVal >> 24) & 0xFF);

    // Size (4 bytes, little-endian, with container bit if needed)
    uint32_t size = static_cast<uint32_t>(data.size());
    if (isContainer) {
      size |= 0x80000000;
    }
    result.push_back(size & 0xFF);
    result.push_back((size >> 8) & 0xFF);
    result.push_back((size >> 16) & 0xFF);
    result.push_back((size >> 24) & 0xFF);

    // Append data
    result.insert(result.end(), data.begin(), data.end());
    return result;
  }

  // Helper to append a float to a byte vector
  static void appendFloat(std::vector<uint8_t> &vec, float f) {
    uint8_t *bytes = reinterpret_cast<uint8_t *>(&f);
    vec.insert(vec.end(), bytes, bytes + sizeof(float));
  }

  // Helper to append a uint32 to a byte vector
  static void appendUint32(std::vector<uint8_t> &vec, uint32_t val) {
    vec.push_back(val & 0xFF);
    vec.push_back((val >> 8) & 0xFF);
    vec.push_back((val >> 16) & 0xFF);
    vec.push_back((val >> 24) & 0xFF);
  }

  // Helper to append a fixed-length string (W3D_NAME_LEN = 16)
  static void appendFixedString(std::vector<uint8_t> &vec, const std::string &str, size_t len = 16) {
    for (size_t i = 0; i < len; ++i) {
      vec.push_back(i < str.size() ? str[i] : '\0');
    }
  }

  // Helper to create a pivot (60 bytes total)
  static std::vector<uint8_t> makePivot(const std::string &name, uint32_t parentIndex,
                                        float tx, float ty, float tz,
                                        float qx = 0.0f, float qy = 0.0f, float qz = 0.0f, float qw = 1.0f) {
    std::vector<uint8_t> data;
    appendFixedString(data, name, 16);
    appendUint32(data, parentIndex);
    // Translation
    appendFloat(data, tx);
    appendFloat(data, ty);
    appendFloat(data, tz);
    // Euler angles (typically unused, but still in format)
    appendFloat(data, 0.0f);
    appendFloat(data, 0.0f);
    appendFloat(data, 0.0f);
    // Quaternion
    appendFloat(data, qx);
    appendFloat(data, qy);
    appendFloat(data, qz);
    appendFloat(data, qw);
    return data;
  }
};

// =============================================================================
// Basic Hierarchy Parsing Tests
// =============================================================================

TEST_F(HierarchyParserTest, EmptyHierarchyReturnsEmptyPivots) {
  // Create hierarchy with header but no pivots
  std::vector<uint8_t> headerData;
  appendUint32(headerData, 1); // version
  appendFixedString(headerData, "TestHierarchy", 16);
  appendUint32(headerData, 0); // numPivots = 0
  // Center (3 floats)
  appendFloat(headerData, 0.0f);
  appendFloat(headerData, 0.0f);
  appendFloat(headerData, 0.0f);

  std::vector<uint8_t> data;
  auto headerChunk = makeChunk(ChunkType::HIERARCHY_HEADER, headerData);
  data.insert(data.end(), headerChunk.begin(), headerChunk.end());

  ChunkReader reader(data);
  Hierarchy hierarchy = HierarchyParser::parse(reader, static_cast<uint32_t>(data.size()));

  EXPECT_EQ(hierarchy.version, 1);
  EXPECT_EQ(hierarchy.name, "TestHierarchy");
  EXPECT_TRUE(hierarchy.pivots.empty());
}

TEST_F(HierarchyParserTest, SingleRootPivotParsing) {
  // Create hierarchy with one root pivot
  std::vector<uint8_t> headerData;
  appendUint32(headerData, 1); // version
  appendFixedString(headerData, "SingleBone", 16);
  appendUint32(headerData, 1); // numPivots = 1
  appendFloat(headerData, 0.0f);
  appendFloat(headerData, 0.0f);
  appendFloat(headerData, 0.0f);

  auto pivot = makePivot("ROOTTRANSFORM", 0xFFFFFFFF, 0.0f, 0.0f, 0.0f);

  std::vector<uint8_t> data;
  auto headerChunk = makeChunk(ChunkType::HIERARCHY_HEADER, headerData);
  auto pivotsChunk = makeChunk(ChunkType::PIVOTS, pivot);
  data.insert(data.end(), headerChunk.begin(), headerChunk.end());
  data.insert(data.end(), pivotsChunk.begin(), pivotsChunk.end());

  ChunkReader reader(data);
  Hierarchy hierarchy = HierarchyParser::parse(reader, static_cast<uint32_t>(data.size()));

  ASSERT_EQ(hierarchy.pivots.size(), 1);
  EXPECT_EQ(hierarchy.pivots[0].name, "ROOTTRANSFORM");
  EXPECT_EQ(hierarchy.pivots[0].parentIndex, 0xFFFFFFFF);
  EXPECT_FLOAT_EQ(hierarchy.pivots[0].translation.x, 0.0f);
}

TEST_F(HierarchyParserTest, MultiplePivotsWithHierarchy) {
  // Create hierarchy: Root -> Spine -> Head
  std::vector<uint8_t> headerData;
  appendUint32(headerData, 1);
  appendFixedString(headerData, "Skeleton", 16);
  appendUint32(headerData, 3);
  appendFloat(headerData, 0.0f);
  appendFloat(headerData, 0.0f);
  appendFloat(headerData, 0.0f);

  auto rootPivot = makePivot("ROOTTRANSFORM", 0xFFFFFFFF, 0.0f, 0.0f, 0.0f);
  auto spinePivot = makePivot("BSPINE", 0, 0.0f, 1.0f, 0.0f);
  auto headPivot = makePivot("BHEAD", 1, 0.0f, 0.5f, 0.0f);

  std::vector<uint8_t> pivotsData;
  pivotsData.insert(pivotsData.end(), rootPivot.begin(), rootPivot.end());
  pivotsData.insert(pivotsData.end(), spinePivot.begin(), spinePivot.end());
  pivotsData.insert(pivotsData.end(), headPivot.begin(), headPivot.end());

  std::vector<uint8_t> data;
  auto headerChunk = makeChunk(ChunkType::HIERARCHY_HEADER, headerData);
  auto pivotsChunk = makeChunk(ChunkType::PIVOTS, pivotsData);
  data.insert(data.end(), headerChunk.begin(), headerChunk.end());
  data.insert(data.end(), pivotsChunk.begin(), pivotsChunk.end());

  ChunkReader reader(data);
  Hierarchy hierarchy = HierarchyParser::parse(reader, static_cast<uint32_t>(data.size()));

  ASSERT_EQ(hierarchy.pivots.size(), 3);

  // Root pivot
  EXPECT_EQ(hierarchy.pivots[0].name, "ROOTTRANSFORM");
  EXPECT_EQ(hierarchy.pivots[0].parentIndex, 0xFFFFFFFF);

  // Spine (parent = root at index 0)
  EXPECT_EQ(hierarchy.pivots[1].name, "BSPINE");
  EXPECT_EQ(hierarchy.pivots[1].parentIndex, 0);
  EXPECT_FLOAT_EQ(hierarchy.pivots[1].translation.y, 1.0f);

  // Head (parent = spine at index 1)
  EXPECT_EQ(hierarchy.pivots[2].name, "BHEAD");
  EXPECT_EQ(hierarchy.pivots[2].parentIndex, 1);
  EXPECT_FLOAT_EQ(hierarchy.pivots[2].translation.y, 0.5f);
}

TEST_F(HierarchyParserTest, PivotTranslationParsing) {
  std::vector<uint8_t> headerData;
  appendUint32(headerData, 1);
  appendFixedString(headerData, "Test", 16);
  appendUint32(headerData, 1);
  appendFloat(headerData, 0.0f);
  appendFloat(headerData, 0.0f);
  appendFloat(headerData, 0.0f);

  auto pivot = makePivot("BONE", 0xFFFFFFFF, 1.5f, 2.5f, -3.5f);

  std::vector<uint8_t> data;
  auto headerChunk = makeChunk(ChunkType::HIERARCHY_HEADER, headerData);
  auto pivotsChunk = makeChunk(ChunkType::PIVOTS, pivot);
  data.insert(data.end(), headerChunk.begin(), headerChunk.end());
  data.insert(data.end(), pivotsChunk.begin(), pivotsChunk.end());

  ChunkReader reader(data);
  Hierarchy hierarchy = HierarchyParser::parse(reader, static_cast<uint32_t>(data.size()));

  ASSERT_EQ(hierarchy.pivots.size(), 1);
  EXPECT_FLOAT_EQ(hierarchy.pivots[0].translation.x, 1.5f);
  EXPECT_FLOAT_EQ(hierarchy.pivots[0].translation.y, 2.5f);
  EXPECT_FLOAT_EQ(hierarchy.pivots[0].translation.z, -3.5f);
}

TEST_F(HierarchyParserTest, PivotQuaternionRotationParsing) {
  std::vector<uint8_t> headerData;
  appendUint32(headerData, 1);
  appendFixedString(headerData, "Test", 16);
  appendUint32(headerData, 1);
  appendFloat(headerData, 0.0f);
  appendFloat(headerData, 0.0f);
  appendFloat(headerData, 0.0f);

  // 90 degree rotation around Y axis: quat(0, sin(45), 0, cos(45)) = (0, 0.707, 0, 0.707)
  auto pivot = makePivot("BONE", 0xFFFFFFFF, 0.0f, 0.0f, 0.0f, 0.0f, 0.707f, 0.0f, 0.707f);

  std::vector<uint8_t> data;
  auto headerChunk = makeChunk(ChunkType::HIERARCHY_HEADER, headerData);
  auto pivotsChunk = makeChunk(ChunkType::PIVOTS, pivot);
  data.insert(data.end(), headerChunk.begin(), headerChunk.end());
  data.insert(data.end(), pivotsChunk.begin(), pivotsChunk.end());

  ChunkReader reader(data);
  Hierarchy hierarchy = HierarchyParser::parse(reader, static_cast<uint32_t>(data.size()));

  ASSERT_EQ(hierarchy.pivots.size(), 1);
  EXPECT_NEAR(hierarchy.pivots[0].rotation.x, 0.0f, 0.001f);
  EXPECT_NEAR(hierarchy.pivots[0].rotation.y, 0.707f, 0.001f);
  EXPECT_NEAR(hierarchy.pivots[0].rotation.z, 0.0f, 0.001f);
  EXPECT_NEAR(hierarchy.pivots[0].rotation.w, 0.707f, 0.001f);
}

TEST_F(HierarchyParserTest, HierarchyCenterParsing) {
  std::vector<uint8_t> headerData;
  appendUint32(headerData, 1);
  appendFixedString(headerData, "Centered", 16);
  appendUint32(headerData, 0);
  appendFloat(headerData, 10.0f);
  appendFloat(headerData, 20.0f);
  appendFloat(headerData, 30.0f);

  std::vector<uint8_t> data;
  auto headerChunk = makeChunk(ChunkType::HIERARCHY_HEADER, headerData);
  data.insert(data.end(), headerChunk.begin(), headerChunk.end());

  ChunkReader reader(data);
  Hierarchy hierarchy = HierarchyParser::parse(reader, static_cast<uint32_t>(data.size()));

  EXPECT_FLOAT_EQ(hierarchy.center.x, 10.0f);
  EXPECT_FLOAT_EQ(hierarchy.center.y, 20.0f);
  EXPECT_FLOAT_EQ(hierarchy.center.z, 30.0f);
}

TEST_F(HierarchyParserTest, PivotFixupsParsing) {
  std::vector<uint8_t> headerData;
  appendUint32(headerData, 1);
  appendFixedString(headerData, "WithFixups", 16);
  appendUint32(headerData, 2);
  appendFloat(headerData, 0.0f);
  appendFloat(headerData, 0.0f);
  appendFloat(headerData, 0.0f);

  auto pivot1 = makePivot("BONE1", 0xFFFFFFFF, 0.0f, 0.0f, 0.0f);
  auto pivot2 = makePivot("BONE2", 0, 1.0f, 0.0f, 0.0f);

  std::vector<uint8_t> pivotsData;
  pivotsData.insert(pivotsData.end(), pivot1.begin(), pivot1.end());
  pivotsData.insert(pivotsData.end(), pivot2.begin(), pivot2.end());

  // Pivot fixups (3 floats per pivot)
  std::vector<uint8_t> fixupsData;
  appendFloat(fixupsData, 0.1f);
  appendFloat(fixupsData, 0.2f);
  appendFloat(fixupsData, 0.3f);
  appendFloat(fixupsData, 0.4f);
  appendFloat(fixupsData, 0.5f);
  appendFloat(fixupsData, 0.6f);

  std::vector<uint8_t> data;
  auto headerChunk = makeChunk(ChunkType::HIERARCHY_HEADER, headerData);
  auto pivotsChunk = makeChunk(ChunkType::PIVOTS, pivotsData);
  auto fixupsChunk = makeChunk(ChunkType::PIVOT_FIXUPS, fixupsData);
  data.insert(data.end(), headerChunk.begin(), headerChunk.end());
  data.insert(data.end(), pivotsChunk.begin(), pivotsChunk.end());
  data.insert(data.end(), fixupsChunk.begin(), fixupsChunk.end());

  ChunkReader reader(data);
  Hierarchy hierarchy = HierarchyParser::parse(reader, static_cast<uint32_t>(data.size()));

  ASSERT_EQ(hierarchy.pivotFixups.size(), 2);
  EXPECT_FLOAT_EQ(hierarchy.pivotFixups[0].x, 0.1f);
  EXPECT_FLOAT_EQ(hierarchy.pivotFixups[0].y, 0.2f);
  EXPECT_FLOAT_EQ(hierarchy.pivotFixups[0].z, 0.3f);
  EXPECT_FLOAT_EQ(hierarchy.pivotFixups[1].x, 0.4f);
  EXPECT_FLOAT_EQ(hierarchy.pivotFixups[1].y, 0.5f);
  EXPECT_FLOAT_EQ(hierarchy.pivotFixups[1].z, 0.6f);
}

TEST_F(HierarchyParserTest, UnknownChunksSkipped) {
  std::vector<uint8_t> headerData;
  appendUint32(headerData, 1);
  appendFixedString(headerData, "Test", 16);
  appendUint32(headerData, 1);
  appendFloat(headerData, 0.0f);
  appendFloat(headerData, 0.0f);
  appendFloat(headerData, 0.0f);

  auto pivot = makePivot("BONE", 0xFFFFFFFF, 1.0f, 2.0f, 3.0f);

  std::vector<uint8_t> data;
  auto headerChunk = makeChunk(ChunkType::HIERARCHY_HEADER, headerData);
  data.insert(data.end(), headerChunk.begin(), headerChunk.end());

  // Unknown chunk
  std::vector<uint8_t> unknownChunk = {
      0xEF, 0xBE, 0xAD, 0xDE, // fake type
      0x04, 0x00, 0x00, 0x00, // size = 4
      0x01, 0x02, 0x03, 0x04,
  };
  data.insert(data.end(), unknownChunk.begin(), unknownChunk.end());

  auto pivotsChunk = makeChunk(ChunkType::PIVOTS, pivot);
  data.insert(data.end(), pivotsChunk.begin(), pivotsChunk.end());

  ChunkReader reader(data);
  Hierarchy hierarchy = HierarchyParser::parse(reader, static_cast<uint32_t>(data.size()));

  // Should still parse pivots despite unknown chunk
  ASSERT_EQ(hierarchy.pivots.size(), 1);
  EXPECT_FLOAT_EQ(hierarchy.pivots[0].translation.x, 1.0f);
}

TEST_F(HierarchyParserTest, LargeBoneHierarchy) {
  // Test with 10 bones in a chain
  std::vector<uint8_t> headerData;
  appendUint32(headerData, 1);
  appendFixedString(headerData, "LargeSkeleton", 16);
  appendUint32(headerData, 10);
  appendFloat(headerData, 0.0f);
  appendFloat(headerData, 0.0f);
  appendFloat(headerData, 0.0f);

  std::vector<uint8_t> pivotsData;
  for (int i = 0; i < 10; ++i) {
    std::string name = "BONE" + std::to_string(i);
    uint32_t parent = (i == 0) ? 0xFFFFFFFF : static_cast<uint32_t>(i - 1);
    auto pivot = makePivot(name, parent, 0.0f, static_cast<float>(i) * 0.5f, 0.0f);
    pivotsData.insert(pivotsData.end(), pivot.begin(), pivot.end());
  }

  std::vector<uint8_t> data;
  auto headerChunk = makeChunk(ChunkType::HIERARCHY_HEADER, headerData);
  auto pivotsChunk = makeChunk(ChunkType::PIVOTS, pivotsData);
  data.insert(data.end(), headerChunk.begin(), headerChunk.end());
  data.insert(data.end(), pivotsChunk.begin(), pivotsChunk.end());

  ChunkReader reader(data);
  Hierarchy hierarchy = HierarchyParser::parse(reader, static_cast<uint32_t>(data.size()));

  ASSERT_EQ(hierarchy.pivots.size(), 10);

  // Verify chain
  EXPECT_EQ(hierarchy.pivots[0].parentIndex, 0xFFFFFFFF);
  for (int i = 1; i < 10; ++i) {
    EXPECT_EQ(hierarchy.pivots[i].parentIndex, static_cast<uint32_t>(i - 1));
    EXPECT_FLOAT_EQ(hierarchy.pivots[i].translation.y, static_cast<float>(i) * 0.5f);
  }
}
