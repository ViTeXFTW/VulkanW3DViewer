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
std::filesystem::path resolveTexturePath(const std::filesystem::path &basePath,
                                         const std::string &w3dName) {
  if (basePath.empty()) {
    return {};
  }

  std::string baseName = toLower(removeExtension(w3dName));
  std::vector<std::string> extensions = {".dds", ".tga", ".DDS", ".TGA"};

  for (const auto &ext : extensions) {
    // Try lowercase name
    std::filesystem::path path = basePath / (baseName + ext);
    if (std::filesystem::exists(path)) {
      return path;
    }

    // Try original case name
    std::string origBase = removeExtension(w3dName);
    path = basePath / (origBase + ext);
    if (std::filesystem::exists(path)) {
      return path;
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
  auto path = fixturesDir_ / "AVTankParts.dds";
  EXPECT_TRUE(std::filesystem::exists(path)) << "DDS file not found at: " << path.string();
}

TEST_F(TextureLoadingTest, TGAFileExists) {
  auto path = fixturesDir_ / "headlights.tga";
  EXPECT_TRUE(std::filesystem::exists(path)) << "TGA file not found at: " << path.string();
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
  std::cerr << "Fixtures dir: " << fixturesDir_.string() << std::endl;
  std::cerr << "Exists: " << std::filesystem::exists(fixturesDir_) << std::endl;

  // List files in directory
  if (std::filesystem::exists(fixturesDir_)) {
    for (const auto &entry : std::filesystem::directory_iterator(fixturesDir_)) {
      std::cerr << "  File: " << entry.path().filename().string() << std::endl;
    }
  }

  auto resolved = resolveTexturePath(fixturesDir_, "AVTankParts.tga");
  std::cerr << "Resolved: " << resolved.string() << std::endl;
  EXPECT_FALSE(resolved.empty())
      << "Should find AVTankParts.dds when searching for AVTankParts.tga";
  // Case-insensitive check
  std::string resolvedLower = toLower(resolved.string());
  EXPECT_TRUE(resolvedLower.find("avtankparts") != std::string::npos);
}

TEST_F(TextureLoadingTest, ResolveTexturePathCaseInsensitive) {
  // Test case insensitivity
  auto resolved = resolveTexturePath(fixturesDir_, "AVTANKPARTS.TGA");
  EXPECT_FALSE(resolved.empty()) << "Should find texture regardless of case";
}

TEST_F(TextureLoadingTest, ResolveTexturePathNotFound) {
  auto resolved = resolveTexturePath(fixturesDir_, "nonexistent.tga");
  EXPECT_TRUE(resolved.empty()) << "Should return empty for non-existent texture";
}
