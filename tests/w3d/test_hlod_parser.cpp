#include "w3d/chunk_reader.hpp"
#include "w3d/hlod_parser.hpp"

#include <gtest/gtest.h>

using namespace w3d;

class HLodParserTest : public ::testing::Test {
protected:
  static std::vector<uint8_t> makeChunk(ChunkType type, const std::vector<uint8_t> &data,
                                        bool isContainer = false) {
    std::vector<uint8_t> result;
    uint32_t typeVal = static_cast<uint32_t>(type);
    result.push_back(typeVal & 0xFF);
    result.push_back((typeVal >> 8) & 0xFF);
    result.push_back((typeVal >> 16) & 0xFF);
    result.push_back((typeVal >> 24) & 0xFF);

    uint32_t size = static_cast<uint32_t>(data.size());
    if (isContainer) {
      size |= 0x80000000;
    }
    result.push_back(size & 0xFF);
    result.push_back((size >> 8) & 0xFF);
    result.push_back((size >> 16) & 0xFF);
    result.push_back((size >> 24) & 0xFF);

    result.insert(result.end(), data.begin(), data.end());
    return result;
  }

  static void appendFloat(std::vector<uint8_t> &vec, float f) {
    uint8_t *bytes = reinterpret_cast<uint8_t *>(&f);
    vec.insert(vec.end(), bytes, bytes + sizeof(float));
  }

  static void appendUint32(std::vector<uint8_t> &vec, uint32_t val) {
    vec.push_back(val & 0xFF);
    vec.push_back((val >> 8) & 0xFF);
    vec.push_back((val >> 16) & 0xFF);
    vec.push_back((val >> 24) & 0xFF);
  }

  static void appendFixedString(std::vector<uint8_t> &vec, const std::string &str, size_t len) {
    for (size_t i = 0; i < len; ++i) {
      vec.push_back(i < str.size() ? str[i] : '\0');
    }
  }

  // Create HLod header chunk data
  static std::vector<uint8_t> makeHLodHeader(const std::string &name, const std::string &hierName,
                                             uint32_t lodCount) {
    std::vector<uint8_t> data;
    appendUint32(data, 1); // version
    appendUint32(data, lodCount);
    appendFixedString(data, name, 16);
    appendFixedString(data, hierName, 16);
    return data;
  }

  // Create sub-object array header
  static std::vector<uint8_t> makeSubObjectArrayHeader(uint32_t modelCount, float maxScreenSize) {
    std::vector<uint8_t> data;
    appendUint32(data, modelCount);
    appendFloat(data, maxScreenSize);
    return data;
  }

  // Create sub-object data (boneIndex + 32-char name)
  static std::vector<uint8_t> makeSubObject(uint32_t boneIndex, const std::string &name) {
    std::vector<uint8_t> data;
    appendUint32(data, boneIndex);
    appendFixedString(data, name, 32); // W3D_NAME_LEN * 2
    return data;
  }
};

// =============================================================================
// HLod Header Tests
// =============================================================================

TEST_F(HLodParserTest, EmptyHLodParsing) {
  auto headerData = makeHLodHeader("TestModel", "TestSkeleton", 0);

  std::vector<uint8_t> data;
  auto headerChunk = makeChunk(ChunkType::HLOD_HEADER, headerData);
  data.insert(data.end(), headerChunk.begin(), headerChunk.end());

  ChunkReader reader(data);
  HLod hlod = HLodParser::parse(reader, static_cast<uint32_t>(data.size()));

  EXPECT_EQ(hlod.version, 1);
  EXPECT_EQ(hlod.lodCount, 0);
  EXPECT_EQ(hlod.name, "TestModel");
  EXPECT_EQ(hlod.hierarchyName, "TestSkeleton");
  EXPECT_TRUE(hlod.lodArrays.empty());
}

TEST_F(HLodParserTest, SingleLodArrayParsing) {
  auto headerData = makeHLodHeader("Model", "Skeleton", 1);

  // Build LOD array with sub-object array header and sub-objects
  std::vector<uint8_t> lodArrayContent;

  auto subObjHeader = makeSubObjectArrayHeader(2, 100.0f);
  auto subObjHeaderChunk = makeChunk(ChunkType::HLOD_SUB_OBJECT_ARRAY_HEADER, subObjHeader);
  lodArrayContent.insert(lodArrayContent.end(), subObjHeaderChunk.begin(), subObjHeaderChunk.end());

  auto subObj1 = makeSubObject(0, "MODEL.MESH1");
  auto subObj1Chunk = makeChunk(ChunkType::HLOD_SUB_OBJECT, subObj1);
  lodArrayContent.insert(lodArrayContent.end(), subObj1Chunk.begin(), subObj1Chunk.end());

  auto subObj2 = makeSubObject(1, "MODEL.MESH2");
  auto subObj2Chunk = makeChunk(ChunkType::HLOD_SUB_OBJECT, subObj2);
  lodArrayContent.insert(lodArrayContent.end(), subObj2Chunk.begin(), subObj2Chunk.end());

  std::vector<uint8_t> data;
  auto headerChunk = makeChunk(ChunkType::HLOD_HEADER, headerData);
  auto lodArrayChunk = makeChunk(ChunkType::HLOD_LOD_ARRAY, lodArrayContent, true);
  data.insert(data.end(), headerChunk.begin(), headerChunk.end());
  data.insert(data.end(), lodArrayChunk.begin(), lodArrayChunk.end());

  ChunkReader reader(data);
  HLod hlod = HLodParser::parse(reader, static_cast<uint32_t>(data.size()));

  ASSERT_EQ(hlod.lodArrays.size(), 1);
  EXPECT_EQ(hlod.lodArrays[0].modelCount, 2);
  EXPECT_FLOAT_EQ(hlod.lodArrays[0].maxScreenSize, 100.0f);

  ASSERT_EQ(hlod.lodArrays[0].subObjects.size(), 2);
  EXPECT_EQ(hlod.lodArrays[0].subObjects[0].boneIndex, 0);
  EXPECT_EQ(hlod.lodArrays[0].subObjects[0].name, "MODEL.MESH1");
  EXPECT_EQ(hlod.lodArrays[0].subObjects[1].boneIndex, 1);
  EXPECT_EQ(hlod.lodArrays[0].subObjects[1].name, "MODEL.MESH2");
}

TEST_F(HLodParserTest, MultipleLodLevels) {
  auto headerData = makeHLodHeader("LODModel", "Skeleton", 3);

  // LOD 0 - highest detail (large screen size)
  std::vector<uint8_t> lod0Content;
  auto lod0Header = makeSubObjectArrayHeader(4, 1000.0f);
  auto lod0HeaderChunk = makeChunk(ChunkType::HLOD_SUB_OBJECT_ARRAY_HEADER, lod0Header);
  lod0Content.insert(lod0Content.end(), lod0HeaderChunk.begin(), lod0HeaderChunk.end());
  for (int i = 0; i < 4; ++i) {
    auto subObj = makeSubObject(i, "MODEL.MESH_HI" + std::to_string(i));
    auto subObjChunk = makeChunk(ChunkType::HLOD_SUB_OBJECT, subObj);
    lod0Content.insert(lod0Content.end(), subObjChunk.begin(), subObjChunk.end());
  }

  // LOD 1 - medium detail
  std::vector<uint8_t> lod1Content;
  auto lod1Header = makeSubObjectArrayHeader(2, 100.0f);
  auto lod1HeaderChunk = makeChunk(ChunkType::HLOD_SUB_OBJECT_ARRAY_HEADER, lod1Header);
  lod1Content.insert(lod1Content.end(), lod1HeaderChunk.begin(), lod1HeaderChunk.end());
  for (int i = 0; i < 2; ++i) {
    auto subObj = makeSubObject(i, "MODEL.MESH_MED" + std::to_string(i));
    auto subObjChunk = makeChunk(ChunkType::HLOD_SUB_OBJECT, subObj);
    lod1Content.insert(lod1Content.end(), subObjChunk.begin(), subObjChunk.end());
  }

  // LOD 2 - lowest detail (small screen size)
  std::vector<uint8_t> lod2Content;
  auto lod2Header = makeSubObjectArrayHeader(1, 10.0f);
  auto lod2HeaderChunk = makeChunk(ChunkType::HLOD_SUB_OBJECT_ARRAY_HEADER, lod2Header);
  lod2Content.insert(lod2Content.end(), lod2HeaderChunk.begin(), lod2HeaderChunk.end());
  auto subObj = makeSubObject(0, "MODEL.MESH_LOW");
  auto subObjChunk = makeChunk(ChunkType::HLOD_SUB_OBJECT, subObj);
  lod2Content.insert(lod2Content.end(), subObjChunk.begin(), subObjChunk.end());

  std::vector<uint8_t> data;
  auto headerChunk = makeChunk(ChunkType::HLOD_HEADER, headerData);
  auto lod0Chunk = makeChunk(ChunkType::HLOD_LOD_ARRAY, lod0Content, true);
  auto lod1Chunk = makeChunk(ChunkType::HLOD_LOD_ARRAY, lod1Content, true);
  auto lod2Chunk = makeChunk(ChunkType::HLOD_LOD_ARRAY, lod2Content, true);
  data.insert(data.end(), headerChunk.begin(), headerChunk.end());
  data.insert(data.end(), lod0Chunk.begin(), lod0Chunk.end());
  data.insert(data.end(), lod1Chunk.begin(), lod1Chunk.end());
  data.insert(data.end(), lod2Chunk.begin(), lod2Chunk.end());

  ChunkReader reader(data);
  HLod hlod = HLodParser::parse(reader, static_cast<uint32_t>(data.size()));

  ASSERT_EQ(hlod.lodArrays.size(), 3);

  EXPECT_FLOAT_EQ(hlod.lodArrays[0].maxScreenSize, 1000.0f);
  EXPECT_EQ(hlod.lodArrays[0].subObjects.size(), 4);

  EXPECT_FLOAT_EQ(hlod.lodArrays[1].maxScreenSize, 100.0f);
  EXPECT_EQ(hlod.lodArrays[1].subObjects.size(), 2);

  EXPECT_FLOAT_EQ(hlod.lodArrays[2].maxScreenSize, 10.0f);
  EXPECT_EQ(hlod.lodArrays[2].subObjects.size(), 1);
}

TEST_F(HLodParserTest, AggregateArrayParsing) {
  auto headerData = makeHLodHeader("AggModel", "Skeleton", 1);

  // LOD array
  std::vector<uint8_t> lodContent;
  auto lodHeader = makeSubObjectArrayHeader(1, 100.0f);
  auto lodHeaderChunk = makeChunk(ChunkType::HLOD_SUB_OBJECT_ARRAY_HEADER, lodHeader);
  lodContent.insert(lodContent.end(), lodHeaderChunk.begin(), lodHeaderChunk.end());
  auto mainMesh = makeSubObject(0, "MODEL.BODY");
  auto mainMeshChunk = makeChunk(ChunkType::HLOD_SUB_OBJECT, mainMesh);
  lodContent.insert(lodContent.end(), mainMeshChunk.begin(), mainMeshChunk.end());

  // Aggregate array
  std::vector<uint8_t> aggContent;
  auto aggObj1 = makeSubObject(5, "MODEL.TURRET");
  auto aggObj1Chunk = makeChunk(ChunkType::HLOD_SUB_OBJECT, aggObj1);
  aggContent.insert(aggContent.end(), aggObj1Chunk.begin(), aggObj1Chunk.end());
  auto aggObj2 = makeSubObject(6, "MODEL.BARREL");
  auto aggObj2Chunk = makeChunk(ChunkType::HLOD_SUB_OBJECT, aggObj2);
  aggContent.insert(aggContent.end(), aggObj2Chunk.begin(), aggObj2Chunk.end());

  std::vector<uint8_t> data;
  auto headerChunk = makeChunk(ChunkType::HLOD_HEADER, headerData);
  auto lodChunk = makeChunk(ChunkType::HLOD_LOD_ARRAY, lodContent, true);
  auto aggChunk = makeChunk(ChunkType::HLOD_AGGREGATE_ARRAY, aggContent, true);
  data.insert(data.end(), headerChunk.begin(), headerChunk.end());
  data.insert(data.end(), lodChunk.begin(), lodChunk.end());
  data.insert(data.end(), aggChunk.begin(), aggChunk.end());

  ChunkReader reader(data);
  HLod hlod = HLodParser::parse(reader, static_cast<uint32_t>(data.size()));

  ASSERT_EQ(hlod.aggregates.size(), 2);
  EXPECT_EQ(hlod.aggregates[0].boneIndex, 5);
  EXPECT_EQ(hlod.aggregates[0].name, "MODEL.TURRET");
  EXPECT_EQ(hlod.aggregates[1].boneIndex, 6);
  EXPECT_EQ(hlod.aggregates[1].name, "MODEL.BARREL");
}

TEST_F(HLodParserTest, ProxyArrayParsing) {
  auto headerData = makeHLodHeader("ProxyModel", "Skeleton", 1);

  // LOD array
  std::vector<uint8_t> lodContent;
  auto lodHeader = makeSubObjectArrayHeader(1, 100.0f);
  auto lodHeaderChunk = makeChunk(ChunkType::HLOD_SUB_OBJECT_ARRAY_HEADER, lodHeader);
  lodContent.insert(lodContent.end(), lodHeaderChunk.begin(), lodHeaderChunk.end());
  auto mesh = makeSubObject(0, "MODEL.BODY");
  auto meshChunk = makeChunk(ChunkType::HLOD_SUB_OBJECT, mesh);
  lodContent.insert(lodContent.end(), meshChunk.begin(), meshChunk.end());

  // Proxy array (attachment points)
  std::vector<uint8_t> proxyContent;
  auto proxy1 = makeSubObject(10, "MODEL.WEAPONBONE");
  auto proxy1Chunk = makeChunk(ChunkType::HLOD_SUB_OBJECT, proxy1);
  proxyContent.insert(proxyContent.end(), proxy1Chunk.begin(), proxy1Chunk.end());

  std::vector<uint8_t> data;
  auto headerChunk = makeChunk(ChunkType::HLOD_HEADER, headerData);
  auto lodChunk = makeChunk(ChunkType::HLOD_LOD_ARRAY, lodContent, true);
  auto proxyChunk = makeChunk(ChunkType::HLOD_PROXY_ARRAY, proxyContent, true);
  data.insert(data.end(), headerChunk.begin(), headerChunk.end());
  data.insert(data.end(), lodChunk.begin(), lodChunk.end());
  data.insert(data.end(), proxyChunk.begin(), proxyChunk.end());

  ChunkReader reader(data);
  HLod hlod = HLodParser::parse(reader, static_cast<uint32_t>(data.size()));

  ASSERT_EQ(hlod.proxies.size(), 1);
  EXPECT_EQ(hlod.proxies[0].boneIndex, 10);
  EXPECT_EQ(hlod.proxies[0].name, "MODEL.WEAPONBONE");
}

// =============================================================================
// Box Parsing Tests
// =============================================================================

TEST_F(HLodParserTest, BoxParsing) {
  std::vector<uint8_t> boxData;
  appendUint32(boxData, 1);                      // version
  appendUint32(boxData, 0);                      // attributes
  appendFixedString(boxData, "BOUNDINGBOX", 32); // name (32 chars)
  // RGB color + padding
  boxData.push_back(255); // r
  boxData.push_back(0);   // g
  boxData.push_back(0);   // b
  boxData.push_back(0);   // padding
  // Center
  appendFloat(boxData, 0.0f);
  appendFloat(boxData, 1.0f);
  appendFloat(boxData, 0.0f);
  // Extent
  appendFloat(boxData, 2.0f);
  appendFloat(boxData, 3.0f);
  appendFloat(boxData, 1.5f);

  ChunkReader reader(boxData);
  Box box = HLodParser::parseBox(reader, static_cast<uint32_t>(boxData.size()));

  EXPECT_EQ(box.version, 1);
  EXPECT_EQ(box.attributes, 0);
  EXPECT_EQ(box.name, "BOUNDINGBOX");
  EXPECT_EQ(box.color.r, 255);
  EXPECT_EQ(box.color.g, 0);
  EXPECT_EQ(box.color.b, 0);
  EXPECT_FLOAT_EQ(box.center.x, 0.0f);
  EXPECT_FLOAT_EQ(box.center.y, 1.0f);
  EXPECT_FLOAT_EQ(box.center.z, 0.0f);
  EXPECT_FLOAT_EQ(box.extent.x, 2.0f);
  EXPECT_FLOAT_EQ(box.extent.y, 3.0f);
  EXPECT_FLOAT_EQ(box.extent.z, 1.5f);
}

TEST_F(HLodParserTest, BoxWithAttributes) {
  std::vector<uint8_t> boxData;
  appendUint32(boxData, 1);
  appendUint32(boxData, 0x0F); // Some attributes
  appendFixedString(boxData, "COLLISION", 32);
  boxData.push_back(0);
  boxData.push_back(255);
  boxData.push_back(0);
  boxData.push_back(0);
  appendFloat(boxData, 5.0f);
  appendFloat(boxData, 5.0f);
  appendFloat(boxData, 5.0f);
  appendFloat(boxData, 10.0f);
  appendFloat(boxData, 10.0f);
  appendFloat(boxData, 10.0f);

  ChunkReader reader(boxData);
  Box box = HLodParser::parseBox(reader, static_cast<uint32_t>(boxData.size()));

  EXPECT_EQ(box.attributes, 0x0F);
  EXPECT_EQ(box.name, "COLLISION");
  EXPECT_EQ(box.color.g, 255);
  EXPECT_FLOAT_EQ(box.center.x, 5.0f);
  EXPECT_FLOAT_EQ(box.extent.x, 10.0f);
}

TEST_F(HLodParserTest, UnknownChunksInHLodSkipped) {
  auto headerData = makeHLodHeader("Test", "Skeleton", 1);

  std::vector<uint8_t> data;
  auto headerChunk = makeChunk(ChunkType::HLOD_HEADER, headerData);
  data.insert(data.end(), headerChunk.begin(), headerChunk.end());

  // Unknown chunk
  std::vector<uint8_t> unknownChunk = {
      0xEF, 0xBE, 0xAD, 0xDE, 0x04, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04,
  };
  data.insert(data.end(), unknownChunk.begin(), unknownChunk.end());

  // LOD array after unknown
  std::vector<uint8_t> lodContent;
  auto lodHeader = makeSubObjectArrayHeader(1, 100.0f);
  auto lodHeaderChunk = makeChunk(ChunkType::HLOD_SUB_OBJECT_ARRAY_HEADER, lodHeader);
  lodContent.insert(lodContent.end(), lodHeaderChunk.begin(), lodHeaderChunk.end());
  auto mesh = makeSubObject(0, "MODEL.TEST");
  auto meshChunk = makeChunk(ChunkType::HLOD_SUB_OBJECT, mesh);
  lodContent.insert(lodContent.end(), meshChunk.begin(), meshChunk.end());
  auto lodChunk = makeChunk(ChunkType::HLOD_LOD_ARRAY, lodContent, true);
  data.insert(data.end(), lodChunk.begin(), lodChunk.end());

  ChunkReader reader(data);
  HLod hlod = HLodParser::parse(reader, static_cast<uint32_t>(data.size()));

  EXPECT_EQ(hlod.name, "Test");
  ASSERT_EQ(hlod.lodArrays.size(), 1);
  EXPECT_EQ(hlod.lodArrays[0].subObjects[0].name, "MODEL.TEST");
}

TEST_F(HLodParserTest, ComplexHLodWithAllArrayTypes) {
  auto headerData = makeHLodHeader("ComplexModel", "Skeleton", 2);

  // LOD 0
  std::vector<uint8_t> lod0Content;
  auto lod0Header = makeSubObjectArrayHeader(2, 500.0f);
  auto lod0HeaderChunk = makeChunk(ChunkType::HLOD_SUB_OBJECT_ARRAY_HEADER, lod0Header);
  lod0Content.insert(lod0Content.end(), lod0HeaderChunk.begin(), lod0HeaderChunk.end());
  auto body = makeSubObject(0, "MODEL.BODY");
  auto bodyChunk = makeChunk(ChunkType::HLOD_SUB_OBJECT, body);
  lod0Content.insert(lod0Content.end(), bodyChunk.begin(), bodyChunk.end());
  auto head = makeSubObject(1, "MODEL.HEAD");
  auto headChunk = makeChunk(ChunkType::HLOD_SUB_OBJECT, head);
  lod0Content.insert(lod0Content.end(), headChunk.begin(), headChunk.end());

  // LOD 1
  std::vector<uint8_t> lod1Content;
  auto lod1Header = makeSubObjectArrayHeader(1, 50.0f);
  auto lod1HeaderChunk = makeChunk(ChunkType::HLOD_SUB_OBJECT_ARRAY_HEADER, lod1Header);
  lod1Content.insert(lod1Content.end(), lod1HeaderChunk.begin(), lod1HeaderChunk.end());
  auto lowMesh = makeSubObject(0, "MODEL.LOW");
  auto lowMeshChunk = makeChunk(ChunkType::HLOD_SUB_OBJECT, lowMesh);
  lod1Content.insert(lod1Content.end(), lowMeshChunk.begin(), lowMeshChunk.end());

  // Aggregates
  std::vector<uint8_t> aggContent;
  auto weapon = makeSubObject(5, "MODEL.WEAPON");
  auto weaponChunk = makeChunk(ChunkType::HLOD_SUB_OBJECT, weapon);
  aggContent.insert(aggContent.end(), weaponChunk.begin(), weaponChunk.end());

  // Proxies
  std::vector<uint8_t> proxyContent;
  auto attach = makeSubObject(10, "MODEL.ATTACH");
  auto attachChunk = makeChunk(ChunkType::HLOD_SUB_OBJECT, attach);
  proxyContent.insert(proxyContent.end(), attachChunk.begin(), attachChunk.end());

  std::vector<uint8_t> data;
  auto headerChunk = makeChunk(ChunkType::HLOD_HEADER, headerData);
  auto lod0Chunk = makeChunk(ChunkType::HLOD_LOD_ARRAY, lod0Content, true);
  auto lod1Chunk = makeChunk(ChunkType::HLOD_LOD_ARRAY, lod1Content, true);
  auto aggChunk = makeChunk(ChunkType::HLOD_AGGREGATE_ARRAY, aggContent, true);
  auto proxyChunk = makeChunk(ChunkType::HLOD_PROXY_ARRAY, proxyContent, true);
  data.insert(data.end(), headerChunk.begin(), headerChunk.end());
  data.insert(data.end(), lod0Chunk.begin(), lod0Chunk.end());
  data.insert(data.end(), lod1Chunk.begin(), lod1Chunk.end());
  data.insert(data.end(), aggChunk.begin(), aggChunk.end());
  data.insert(data.end(), proxyChunk.begin(), proxyChunk.end());

  ChunkReader reader(data);
  HLod hlod = HLodParser::parse(reader, static_cast<uint32_t>(data.size()));

  EXPECT_EQ(hlod.name, "ComplexModel");
  EXPECT_EQ(hlod.lodArrays.size(), 2);
  EXPECT_EQ(hlod.aggregates.size(), 1);
  EXPECT_EQ(hlod.proxies.size(), 1);

  EXPECT_EQ(hlod.lodArrays[0].subObjects.size(), 2);
  EXPECT_EQ(hlod.lodArrays[1].subObjects.size(), 1);
  EXPECT_EQ(hlod.aggregates[0].name, "MODEL.WEAPON");
  EXPECT_EQ(hlod.proxies[0].name, "MODEL.ATTACH");
}
