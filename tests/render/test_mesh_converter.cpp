#include "render/mesh_converter.hpp"
#include "w3d/types.hpp"

#include <gtest/gtest.h>

using namespace w3d;

class MeshConverterTest : public ::testing::Test {
protected:
  // Create a basic mesh with vertices, normals, and triangles
  static Mesh createBasicMesh(size_t vertexCount, size_t triCount) {
    Mesh mesh;
    mesh.header.meshName = "TestMesh";
    mesh.header.numVertices = static_cast<uint32_t>(vertexCount);
    mesh.header.numTris = static_cast<uint32_t>(triCount);

    // Create vertices
    for (size_t i = 0; i < vertexCount; ++i) {
      mesh.vertices.push_back({static_cast<float>(i), 0.0f, 0.0f});
      mesh.normals.push_back({0.0f, 1.0f, 0.0f});
    }

    // Create triangles (simple fan from vertex 0)
    for (size_t i = 0; i < triCount; ++i) {
      Triangle tri;
      tri.vertexIndices[0] = 0;
      tri.vertexIndices[1] = static_cast<uint32_t>(i + 1);
      tri.vertexIndices[2] = static_cast<uint32_t>(i + 2);
      mesh.triangles.push_back(tri);
    }

    return mesh;
  }
};

// =============================================================================
// Basic Conversion Tests
// =============================================================================

TEST_F(MeshConverterTest, EmptyMeshReturnsEmpty) {
  Mesh mesh;
  auto converted = MeshConverter::convert(mesh);

  EXPECT_TRUE(converted.vertices.empty());
  EXPECT_TRUE(converted.indices.empty());
}

TEST_F(MeshConverterTest, BasicMeshConversion) {
  auto mesh = createBasicMesh(4, 2);

  auto converted = MeshConverter::convert(mesh);

  EXPECT_EQ(converted.vertices.size(), 4);
  EXPECT_EQ(converted.indices.size(), 6); // 2 triangles * 3 indices
  EXPECT_EQ(converted.name, "TestMesh");
}

TEST_F(MeshConverterTest, VertexPositionsPreserved) {
  Mesh mesh;
  mesh.header.meshName = "Test";
  mesh.vertices.push_back({1.0f, 2.0f, 3.0f});
  mesh.vertices.push_back({4.0f, 5.0f, 6.0f});
  mesh.normals.push_back({0.0f, 1.0f, 0.0f});
  mesh.normals.push_back({0.0f, 1.0f, 0.0f});

  Triangle tri;
  tri.vertexIndices[0] = 0;
  tri.vertexIndices[1] = 1;
  tri.vertexIndices[2] = 0;
  mesh.triangles.push_back(tri);

  auto converted = MeshConverter::convert(mesh);

  ASSERT_EQ(converted.vertices.size(), 2);
  EXPECT_FLOAT_EQ(converted.vertices[0].position.x, 1.0f);
  EXPECT_FLOAT_EQ(converted.vertices[0].position.y, 2.0f);
  EXPECT_FLOAT_EQ(converted.vertices[0].position.z, 3.0f);
  EXPECT_FLOAT_EQ(converted.vertices[1].position.x, 4.0f);
  EXPECT_FLOAT_EQ(converted.vertices[1].position.y, 5.0f);
  EXPECT_FLOAT_EQ(converted.vertices[1].position.z, 6.0f);
}

TEST_F(MeshConverterTest, NormalsPreserved) {
  Mesh mesh;
  mesh.header.meshName = "Test";
  mesh.vertices.push_back({0.0f, 0.0f, 0.0f});
  mesh.normals.push_back({0.577f, 0.577f, 0.577f});

  Triangle tri;
  tri.vertexIndices[0] = 0;
  tri.vertexIndices[1] = 0;
  tri.vertexIndices[2] = 0;
  mesh.triangles.push_back(tri);

  auto converted = MeshConverter::convert(mesh);

  ASSERT_EQ(converted.vertices.size(), 1);
  EXPECT_FLOAT_EQ(converted.vertices[0].normal.x, 0.577f);
  EXPECT_FLOAT_EQ(converted.vertices[0].normal.y, 0.577f);
  EXPECT_FLOAT_EQ(converted.vertices[0].normal.z, 0.577f);
}

TEST_F(MeshConverterTest, MissingNormalsGetDefaultUpVector) {
  Mesh mesh;
  mesh.header.meshName = "Test";
  mesh.vertices.push_back({0.0f, 0.0f, 0.0f});
  // No normals provided

  Triangle tri;
  tri.vertexIndices[0] = 0;
  tri.vertexIndices[1] = 0;
  tri.vertexIndices[2] = 0;
  mesh.triangles.push_back(tri);

  auto converted = MeshConverter::convert(mesh);

  ASSERT_EQ(converted.vertices.size(), 1);
  EXPECT_FLOAT_EQ(converted.vertices[0].normal.x, 0.0f);
  EXPECT_FLOAT_EQ(converted.vertices[0].normal.y, 1.0f);
  EXPECT_FLOAT_EQ(converted.vertices[0].normal.z, 0.0f);
}

// =============================================================================
// Per-Vertex UV Tests
// =============================================================================

TEST_F(MeshConverterTest, MeshLevelTexCoordsUsed) {
  Mesh mesh;
  mesh.header.meshName = "Test";
  mesh.vertices.push_back({0.0f, 0.0f, 0.0f});
  mesh.vertices.push_back({1.0f, 0.0f, 0.0f});
  mesh.normals.push_back({0.0f, 1.0f, 0.0f});
  mesh.normals.push_back({0.0f, 1.0f, 0.0f});
  mesh.texCoords.push_back({0.25f, 0.75f});
  mesh.texCoords.push_back({0.5f, 0.5f});

  Triangle tri;
  tri.vertexIndices[0] = 0;
  tri.vertexIndices[1] = 1;
  tri.vertexIndices[2] = 0;
  mesh.triangles.push_back(tri);

  auto converted = MeshConverter::convert(mesh);

  ASSERT_EQ(converted.vertices.size(), 2);
  EXPECT_FLOAT_EQ(converted.vertices[0].texCoord.x, 0.25f);
  EXPECT_FLOAT_EQ(converted.vertices[0].texCoord.y, 0.75f);
  EXPECT_FLOAT_EQ(converted.vertices[1].texCoord.x, 0.5f);
  EXPECT_FLOAT_EQ(converted.vertices[1].texCoord.y, 0.5f);
}

TEST_F(MeshConverterTest, StageLevelTexCoordsUsedWhenMeshLevelEmpty) {
  Mesh mesh;
  mesh.header.meshName = "Test";
  mesh.vertices.push_back({0.0f, 0.0f, 0.0f});
  mesh.vertices.push_back({1.0f, 0.0f, 0.0f});
  mesh.normals.push_back({0.0f, 1.0f, 0.0f});
  mesh.normals.push_back({0.0f, 1.0f, 0.0f});
  // No mesh-level texCoords

  // Add stage-level UVs
  MaterialPass pass;
  TextureStage stage;
  stage.texCoords.push_back({0.1f, 0.9f});
  stage.texCoords.push_back({0.2f, 0.8f});
  pass.textureStages.push_back(stage);
  mesh.materialPasses.push_back(pass);

  Triangle tri;
  tri.vertexIndices[0] = 0;
  tri.vertexIndices[1] = 1;
  tri.vertexIndices[2] = 0;
  mesh.triangles.push_back(tri);

  auto converted = MeshConverter::convert(mesh);

  ASSERT_EQ(converted.vertices.size(), 2);
  EXPECT_FLOAT_EQ(converted.vertices[0].texCoord.x, 0.1f);
  EXPECT_FLOAT_EQ(converted.vertices[0].texCoord.y, 0.9f);
  EXPECT_FLOAT_EQ(converted.vertices[1].texCoord.x, 0.2f);
  EXPECT_FLOAT_EQ(converted.vertices[1].texCoord.y, 0.8f);
}

TEST_F(MeshConverterTest, MissingTexCoordsGetZero) {
  Mesh mesh;
  mesh.header.meshName = "Test";
  mesh.vertices.push_back({0.0f, 0.0f, 0.0f});
  mesh.normals.push_back({0.0f, 1.0f, 0.0f});
  // No texCoords

  Triangle tri;
  tri.vertexIndices[0] = 0;
  tri.vertexIndices[1] = 0;
  tri.vertexIndices[2] = 0;
  mesh.triangles.push_back(tri);

  auto converted = MeshConverter::convert(mesh);

  ASSERT_EQ(converted.vertices.size(), 1);
  EXPECT_FLOAT_EQ(converted.vertices[0].texCoord.x, 0.0f);
  EXPECT_FLOAT_EQ(converted.vertices[0].texCoord.y, 0.0f);
}

// =============================================================================
// Per-Face UV Index Tests
// =============================================================================

TEST_F(MeshConverterTest, PerFaceUVIndicesUnrollMesh) {
  Mesh mesh;
  mesh.header.meshName = "Test";

  // 4 vertices forming a quad
  mesh.vertices.push_back({0.0f, 0.0f, 0.0f}); // 0
  mesh.vertices.push_back({1.0f, 0.0f, 0.0f}); // 1
  mesh.vertices.push_back({1.0f, 1.0f, 0.0f}); // 2
  mesh.vertices.push_back({0.0f, 1.0f, 0.0f}); // 3

  for (int i = 0; i < 4; ++i) {
    mesh.normals.push_back({0.0f, 0.0f, 1.0f});
  }

  // 2 triangles forming the quad
  Triangle tri1;
  tri1.vertexIndices[0] = 0;
  tri1.vertexIndices[1] = 1;
  tri1.vertexIndices[2] = 2;
  mesh.triangles.push_back(tri1);

  Triangle tri2;
  tri2.vertexIndices[0] = 0;
  tri2.vertexIndices[1] = 2;
  tri2.vertexIndices[2] = 3;
  mesh.triangles.push_back(tri2);

  // Per-face UV setup with stage
  MaterialPass pass;
  TextureStage stage;

  // UV pool (can be reused by different face corners)
  stage.texCoords.push_back({0.0f, 0.0f}); // 0: bottom-left
  stage.texCoords.push_back({1.0f, 0.0f}); // 1: bottom-right
  stage.texCoords.push_back({1.0f, 1.0f}); // 2: top-right
  stage.texCoords.push_back({0.0f, 1.0f}); // 3: top-left

  // Per-face UV indices: 2 triangles * 3 corners = 6 indices
  // Triangle 1 (0,1,2): UV indices 0,1,2
  stage.perFaceTexCoordIds.push_back(0);
  stage.perFaceTexCoordIds.push_back(1);
  stage.perFaceTexCoordIds.push_back(2);
  // Triangle 2 (0,2,3): UV indices 0,2,3
  stage.perFaceTexCoordIds.push_back(0);
  stage.perFaceTexCoordIds.push_back(2);
  stage.perFaceTexCoordIds.push_back(3);

  pass.textureStages.push_back(stage);
  mesh.materialPasses.push_back(pass);

  auto converted = MeshConverter::convert(mesh);

  // With per-face UVs, mesh should be unrolled: 2 tris * 3 verts = 6 vertices
  ASSERT_EQ(converted.vertices.size(), 6);
  ASSERT_EQ(converted.indices.size(), 6);

  // Verify triangle 1 UVs (indices 0,1,2 -> UVs 0,1,2)
  EXPECT_FLOAT_EQ(converted.vertices[0].texCoord.x, 0.0f);
  EXPECT_FLOAT_EQ(converted.vertices[0].texCoord.y, 0.0f);
  EXPECT_FLOAT_EQ(converted.vertices[1].texCoord.x, 1.0f);
  EXPECT_FLOAT_EQ(converted.vertices[1].texCoord.y, 0.0f);
  EXPECT_FLOAT_EQ(converted.vertices[2].texCoord.x, 1.0f);
  EXPECT_FLOAT_EQ(converted.vertices[2].texCoord.y, 1.0f);

  // Verify triangle 2 UVs (indices 0,2,3 -> UVs 0,2,3)
  EXPECT_FLOAT_EQ(converted.vertices[3].texCoord.x, 0.0f);
  EXPECT_FLOAT_EQ(converted.vertices[3].texCoord.y, 0.0f);
  EXPECT_FLOAT_EQ(converted.vertices[4].texCoord.x, 1.0f);
  EXPECT_FLOAT_EQ(converted.vertices[4].texCoord.y, 1.0f);
  EXPECT_FLOAT_EQ(converted.vertices[5].texCoord.x, 0.0f);
  EXPECT_FLOAT_EQ(converted.vertices[5].texCoord.y, 1.0f);

  // Indices should be sequential for unrolled mesh
  for (size_t i = 0; i < 6; ++i) {
    EXPECT_EQ(converted.indices[i], static_cast<uint32_t>(i));
  }
}

TEST_F(MeshConverterTest, PerFaceUVPreservesPositions) {
  Mesh mesh;
  mesh.header.meshName = "Test";

  mesh.vertices.push_back({0.0f, 0.0f, 0.0f}); // 0
  mesh.vertices.push_back({1.0f, 0.0f, 0.0f}); // 1
  mesh.vertices.push_back({0.5f, 1.0f, 0.0f}); // 2

  for (int i = 0; i < 3; ++i) {
    mesh.normals.push_back({0.0f, 0.0f, 1.0f});
  }

  Triangle tri;
  tri.vertexIndices[0] = 0;
  tri.vertexIndices[1] = 1;
  tri.vertexIndices[2] = 2;
  mesh.triangles.push_back(tri);

  MaterialPass pass;
  TextureStage stage;
  stage.texCoords.push_back({0.0f, 0.0f});
  stage.texCoords.push_back({1.0f, 0.0f});
  stage.texCoords.push_back({0.5f, 1.0f});
  stage.perFaceTexCoordIds.push_back(0);
  stage.perFaceTexCoordIds.push_back(1);
  stage.perFaceTexCoordIds.push_back(2);
  pass.textureStages.push_back(stage);
  mesh.materialPasses.push_back(pass);

  auto converted = MeshConverter::convert(mesh);

  ASSERT_EQ(converted.vertices.size(), 3);

  // Positions should match original vertices
  EXPECT_FLOAT_EQ(converted.vertices[0].position.x, 0.0f);
  EXPECT_FLOAT_EQ(converted.vertices[0].position.y, 0.0f);
  EXPECT_FLOAT_EQ(converted.vertices[1].position.x, 1.0f);
  EXPECT_FLOAT_EQ(converted.vertices[1].position.y, 0.0f);
  EXPECT_FLOAT_EQ(converted.vertices[2].position.x, 0.5f);
  EXPECT_FLOAT_EQ(converted.vertices[2].position.y, 1.0f);
}

TEST_F(MeshConverterTest, PerFaceUVWithSharedVertexDifferentUVs) {
  // Test case where same vertex position has different UVs on different faces
  Mesh mesh;
  mesh.header.meshName = "Test";

  // Single vertex used by two triangles
  mesh.vertices.push_back({0.0f, 0.0f, 0.0f}); // shared vertex
  mesh.vertices.push_back({1.0f, 0.0f, 0.0f});
  mesh.vertices.push_back({0.0f, 1.0f, 0.0f});
  mesh.vertices.push_back({-1.0f, 0.0f, 0.0f});

  for (int i = 0; i < 4; ++i) {
    mesh.normals.push_back({0.0f, 0.0f, 1.0f});
  }

  // Two triangles sharing vertex 0
  Triangle tri1;
  tri1.vertexIndices[0] = 0;
  tri1.vertexIndices[1] = 1;
  tri1.vertexIndices[2] = 2;
  mesh.triangles.push_back(tri1);

  Triangle tri2;
  tri2.vertexIndices[0] = 0;
  tri2.vertexIndices[1] = 2;
  tri2.vertexIndices[2] = 3;
  mesh.triangles.push_back(tri2);

  MaterialPass pass;
  TextureStage stage;

  // UV pool
  stage.texCoords.push_back({0.0f, 0.0f}); // 0: used by tri1 for vertex 0
  stage.texCoords.push_back({1.0f, 0.0f}); // 1
  stage.texCoords.push_back({0.5f, 1.0f}); // 2
  stage.texCoords.push_back({0.5f, 0.5f}); // 3: DIFFERENT UV for vertex 0 in tri2
  stage.texCoords.push_back({0.0f, 1.0f}); // 4

  // Tri1: vertex 0 uses UV 0
  stage.perFaceTexCoordIds.push_back(0);
  stage.perFaceTexCoordIds.push_back(1);
  stage.perFaceTexCoordIds.push_back(2);
  // Tri2: vertex 0 uses UV 3 (different!)
  stage.perFaceTexCoordIds.push_back(3);
  stage.perFaceTexCoordIds.push_back(2);
  stage.perFaceTexCoordIds.push_back(4);

  pass.textureStages.push_back(stage);
  mesh.materialPasses.push_back(pass);

  auto converted = MeshConverter::convert(mesh);

  ASSERT_EQ(converted.vertices.size(), 6);

  // Vertex 0 in triangle 1 should have UV (0,0)
  EXPECT_FLOAT_EQ(converted.vertices[0].texCoord.x, 0.0f);
  EXPECT_FLOAT_EQ(converted.vertices[0].texCoord.y, 0.0f);

  // Vertex 0 in triangle 2 should have UV (0.5,0.5) - different!
  EXPECT_FLOAT_EQ(converted.vertices[3].texCoord.x, 0.5f);
  EXPECT_FLOAT_EQ(converted.vertices[3].texCoord.y, 0.5f);

  // But both should have same position
  EXPECT_FLOAT_EQ(converted.vertices[0].position.x, 0.0f);
  EXPECT_FLOAT_EQ(converted.vertices[0].position.y, 0.0f);
  EXPECT_FLOAT_EQ(converted.vertices[3].position.x, 0.0f);
  EXPECT_FLOAT_EQ(converted.vertices[3].position.y, 0.0f);
}

// =============================================================================
// Bounding Box Tests
// =============================================================================

TEST_F(MeshConverterTest, BoundsCalculatedCorrectly) {
  Mesh mesh;
  mesh.header.meshName = "Test";
  mesh.vertices.push_back({-5.0f, -3.0f, -1.0f});
  mesh.vertices.push_back({10.0f, 7.0f, 4.0f});
  mesh.normals.push_back({0.0f, 1.0f, 0.0f});
  mesh.normals.push_back({0.0f, 1.0f, 0.0f});

  Triangle tri;
  tri.vertexIndices[0] = 0;
  tri.vertexIndices[1] = 1;
  tri.vertexIndices[2] = 0;
  mesh.triangles.push_back(tri);

  auto converted = MeshConverter::convert(mesh);

  EXPECT_FLOAT_EQ(converted.bounds.min.x, -5.0f);
  EXPECT_FLOAT_EQ(converted.bounds.min.y, -3.0f);
  EXPECT_FLOAT_EQ(converted.bounds.min.z, -1.0f);
  EXPECT_FLOAT_EQ(converted.bounds.max.x, 10.0f);
  EXPECT_FLOAT_EQ(converted.bounds.max.y, 7.0f);
  EXPECT_FLOAT_EQ(converted.bounds.max.z, 4.0f);
}

// =============================================================================
// Vertex Color Tests
// =============================================================================

TEST_F(MeshConverterTest, VertexColorsApplied) {
  Mesh mesh;
  mesh.header.meshName = "Test";
  mesh.vertices.push_back({0.0f, 0.0f, 0.0f});
  mesh.normals.push_back({0.0f, 1.0f, 0.0f});
  mesh.vertexColors.push_back({255, 128, 64, 255});

  Triangle tri;
  tri.vertexIndices[0] = 0;
  tri.vertexIndices[1] = 0;
  tri.vertexIndices[2] = 0;
  mesh.triangles.push_back(tri);

  auto converted = MeshConverter::convert(mesh);

  ASSERT_EQ(converted.vertices.size(), 1);
  EXPECT_FLOAT_EQ(converted.vertices[0].color.r, 1.0f);
  EXPECT_NEAR(converted.vertices[0].color.g, 0.502f, 0.01f);
  EXPECT_NEAR(converted.vertices[0].color.b, 0.251f, 0.01f);
}

TEST_F(MeshConverterTest, DefaultColorWhenNoVertexColors) {
  Mesh mesh;
  mesh.header.meshName = "Test";
  mesh.vertices.push_back({0.0f, 0.0f, 0.0f});
  mesh.normals.push_back({0.0f, 1.0f, 0.0f});
  // No vertex colors, no materials

  Triangle tri;
  tri.vertexIndices[0] = 0;
  tri.vertexIndices[1] = 0;
  tri.vertexIndices[2] = 0;
  mesh.triangles.push_back(tri);

  auto converted = MeshConverter::convert(mesh);

  ASSERT_EQ(converted.vertices.size(), 1);
  // Default is light gray (0.8, 0.8, 0.8)
  EXPECT_FLOAT_EQ(converted.vertices[0].color.r, 0.8f);
  EXPECT_FLOAT_EQ(converted.vertices[0].color.g, 0.8f);
  EXPECT_FLOAT_EQ(converted.vertices[0].color.b, 0.8f);
}
