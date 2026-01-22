#include "w3d/chunk_reader.hpp"
#include "w3d/mesh_parser.hpp"

#include <gtest/gtest.h>

using namespace w3d;

class MeshParserTest : public ::testing::Test {
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

  // Helper to append a uint16 to a byte vector
  static void appendUint16(std::vector<uint8_t> &vec, uint16_t val) {
    vec.push_back(val & 0xFF);
    vec.push_back((val >> 8) & 0xFF);
  }

  // Helper to append a fixed-length string
  static void appendFixedString(std::vector<uint8_t> &vec, const std::string &str, size_t len) {
    for (size_t i = 0; i < len; ++i) {
      vec.push_back(i < str.size() ? str[i] : '\0');
    }
  }
};

// =============================================================================
// Shader Struct Tests (16-byte alignment is critical)
// =============================================================================

TEST_F(MeshParserTest, ShaderStructIs16Bytes) {
  // Create shader data: exactly 16 bytes
  std::vector<uint8_t> shaderData = {
      0x03, // depthCompare = PASS_LEQUAL
      0x01, // depthMask = WRITE_ENABLE
      0x00, // colorMask
      0x00, // destBlend = ZERO
      0x00, // fogFunc
      0x01, // priGradient = MODULATE
      0x00, // secGradient = DISABLE
      0x01, // srcBlend = ONE
      0x01, // texturing = ENABLE
      0x00, // detailColorFunc
      0x00, // detailAlphaFunc
      0x00, // shaderPreset
      0x00, // alphaTest
      0x00, // postDetailColorFunc
      0x00, // postDetailAlphaFunc
      0x00, // padding
  };
  EXPECT_EQ(shaderData.size(), 16);

  // Build a mesh with just shaders chunk
  std::vector<uint8_t> meshData;

  // Add MESH_HEADER3 (92 bytes minimum required)
  std::vector<uint8_t> headerData;
  appendUint32(headerData, 0x00040002); // version 4.2
  appendUint32(headerData, 0);          // attributes
  appendFixedString(headerData, "TestMesh", 16);
  appendFixedString(headerData, "", 16);
  appendUint32(headerData, 2); // numTris
  appendUint32(headerData, 4); // numVertices
  appendUint32(headerData, 0); // numMaterials
  appendUint32(headerData, 0); // numDamageStages
  appendUint32(headerData, 0); // sortLevel (int32)
  appendUint32(headerData, 0); // prelitVersion
  appendUint32(headerData, 0); // futureCounts
  appendUint32(headerData, 0); // vertexChannels
  appendUint32(headerData, 0); // faceChannels
  // Bounding box: min
  appendFloat(headerData, -1.0f);
  appendFloat(headerData, -1.0f);
  appendFloat(headerData, -1.0f);
  // Bounding box: max
  appendFloat(headerData, 1.0f);
  appendFloat(headerData, 1.0f);
  appendFloat(headerData, 1.0f);
  // Bounding sphere
  appendFloat(headerData, 0.0f);
  appendFloat(headerData, 0.0f);
  appendFloat(headerData, 0.0f);
  appendFloat(headerData, 1.732f);

  auto headerChunk = makeChunk(ChunkType::MESH_HEADER3, headerData);
  meshData.insert(meshData.end(), headerChunk.begin(), headerChunk.end());

  // Add SHADERS chunk with 2 shaders (32 bytes)
  std::vector<uint8_t> shadersData;
  shadersData.insert(shadersData.end(), shaderData.begin(), shaderData.end());
  shadersData.insert(shadersData.end(), shaderData.begin(), shaderData.end());

  auto shadersChunk = makeChunk(ChunkType::SHADERS, shadersData);
  meshData.insert(meshData.end(), shadersChunk.begin(), shadersChunk.end());

  // Parse it
  ChunkReader reader(meshData);
  Mesh mesh = MeshParser::parse(reader, static_cast<uint32_t>(meshData.size()));

  EXPECT_EQ(mesh.shaders.size(), 2);
  EXPECT_EQ(mesh.shaders[0].depthCompare, 0x03);
  EXPECT_EQ(mesh.shaders[0].depthMask, 0x01);
  EXPECT_EQ(mesh.shaders[0].texturing, 0x01);
}

TEST_F(MeshParserTest, ShaderMultipleCorrectlyParsed) {
  // Create 3 different shaders
  std::vector<uint8_t> shadersData;

  // Shader 1: depth test disabled
  std::vector<uint8_t> shader1 = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};
  // Shader 2: alpha blend enabled
  std::vector<uint8_t> shader2 = {0x03, 0x01, 0x00, 0x05, 0x00, 0x01, 0x00, 0x02,
                                  0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
  // Shader 3: different preset
  std::vector<uint8_t> shader3 = {0x03, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01,
                                  0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00};

  shadersData.insert(shadersData.end(), shader1.begin(), shader1.end());
  shadersData.insert(shadersData.end(), shader2.begin(), shader2.end());
  shadersData.insert(shadersData.end(), shader3.begin(), shader3.end());

  // Build minimal mesh with header + shaders
  std::vector<uint8_t> meshData;

  // Minimal header
  std::vector<uint8_t> headerData(116, 0);
  // Set version
  headerData[0] = 0x02;
  headerData[1] = 0x00;
  headerData[2] = 0x04;
  headerData[3] = 0x00;
  auto headerChunk = makeChunk(ChunkType::MESH_HEADER3, headerData);
  meshData.insert(meshData.end(), headerChunk.begin(), headerChunk.end());

  auto shadersChunk = makeChunk(ChunkType::SHADERS, shadersData);
  meshData.insert(meshData.end(), shadersChunk.begin(), shadersChunk.end());

  ChunkReader reader(meshData);
  Mesh mesh = MeshParser::parse(reader, static_cast<uint32_t>(meshData.size()));

  ASSERT_EQ(mesh.shaders.size(), 3);

  // Verify shader 1
  EXPECT_EQ(mesh.shaders[0].shaderPreset, 0x01);
  EXPECT_EQ(mesh.shaders[0].depthCompare, 0x00);

  // Verify shader 2 (alpha blend)
  EXPECT_EQ(mesh.shaders[1].destBlend, 0x05); // ONE_MINUS_SRC_ALPHA
  EXPECT_EQ(mesh.shaders[1].srcBlend, 0x02);  // SRC_ALPHA
  EXPECT_EQ(mesh.shaders[1].alphaTest, 0x01);

  // Verify shader 3
  EXPECT_EQ(mesh.shaders[2].shaderPreset, 0x02);
}

// =============================================================================
// Triangle Parsing Tests (32 bytes each)
// =============================================================================

TEST_F(MeshParserTest, TriangleParsing) {
  std::vector<uint8_t> triData;

  // Triangle 1: indices 0,1,2, attr=0, normal=(0,1,0), dist=0
  appendUint32(triData, 0);
  appendUint32(triData, 1);
  appendUint32(triData, 2);
  appendUint32(triData, 0);
  appendFloat(triData, 0.0f);
  appendFloat(triData, 1.0f);
  appendFloat(triData, 0.0f);
  appendFloat(triData, 0.0f);

  // Triangle 2: indices 2,3,0, attr=1, normal=(0,0,1), dist=1.5
  appendUint32(triData, 2);
  appendUint32(triData, 3);
  appendUint32(triData, 0);
  appendUint32(triData, 1);
  appendFloat(triData, 0.0f);
  appendFloat(triData, 0.0f);
  appendFloat(triData, 1.0f);
  appendFloat(triData, 1.5f);

  EXPECT_EQ(triData.size(), 64); // 2 triangles * 32 bytes

  // Build mesh
  std::vector<uint8_t> meshData;

  std::vector<uint8_t> headerData(116, 0);
  headerData[0] = 0x02;
  headerData[1] = 0x00;
  headerData[2] = 0x04;
  headerData[3] = 0x00;
  auto headerChunk = makeChunk(ChunkType::MESH_HEADER3, headerData);
  meshData.insert(meshData.end(), headerChunk.begin(), headerChunk.end());

  auto triChunk = makeChunk(ChunkType::TRIANGLES, triData);
  meshData.insert(meshData.end(), triChunk.begin(), triChunk.end());

  ChunkReader reader(meshData);
  Mesh mesh = MeshParser::parse(reader, static_cast<uint32_t>(meshData.size()));

  ASSERT_EQ(mesh.triangles.size(), 2);

  // Verify triangle 1
  EXPECT_EQ(mesh.triangles[0].vertexIndices[0], 0);
  EXPECT_EQ(mesh.triangles[0].vertexIndices[1], 1);
  EXPECT_EQ(mesh.triangles[0].vertexIndices[2], 2);
  EXPECT_FLOAT_EQ(mesh.triangles[0].normal.y, 1.0f);

  // Verify triangle 2
  EXPECT_EQ(mesh.triangles[1].vertexIndices[0], 2);
  EXPECT_EQ(mesh.triangles[1].attributes, 1);
  EXPECT_FLOAT_EQ(mesh.triangles[1].normal.z, 1.0f);
  EXPECT_FLOAT_EQ(mesh.triangles[1].distance, 1.5f);
}

// =============================================================================
// Vertices and Normals Tests
// =============================================================================

TEST_F(MeshParserTest, VertexParsing) {
  std::vector<uint8_t> vertData;
  appendFloat(vertData, 1.0f);
  appendFloat(vertData, 2.0f);
  appendFloat(vertData, 3.0f);
  appendFloat(vertData, 4.0f);
  appendFloat(vertData, 5.0f);
  appendFloat(vertData, 6.0f);

  std::vector<uint8_t> meshData;
  std::vector<uint8_t> headerData(116, 0);
  headerData[0] = 0x02;
  headerData[1] = 0x00;
  headerData[2] = 0x04;
  headerData[3] = 0x00;
  auto headerChunk = makeChunk(ChunkType::MESH_HEADER3, headerData);
  meshData.insert(meshData.end(), headerChunk.begin(), headerChunk.end());

  auto vertChunk = makeChunk(ChunkType::VERTICES, vertData);
  meshData.insert(meshData.end(), vertChunk.begin(), vertChunk.end());

  ChunkReader reader(meshData);
  Mesh mesh = MeshParser::parse(reader, static_cast<uint32_t>(meshData.size()));

  ASSERT_EQ(mesh.vertices.size(), 2);
  EXPECT_FLOAT_EQ(mesh.vertices[0].x, 1.0f);
  EXPECT_FLOAT_EQ(mesh.vertices[0].y, 2.0f);
  EXPECT_FLOAT_EQ(mesh.vertices[0].z, 3.0f);
  EXPECT_FLOAT_EQ(mesh.vertices[1].x, 4.0f);
  EXPECT_FLOAT_EQ(mesh.vertices[1].y, 5.0f);
  EXPECT_FLOAT_EQ(mesh.vertices[1].z, 6.0f);
}

TEST_F(MeshParserTest, NormalParsing) {
  std::vector<uint8_t> normalData;
  appendFloat(normalData, 0.0f);
  appendFloat(normalData, 1.0f);
  appendFloat(normalData, 0.0f);
  appendFloat(normalData, 0.0f);
  appendFloat(normalData, 0.0f);
  appendFloat(normalData, 1.0f);

  std::vector<uint8_t> meshData;
  std::vector<uint8_t> headerData(116, 0);
  headerData[0] = 0x02;
  headerData[1] = 0x00;
  headerData[2] = 0x04;
  headerData[3] = 0x00;
  auto headerChunk = makeChunk(ChunkType::MESH_HEADER3, headerData);
  meshData.insert(meshData.end(), headerChunk.begin(), headerChunk.end());

  auto normalChunk = makeChunk(ChunkType::VERTEX_NORMALS, normalData);
  meshData.insert(meshData.end(), normalChunk.begin(), normalChunk.end());

  ChunkReader reader(meshData);
  Mesh mesh = MeshParser::parse(reader, static_cast<uint32_t>(meshData.size()));

  ASSERT_EQ(mesh.normals.size(), 2);
  EXPECT_FLOAT_EQ(mesh.normals[0].y, 1.0f);
  EXPECT_FLOAT_EQ(mesh.normals[1].z, 1.0f);
}

// =============================================================================
// Material Info Tests
// =============================================================================

TEST_F(MeshParserTest, MaterialInfoParsing) {
  std::vector<uint8_t> matInfoData;
  appendUint32(matInfoData, 2); // passCount
  appendUint32(matInfoData, 3); // vertexMaterialCount
  appendUint32(matInfoData, 4); // shaderCount
  appendUint32(matInfoData, 5); // textureCount

  std::vector<uint8_t> meshData;
  std::vector<uint8_t> headerData(116, 0);
  headerData[0] = 0x02;
  headerData[1] = 0x00;
  headerData[2] = 0x04;
  headerData[3] = 0x00;
  auto headerChunk = makeChunk(ChunkType::MESH_HEADER3, headerData);
  meshData.insert(meshData.end(), headerChunk.begin(), headerChunk.end());

  auto matInfoChunk = makeChunk(ChunkType::MATERIAL_INFO, matInfoData);
  meshData.insert(meshData.end(), matInfoChunk.begin(), matInfoChunk.end());

  ChunkReader reader(meshData);
  Mesh mesh = MeshParser::parse(reader, static_cast<uint32_t>(meshData.size()));

  EXPECT_EQ(mesh.materialInfo.passCount, 2);
  EXPECT_EQ(mesh.materialInfo.vertexMaterialCount, 3);
  EXPECT_EQ(mesh.materialInfo.shaderCount, 4);
  EXPECT_EQ(mesh.materialInfo.textureCount, 5);
}

// =============================================================================
// Mesh Header Tests
// =============================================================================

TEST_F(MeshParserTest, MeshHeaderParsing) {
  std::vector<uint8_t> headerData;
  appendUint32(headerData, 0x00040002);           // version 4.2
  appendUint32(headerData, MeshFlags::TWO_SIDED); // attributes
  appendFixedString(headerData, "MyMesh", 16);
  appendFixedString(headerData, "Container", 16);
  appendUint32(headerData, 100); // numTris
  appendUint32(headerData, 200); // numVertices
  appendUint32(headerData, 3);   // numMaterials
  appendUint32(headerData, 0);   // numDamageStages
  appendUint32(headerData, 5);   // sortLevel (int32)
  appendUint32(headerData, 0);   // prelitVersion
  appendUint32(headerData, 0);   // futureCounts
  appendUint32(headerData, VertexChannels::LOCATION | VertexChannels::NORMAL);
  appendUint32(headerData, FaceChannels::FACE);
  // Bounding box
  appendFloat(headerData, -10.0f);
  appendFloat(headerData, -10.0f);
  appendFloat(headerData, -10.0f);
  appendFloat(headerData, 10.0f);
  appendFloat(headerData, 10.0f);
  appendFloat(headerData, 10.0f);
  // Bounding sphere
  appendFloat(headerData, 0.0f);
  appendFloat(headerData, 0.0f);
  appendFloat(headerData, 0.0f);
  appendFloat(headerData, 17.32f);

  EXPECT_EQ(headerData.size(), 116); // Verify header is exactly 116 bytes

  std::vector<uint8_t> meshData;
  auto headerChunk = makeChunk(ChunkType::MESH_HEADER3, headerData);
  meshData.insert(meshData.end(), headerChunk.begin(), headerChunk.end());

  ChunkReader reader(meshData);
  Mesh mesh = MeshParser::parse(reader, static_cast<uint32_t>(meshData.size()));

  EXPECT_EQ(mesh.header.version, 0x00040002);
  EXPECT_EQ(mesh.header.attributes, MeshFlags::TWO_SIDED);
  EXPECT_EQ(mesh.header.meshName, "MyMesh");
  EXPECT_EQ(mesh.header.containerName, "Container");
  EXPECT_EQ(mesh.header.numTris, 100);
  EXPECT_EQ(mesh.header.numVertices, 200);
  EXPECT_EQ(mesh.header.numMaterials, 3);
  EXPECT_FLOAT_EQ(mesh.header.min.x, -10.0f);
  EXPECT_FLOAT_EQ(mesh.header.max.x, 10.0f);
  EXPECT_FLOAT_EQ(mesh.header.sphRadius, 17.32f);
}

// =============================================================================
// Vertex Colors Tests
// =============================================================================

TEST_F(MeshParserTest, VertexColorsParsing) {
  std::vector<uint8_t> colorData = {
      0xFF, 0x00, 0x00, 0xFF, // Red, full alpha
      0x00, 0xFF, 0x00, 0x80, // Green, half alpha
      0x00, 0x00, 0xFF, 0x00, // Blue, zero alpha
  };

  std::vector<uint8_t> meshData;
  std::vector<uint8_t> headerData(116, 0);
  headerData[0] = 0x02;
  headerData[1] = 0x00;
  headerData[2] = 0x04;
  headerData[3] = 0x00;
  auto headerChunk = makeChunk(ChunkType::MESH_HEADER3, headerData);
  meshData.insert(meshData.end(), headerChunk.begin(), headerChunk.end());

  auto colorChunk = makeChunk(ChunkType::VERTEX_COLORS, colorData);
  meshData.insert(meshData.end(), colorChunk.begin(), colorChunk.end());

  ChunkReader reader(meshData);
  Mesh mesh = MeshParser::parse(reader, static_cast<uint32_t>(meshData.size()));

  ASSERT_EQ(mesh.vertexColors.size(), 3);
  EXPECT_EQ(mesh.vertexColors[0].r, 255);
  EXPECT_EQ(mesh.vertexColors[0].g, 0);
  EXPECT_EQ(mesh.vertexColors[0].a, 255);
  EXPECT_EQ(mesh.vertexColors[1].g, 255);
  EXPECT_EQ(mesh.vertexColors[1].a, 128);
  EXPECT_EQ(mesh.vertexColors[2].b, 255);
  EXPECT_EQ(mesh.vertexColors[2].a, 0);
}

// =============================================================================
// Vertex Influences (Skinning) Tests
// =============================================================================

TEST_F(MeshParserTest, VertexInfluencesParsing) {
  std::vector<uint8_t> influenceData;
  appendUint16(influenceData, 0); // bone1
  appendUint16(influenceData, 1); // bone2
  appendUint16(influenceData, 5); // bone1
  appendUint16(influenceData, 0); // bone2

  std::vector<uint8_t> meshData;
  std::vector<uint8_t> headerData(116, 0);
  headerData[0] = 0x02;
  headerData[1] = 0x00;
  headerData[2] = 0x04;
  headerData[3] = 0x00;
  auto headerChunk = makeChunk(ChunkType::MESH_HEADER3, headerData);
  meshData.insert(meshData.end(), headerChunk.begin(), headerChunk.end());

  auto influenceChunk = makeChunk(ChunkType::VERTEX_INFLUENCES, influenceData);
  meshData.insert(meshData.end(), influenceChunk.begin(), influenceChunk.end());

  ChunkReader reader(meshData);
  Mesh mesh = MeshParser::parse(reader, static_cast<uint32_t>(meshData.size()));

  ASSERT_EQ(mesh.vertexInfluences.size(), 2);
  EXPECT_EQ(mesh.vertexInfluences[0].boneIndex, 0);
  EXPECT_EQ(mesh.vertexInfluences[0].boneIndex2, 1);
  EXPECT_EQ(mesh.vertexInfluences[1].boneIndex, 5);
  EXPECT_EQ(mesh.vertexInfluences[1].boneIndex2, 0);
}

// =============================================================================
// User Text Tests
// =============================================================================

TEST_F(MeshParserTest, UserTextParsing) {
  std::string userText = "This is custom mesh metadata";
  std::vector<uint8_t> textData(userText.begin(), userText.end());
  textData.push_back('\0');

  std::vector<uint8_t> meshData;
  std::vector<uint8_t> headerData(116, 0);
  headerData[0] = 0x02;
  headerData[1] = 0x00;
  headerData[2] = 0x04;
  headerData[3] = 0x00;
  auto headerChunk = makeChunk(ChunkType::MESH_HEADER3, headerData);
  meshData.insert(meshData.end(), headerChunk.begin(), headerChunk.end());

  auto textChunk = makeChunk(ChunkType::MESH_USER_TEXT, textData);
  meshData.insert(meshData.end(), textChunk.begin(), textChunk.end());

  ChunkReader reader(meshData);
  Mesh mesh = MeshParser::parse(reader, static_cast<uint32_t>(meshData.size()));

  EXPECT_EQ(mesh.userText, userText);
}

// =============================================================================
// Unknown Chunks Are Skipped
// =============================================================================

TEST_F(MeshParserTest, UnknownChunksSkipped) {
  std::vector<uint8_t> meshData;

  // Header
  std::vector<uint8_t> headerData(116, 0);
  headerData[0] = 0x02;
  headerData[1] = 0x00;
  headerData[2] = 0x04;
  headerData[3] = 0x00;
  auto headerChunk = makeChunk(ChunkType::MESH_HEADER3, headerData);
  meshData.insert(meshData.end(), headerChunk.begin(), headerChunk.end());

  // Unknown chunk (fake chunk type 0xDEADBEEF)
  std::vector<uint8_t> unknownChunk = {
      0xEF, 0xBE, 0xAD, 0xDE, // type
      0x08, 0x00, 0x00, 0x00, // size = 8
      0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
  };
  meshData.insert(meshData.end(), unknownChunk.begin(), unknownChunk.end());

  // Valid vertices after unknown chunk
  std::vector<uint8_t> vertData;
  appendFloat(vertData, 1.0f);
  appendFloat(vertData, 2.0f);
  appendFloat(vertData, 3.0f);
  auto vertChunk = makeChunk(ChunkType::VERTICES, vertData);
  meshData.insert(meshData.end(), vertChunk.begin(), vertChunk.end());

  ChunkReader reader(meshData);
  Mesh mesh = MeshParser::parse(reader, static_cast<uint32_t>(meshData.size()));

  // Should have parsed vertices despite unknown chunk
  ASSERT_EQ(mesh.vertices.size(), 1);
  EXPECT_FLOAT_EQ(mesh.vertices[0].x, 1.0f);
}

// =============================================================================
// Texture Coordinates Tests
// =============================================================================

TEST_F(MeshParserTest, TexCoordsParsing) {
  std::vector<uint8_t> texData;
  appendFloat(texData, 0.0f);
  appendFloat(texData, 0.0f);
  appendFloat(texData, 1.0f);
  appendFloat(texData, 0.0f);
  appendFloat(texData, 1.0f);
  appendFloat(texData, 1.0f);
  appendFloat(texData, 0.0f);
  appendFloat(texData, 1.0f);

  std::vector<uint8_t> meshData;
  std::vector<uint8_t> headerData(116, 0);
  headerData[0] = 0x02;
  headerData[1] = 0x00;
  headerData[2] = 0x04;
  headerData[3] = 0x00;
  auto headerChunk = makeChunk(ChunkType::MESH_HEADER3, headerData);
  meshData.insert(meshData.end(), headerChunk.begin(), headerChunk.end());

  auto texChunk = makeChunk(ChunkType::TEXCOORDS, texData);
  meshData.insert(meshData.end(), texChunk.begin(), texChunk.end());

  ChunkReader reader(meshData);
  Mesh mesh = MeshParser::parse(reader, static_cast<uint32_t>(meshData.size()));

  ASSERT_EQ(mesh.texCoords.size(), 4);
  // V-coordinate is flipped during parsing (v = 1.0 - v) for Vulkan compatibility
  // File values: (0,0), (1,0), (1,1), (0,1) -> After flip: (0,1), (1,1), (1,0), (0,0)
  EXPECT_FLOAT_EQ(mesh.texCoords[0].u, 0.0f);
  EXPECT_FLOAT_EQ(mesh.texCoords[0].v, 1.0f); // was 0.0 in file
  EXPECT_FLOAT_EQ(mesh.texCoords[1].u, 1.0f);
  EXPECT_FLOAT_EQ(mesh.texCoords[1].v, 1.0f); // was 0.0 in file
  EXPECT_FLOAT_EQ(mesh.texCoords[2].u, 1.0f);
  EXPECT_FLOAT_EQ(mesh.texCoords[2].v, 0.0f); // was 1.0 in file
  EXPECT_FLOAT_EQ(mesh.texCoords[3].u, 0.0f);
  EXPECT_FLOAT_EQ(mesh.texCoords[3].v, 0.0f); // was 1.0 in file
}
