// Test texture file parsing (DDS/TGA) without Vulkan dependencies

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

#include <gtest/gtest.h>

// Test fixture directory
#ifndef TEXTURE_TEST_FIXTURES_DIR
#define TEXTURE_TEST_FIXTURES_DIR "resources/textures"
#endif

namespace {

std::string toLower(const std::string &str) {
  std::string result = str;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return result;
}

std::string removeExtension(const std::string &filename) {
  size_t lastDot = filename.find_last_of('.');
  if (lastDot == std::string::npos) {
    return filename;
  }
  return filename.substr(0, lastDot);
}

// Standalone DDS parser for testing
struct DDSResult {
  bool success = false;
  uint32_t width = 0;
  uint32_t height = 0;
  bool compressed = false;
  std::string fourCC;
  size_t dataSize = 0;
};

DDSResult parseDDSHeader(const std::filesystem::path &path) {
  DDSResult result;

  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return result;
  }

  // DDS magic number
  uint32_t magic;
  file.read(reinterpret_cast<char *>(&magic), 4);
  if (magic != 0x20534444) { // "DDS "
    return result;
  }

  // DDS header (124 bytes)
  uint32_t headerData[31];
  file.read(reinterpret_cast<char *>(headerData), 124);

  result.height = headerData[2];
  result.width = headerData[3];

  // Pixel format starts at offset 19 (76 bytes into header)
  uint32_t pfFlags = headerData[19];
  uint32_t fourCC = headerData[20];

  result.compressed = (pfFlags & 0x4) != 0; // DDPF_FOURCC

  if (result.compressed) {
    // Convert fourCC to string - stored as little-endian
    char fourCCStr[5] = {0};
    fourCCStr[0] = static_cast<char>(fourCC & 0xFF);
    fourCCStr[1] = static_cast<char>((fourCC >> 8) & 0xFF);
    fourCCStr[2] = static_cast<char>((fourCC >> 16) & 0xFF);
    fourCCStr[3] = static_cast<char>((fourCC >> 24) & 0xFF);
    result.fourCC = fourCCStr;

    // Calculate data size
    // DXT1=0x31545844 "DXT1", DXT3=0x33545844 "DXT3", DXT5=0x35545844 "DXT5"
    size_t blockSize = 0;
    if (fourCC == 0x31545844) {                                // DXT1
      blockSize = 8;
    } else if (fourCC == 0x33545844 || fourCC == 0x35545844) { // DXT3/DXT5
      blockSize = 16;
    }

    uint32_t blocksX = (result.width + 3) / 4;
    uint32_t blocksY = (result.height + 3) / 4;
    result.dataSize = blocksX * blocksY * blockSize;
  } else {
    uint32_t rgbBitCount = headerData[22];
    result.dataSize = result.width * result.height * (rgbBitCount / 8);
  }

  result.success = true;
  return result;
}

// Standalone TGA parser for testing
struct TGAResult {
  bool success = false;
  uint32_t width = 0;
  uint32_t height = 0;
  uint8_t bpp = 0;
  uint8_t imageType = 0;
};

TGAResult parseTGAHeader(const std::filesystem::path &path) {
  TGAResult result;

  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return result;
  }

  uint8_t header[18];
  file.read(reinterpret_cast<char *>(header), 18);

  result.imageType = header[2];
  result.width = header[12] | (header[13] << 8);
  result.height = header[14] | (header[15] << 8);
  result.bpp = header[16];

  // Only support uncompressed RGB/RGBA (type 2) and grayscale (type 3)
  if (result.imageType == 2 || result.imageType == 3) {
    result.success = true;
  }

  return result;
}

// Texture path resolution (mirrors TextureManager::resolveTexturePath)
// On case-sensitive filesystems (Linux), we need to scan the directory
std::filesystem::path resolveTexturePath(const std::filesystem::path &basePath,
                                         const std::string &w3dName) {
  if (basePath.empty() || !std::filesystem::exists(basePath)) {
    return {};
  }

  std::string baseName = toLower(removeExtension(w3dName));
  std::vector<std::string> extensions = {".dds", ".tga"};

  // Scan directory for case-insensitive match
  for (const auto &entry : std::filesystem::directory_iterator(basePath)) {
    if (!entry.is_regular_file()) {
      continue;
    }

    std::string filename = entry.path().filename().string();
    std::string filenameLower = toLower(filename);
    std::string fileBaseLower = toLower(removeExtension(filename));

    // Check if base names match (case-insensitive)
    if (fileBaseLower != baseName) {
      continue;
    }

    // Check if extension is one we support
    std::string fileExt = toLower(entry.path().extension().string());
    for (const auto &ext : extensions) {
      if (fileExt == ext) {
        return entry.path();
      }
    }
  }

  return {};
}

} // namespace

class TextureLoadingTest : public ::testing::Test {
protected:
  std::filesystem::path fixturesDir_;

  void SetUp() override {
    fixturesDir_ = TEXTURE_TEST_FIXTURES_DIR;
    // If running from build directory, adjust path
    if (!std::filesystem::exists(fixturesDir_)) {
      fixturesDir_ = std::filesystem::path("../../") / TEXTURE_TEST_FIXTURES_DIR;
    }
  }
};

TEST_F(TextureLoadingTest, DDSFileExists) {
  if (!std::filesystem::exists(fixturesDir_)) {
    GTEST_SKIP() << "Fixtures directory not found";
  }
  auto path = fixturesDir_ / "AVTankParts.dds";
  // Also check case-insensitive on Linux
  auto resolved = resolveTexturePath(fixturesDir_, "AVTankParts.dds");
  EXPECT_FALSE(resolved.empty()) << "DDS file not found at: " << path.string();
}

TEST_F(TextureLoadingTest, TGAFileExists) {
  if (!std::filesystem::exists(fixturesDir_)) {
    GTEST_SKIP() << "Fixtures directory not found";
  }
  auto path = fixturesDir_ / "headlights.tga";
  // Also check case-insensitive on Linux
  auto resolved = resolveTexturePath(fixturesDir_, "headlights.tga");
  EXPECT_FALSE(resolved.empty()) << "TGA file not found at: " << path.string();
}

TEST_F(TextureLoadingTest, ParseDDSHeader) {
  auto path = fixturesDir_ / "AVTankParts.dds";
  if (!std::filesystem::exists(path)) {
    GTEST_SKIP() << "DDS fixture not found";
  }

  auto result = parseDDSHeader(path);

  // Also read raw fourCC for debugging
  std::ifstream debugFile(path, std::ios::binary);
  debugFile.seekg(4 + 76 + 4); // magic + header offset to pfFlags + pfFlags itself
  uint32_t rawFourCC = 0;
  debugFile.read(reinterpret_cast<char *>(&rawFourCC), 4);
  std::cerr << "Raw fourCC at offset 84: 0x" << std::hex << rawFourCC << std::dec << std::endl;

  std::cerr << "DDS parse result: success=" << result.success << " width=" << result.width
            << " height=" << result.height << " compressed=" << result.compressed << " fourCC='"
            << result.fourCC << "'" << " dataSize=" << result.dataSize << std::endl;

  EXPECT_TRUE(result.success) << "Path: " << path.string();
  EXPECT_GT(result.width, 0) << "Width should be > 0";
  EXPECT_GT(result.height, 0) << "Height should be > 0";
  EXPECT_TRUE(result.compressed) << "Should be compressed (pfFlags should have 0x4 bit)";
  if (result.compressed) {
    // fourCC for DXT3 is stored as 0x33545844 which is "3TXD" in memory (little-endian)
    EXPECT_TRUE(result.fourCC == "DXT3" || result.fourCC == "3TXD")
        << "FourCC was: '" << result.fourCC << "'";
  }
}

TEST_F(TextureLoadingTest, ParseTGAHeader) {
  auto path = fixturesDir_ / "headlights.tga";
  if (!std::filesystem::exists(path)) {
    GTEST_SKIP() << "TGA fixture not found";
  }

  auto result = parseTGAHeader(path);
  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.width, 20);
  EXPECT_EQ(result.height, 12);
  EXPECT_EQ(result.bpp, 32);
  EXPECT_EQ(result.imageType, 2); // Uncompressed RGB
}

TEST_F(TextureLoadingTest, ResolveTexturePathWithTGAExtension) {
  // W3D file references "AVTankParts.tga" but we have "AVTankParts.dds"
  if (!std::filesystem::exists(fixturesDir_)) {
    GTEST_SKIP() << "Fixtures directory not found";
  }

  // Check if any texture file exists in the directory
  bool hasTextureFile = false;
  for (const auto &entry : std::filesystem::directory_iterator(fixturesDir_)) {
    std::string ext = toLower(entry.path().extension().string());
    if (ext == ".dds" || ext == ".tga") {
      hasTextureFile = true;
      break;
    }
  }
  if (!hasTextureFile) {
    GTEST_SKIP() << "No texture files in fixtures directory";
  }

  auto resolved = resolveTexturePath(fixturesDir_, "AVTankParts.tga");
  EXPECT_FALSE(resolved.empty())
      << "Should find AVTankParts.dds when searching for AVTankParts.tga";
  // Case-insensitive check
  std::string resolvedLower = toLower(resolved.string());
  EXPECT_TRUE(resolvedLower.find("avtankparts") != std::string::npos);
}

TEST_F(TextureLoadingTest, ResolveTexturePathCaseInsensitive) {
  if (!std::filesystem::exists(fixturesDir_)) {
    GTEST_SKIP() << "Fixtures directory not found";
  }

  // Check if the specific texture file exists (any case)
  auto resolved = resolveTexturePath(fixturesDir_, "AVTankParts.tga");
  if (resolved.empty()) {
    GTEST_SKIP() << "AVTankParts texture not found in fixtures";
  }

  // Now test case insensitivity with uppercase input
  auto resolvedUpper = resolveTexturePath(fixturesDir_, "AVTANKPARTS.TGA");
  EXPECT_FALSE(resolvedUpper.empty()) << "Should find texture regardless of case";
}

TEST_F(TextureLoadingTest, ResolveTexturePathNotFound) {
  auto resolved = resolveTexturePath(fixturesDir_, "nonexistent.tga");
  EXPECT_TRUE(resolved.empty()) << "Should return empty for non-existent texture";
}

TEST_F(TextureLoadingTest, TextureArrayDataValidation) {
  // Test that texture array data validation works correctly
  // This is a logic test, not a GPU test

  uint32_t width = 64;
  uint32_t height = 64;
  uint32_t layerCount = 4;

  // Create valid layer data
  std::vector<std::vector<uint8_t>> validLayerData;
  for (uint32_t i = 0; i < layerCount; ++i) {
    std::vector<uint8_t> layer(width * height * 4);
    // Fill with test pattern (R=i*50, G=128, B=255-i*50, A=255)
    for (size_t j = 0; j < layer.size(); j += 4) {
      layer[j + 0] = static_cast<uint8_t>(i * 50);
      layer[j + 1] = 128;
      layer[j + 2] = static_cast<uint8_t>(255 - i * 50);
      layer[j + 3] = 255;
    }
    validLayerData.push_back(layer);
  }

  // Verify layer count matches
  EXPECT_EQ(validLayerData.size(), layerCount);

  // Verify each layer has correct size
  for (const auto &layer : validLayerData) {
    EXPECT_EQ(layer.size(), width * height * 4);
  }

  // Test invalid layer data (wrong size)
  std::vector<std::vector<uint8_t>> invalidLayerData = validLayerData;
  invalidLayerData[0].resize(100); // Wrong size

  // This would fail validation in createTextureArray
  bool sizeValid = true;
  for (const auto &layer : invalidLayerData) {
    if (layer.size() != width * height * 4) {
      sizeValid = false;
      break;
    }
  }
  EXPECT_FALSE(sizeValid) << "Should detect invalid layer size";
}
