#include <filesystem>
#include <fstream>

#include "w3d/loader.hpp"

#include <gtest/gtest.h>

using namespace w3d;
namespace fs = std::filesystem;

class LoaderTest : public ::testing::Test {
protected:
  // Get path to test fixtures directory
  static fs::path fixturesDir() { return fs::path(W3D_TEST_FIXTURES_DIR); }

  // Check if test fixtures are available
  static bool fixturesAvailable() {
    return fs::exists(fixturesDir()) && fs::is_directory(fixturesDir());
  }

  // Helper to create synthetic W3D data in memory
  static std::vector<uint8_t> makeMinimalMeshFile() {
    std::vector<uint8_t> data;

    // MESH chunk header (container)
    appendUint32(data, 0x00000000);       // ChunkType::MESH
    appendUint32(data, 0x80000000 | 124); // Size with container bit (8 + 116 = 124)

    // MESH_HEADER3 chunk
    appendUint32(data, 0x0000001F); // ChunkType::MESH_HEADER3
    appendUint32(data, 116);        // Size

    // Header data (116 bytes)
    appendUint32(data, 0x00040002); // version 4.2
    appendUint32(data, 0);          // attributes
    appendFixedString(data, "TestMesh", 16);
    appendFixedString(data, "", 16);
    appendUint32(data, 1); // numTris
    appendUint32(data, 3); // numVertices
    appendUint32(data, 0); // numMaterials
    appendUint32(data, 0); // numDamageStages
    appendUint32(data, 0); // sortLevel
    appendUint32(data, 0); // prelitVersion
    appendUint32(data, 0); // futureCounts
    appendUint32(data, 0); // vertexChannels
    appendUint32(data, 0); // faceChannels
    // Bounding box: min
    appendFloat(data, -1.0f);
    appendFloat(data, -1.0f);
    appendFloat(data, -1.0f);
    // Bounding box: max
    appendFloat(data, 1.0f);
    appendFloat(data, 1.0f);
    appendFloat(data, 1.0f);
    // Bounding sphere
    appendFloat(data, 0.0f);
    appendFloat(data, 0.0f);
    appendFloat(data, 0.0f);
    appendFloat(data, 1.732f);

    return data;
  }

  static void appendUint32(std::vector<uint8_t> &vec, uint32_t val) {
    vec.push_back(val & 0xFF);
    vec.push_back((val >> 8) & 0xFF);
    vec.push_back((val >> 16) & 0xFF);
    vec.push_back((val >> 24) & 0xFF);
  }

  static void appendFloat(std::vector<uint8_t> &vec, float f) {
    uint8_t *bytes = reinterpret_cast<uint8_t *>(&f);
    vec.insert(vec.end(), bytes, bytes + sizeof(float));
  }

  static void appendFixedString(std::vector<uint8_t> &vec, const std::string &str, size_t len) {
    for (size_t i = 0; i < len; ++i) {
      vec.push_back(i < str.size() ? str[i] : '\0');
    }
  }
};

// =============================================================================
// Memory Loading Tests
// =============================================================================

TEST_F(LoaderTest, LoadFromMemoryMinimalMesh) {
  auto data = makeMinimalMeshFile();

  std::string error;
  auto result = Loader::loadFromMemory(data.data(), data.size(), &error);

  ASSERT_TRUE(result.has_value()) << "Load failed: " << error;
  EXPECT_EQ(result->meshes.size(), 1);
  EXPECT_EQ(result->meshes[0].header.meshName, "TestMesh");
}

TEST_F(LoaderTest, LoadFromMemoryEmptyData) {
  std::string error;
  auto result = Loader::loadFromMemory(nullptr, 0, &error);

  // Empty data should succeed with empty file
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->meshes.empty());
  EXPECT_TRUE(result->hierarchies.empty());
  EXPECT_TRUE(result->animations.empty());
}

TEST_F(LoaderTest, LoadFromMemoryTruncatedChunkHeader) {
  // Only 4 bytes - not enough for a chunk header (needs 8)
  std::vector<uint8_t> data = {0x00, 0x00, 0x00, 0x00};

  std::string error;
  auto result = Loader::loadFromMemory(data.data(), data.size(), &error);

  // Should succeed but not parse incomplete data
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->meshes.empty());
}

TEST_F(LoaderTest, LoadFromMemoryChunkSizeExceedsData) {
  std::vector<uint8_t> data;
  appendUint32(data, 0x00000000); // MESH chunk type
  appendUint32(data, 0x80001000); // Size claims 4096 bytes but we don't have that

  std::string error;
  auto result = Loader::loadFromMemory(data.data(), data.size(), &error);

  EXPECT_FALSE(result.has_value());
  EXPECT_FALSE(error.empty());
}

TEST_F(LoaderTest, LoadFromMemoryUnknownTopLevelChunk) {
  std::vector<uint8_t> data;
  appendUint32(data, 0xDEADBEEF); // Unknown chunk type
  appendUint32(data, 4);          // Size = 4
  appendUint32(data, 0x12345678); // Some data

  std::string error;
  auto result = Loader::loadFromMemory(data.data(), data.size(), &error);

  // Unknown chunks should be skipped, not cause failure
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->meshes.empty());
}

// =============================================================================
// File Loading Tests (with real W3D files)
// =============================================================================

TEST_F(LoaderTest, LoadNonexistentFile) {
  std::string error;
  auto result = Loader::load(fs::path("/nonexistent/path/to/file.w3d"), &error);

  EXPECT_FALSE(result.has_value());
  EXPECT_FALSE(error.empty());
}

TEST_F(LoaderTest, LoadRealW3DFile_CBAIRPORT2) {
  if (!fixturesAvailable()) {
    GTEST_SKIP() << "Test fixtures not available at " << fixturesDir();
  }

  fs::path testFile = fixturesDir() / "CBAIRPORT2.w3d";
  if (!fs::exists(testFile)) {
    GTEST_SKIP() << "Test file not found: " << testFile;
  }

  std::string error;
  auto result = Loader::load(testFile, &error);

  ASSERT_TRUE(result.has_value()) << "Load failed: " << error;

  // This is a building model, should have meshes
  EXPECT_FALSE(result->meshes.empty());

  // Verify mesh data is reasonable
  for (const auto &mesh : result->meshes) {
    EXPECT_FALSE(mesh.header.meshName.empty());
    // Vertex counts should match what header says (or be close)
    if (mesh.header.numVertices > 0) {
      EXPECT_FALSE(mesh.vertices.empty());
    }
  }
}

TEST_F(LoaderTest, LoadRealW3DFile_CBChalet2) {
  if (!fixturesAvailable()) {
    GTEST_SKIP() << "Test fixtures not available at " << fixturesDir();
  }

  fs::path testFile = fixturesDir() / "CBChalet2.w3d";
  if (!fs::exists(testFile)) {
    GTEST_SKIP() << "Test file not found: " << testFile;
  }

  std::string error;
  auto result = Loader::load(testFile, &error);

  ASSERT_TRUE(result.has_value()) << "Load failed: " << error;
  EXPECT_FALSE(result->meshes.empty());
}

// Test loading multiple files to ensure consistency
TEST_F(LoaderTest, LoadMultipleW3DFiles) {
  if (!fixturesAvailable()) {
    GTEST_SKIP() << "Test fixtures not available at " << fixturesDir();
  }

  std::vector<std::string> testFiles = {
      "CBAIRPORT2.w3d",
      "CBChalet2.w3d",
      "CBChalet3.w3d",
  };

  int loadedCount = 0;
  for (const auto &filename : testFiles) {
    fs::path testFile = fixturesDir() / filename;
    if (!fs::exists(testFile)) {
      continue;
    }

    std::string error;
    auto result = Loader::load(testFile, &error);

    EXPECT_TRUE(result.has_value()) << "Failed to load " << filename << ": " << error;
    if (result.has_value()) {
      loadedCount++;
    }
  }

  // Should have loaded at least one file
  EXPECT_GT(loadedCount, 0) << "No test files were loaded";
}

// =============================================================================
// Describe Function Tests
// =============================================================================

TEST_F(LoaderTest, DescribeEmptyFile) {
  W3DFile emptyFile;
  std::string description = Loader::describe(emptyFile);

  // Should produce some output even for empty file
  EXPECT_FALSE(description.empty());
}

TEST_F(LoaderTest, DescribeFileWithMesh) {
  auto data = makeMinimalMeshFile();

  std::string error;
  auto result = Loader::loadFromMemory(data.data(), data.size(), &error);
  ASSERT_TRUE(result.has_value());

  std::string description = Loader::describe(*result);

  EXPECT_FALSE(description.empty());
  // Should mention the mesh name
  EXPECT_NE(description.find("TestMesh"), std::string::npos);
}

TEST_F(LoaderTest, DescribeRealFile) {
  if (!fixturesAvailable()) {
    GTEST_SKIP() << "Test fixtures not available";
  }

  fs::path testFile = fixturesDir() / "CBAIRPORT2.w3d";
  if (!fs::exists(testFile)) {
    GTEST_SKIP() << "Test file not found";
  }

  std::string error;
  auto result = Loader::load(testFile, &error);
  ASSERT_TRUE(result.has_value());

  std::string description = Loader::describe(*result);

  EXPECT_FALSE(description.empty());
  // Should contain mesh information
  EXPECT_NE(description.find("Mesh"), std::string::npos);
}

// =============================================================================
// Edge Cases and Error Handling
// =============================================================================

TEST_F(LoaderTest, LoadCorruptedData) {
  // Create data that looks like a valid chunk but has corrupted internals
  std::vector<uint8_t> data;
  appendUint32(data, 0x00000000); // MESH chunk type
  appendUint32(data, 0x80000010); // Container, size = 16

  // Invalid mesh header chunk (too small)
  appendUint32(data, 0x0000001F); // MESH_HEADER3
  appendUint32(data, 8);          // Size = 8 (should be 92)
  // Only 8 bytes of data
  appendUint32(data, 0x00040002);
  appendUint32(data, 0);

  std::string error;
  auto result = Loader::loadFromMemory(data.data(), data.size(), &error);

  // May fail or succeed partially depending on implementation
  // The important thing is it doesn't crash
  if (!result.has_value()) {
    EXPECT_FALSE(error.empty());
  }
}

TEST_F(LoaderTest, LoadWithNullErrorPointer) {
  auto data = makeMinimalMeshFile();

  // Should work without error pointer
  auto result = Loader::loadFromMemory(data.data(), data.size(), nullptr);

  EXPECT_TRUE(result.has_value());
}

// =============================================================================
// Hierarchy Loading Tests
// =============================================================================

TEST_F(LoaderTest, LoadHierarchyFromMemory) {
  std::vector<uint8_t> data;

  // HIERARCHY chunk (container)
  appendUint32(data, 0x00000100);      // ChunkType::HIERARCHY
  uint32_t hierarchyDataSize = 8 + 72; // header chunk + data
  appendUint32(data, 0x80000000 | hierarchyDataSize);

  // HIERARCHY_HEADER chunk
  appendUint32(data, 0x00000101); // ChunkType::HIERARCHY_HEADER
  appendUint32(data, 64);         // Size

  // Header data
  appendUint32(data, 0x00040001); // version
  appendFixedString(data, "TestHier", 16);
  appendUint32(data, 1);          // numPivots
  appendFloat(data, 0.0f);        // center x
  appendFloat(data, 0.0f);        // center y
  appendFloat(data, 0.0f);        // center z

  // Pad to 64 bytes
  for (size_t i = data.size() - 8 - 28; i < 64; ++i) {
    data.push_back(0);
  }

  std::string error;
  auto result = Loader::loadFromMemory(data.data(), data.size(), &error);

  ASSERT_TRUE(result.has_value()) << "Load failed: " << error;
  // Hierarchy parsing should work (may be empty if pivots chunk missing)
  EXPECT_EQ(result->hierarchies.size(), 1);
}

// =============================================================================
// Box Loading Tests
// =============================================================================

TEST_F(LoaderTest, LoadBoxFromMemory) {
  std::vector<uint8_t> data;

  // BOX chunk
  appendUint32(data, 0x00000740); // ChunkType::BOX
  uint32_t boxDataSize =
      4 + 4 + 32 + 4 + 12 + 12;   // version + attrs + name + color + center + extent
  appendUint32(data, boxDataSize);

  // Box data
  appendUint32(data, 0x00010000); // version
  appendUint32(data, 0);          // attributes
  appendFixedString(data, "CollisionBox", 32);
  // RGB color (3 bytes + 1 padding)
  data.push_back(255);
  data.push_back(0);
  data.push_back(0);
  data.push_back(0); // padding
  // Center
  appendFloat(data, 0.0f);
  appendFloat(data, 1.0f);
  appendFloat(data, 0.0f);
  // Extent
  appendFloat(data, 5.0f);
  appendFloat(data, 5.0f);
  appendFloat(data, 5.0f);

  std::string error;
  auto result = Loader::loadFromMemory(data.data(), data.size(), &error);

  ASSERT_TRUE(result.has_value()) << "Load failed: " << error;
  ASSERT_EQ(result->boxes.size(), 1);
  EXPECT_EQ(result->boxes[0].name, "CollisionBox");
  EXPECT_FLOAT_EQ(result->boxes[0].center.y, 1.0f);
  EXPECT_FLOAT_EQ(result->boxes[0].extent.x, 5.0f);
}

// =============================================================================
// Performance / Stress Tests
// =============================================================================

TEST_F(LoaderTest, LoadAllAvailableFixtures) {
  if (!fixturesAvailable()) {
    GTEST_SKIP() << "Test fixtures not available";
  }

  int totalFiles = 0;
  int loadedFiles = 0;
  int failedFiles = 0;

  for (const auto &entry : fs::directory_iterator(fixturesDir())) {
    if (entry.path().extension() == ".w3d") {
      totalFiles++;

      std::string error;
      auto result = Loader::load(entry.path(), &error);

      if (result.has_value()) {
        loadedFiles++;
      } else {
        failedFiles++;
        // Only print first few failures
        if (failedFiles <= 3) {
          std::cerr << "Failed to load " << entry.path().filename() << ": " << error << std::endl;
        }
      }
    }
  }

  std::cout << "Loaded " << loadedFiles << "/" << totalFiles << " W3D files" << std::endl;

  // Most files should load successfully
  if (totalFiles > 0) {
    double successRate = static_cast<double>(loadedFiles) / totalFiles;
    EXPECT_GT(successRate, 0.9) << "Less than 90% of files loaded successfully";
  }
}
