#include <gtest/gtest.h>

#include "core/settings.hpp"

#include <filesystem>
#include <fstream>

using namespace w3d;

class SettingsTest : public ::testing::Test {
protected:
  std::filesystem::path tempDir;
  std::filesystem::path tempSettingsPath;

  void SetUp() override {
    tempDir = std::filesystem::temp_directory_path() / "w3d_settings_test";
    std::filesystem::create_directories(tempDir);
    tempSettingsPath = tempDir / "settings.json";
  }

  void TearDown() override {
    std::error_code ec;
    std::filesystem::remove_all(tempDir, ec);
  }
};

TEST_F(SettingsTest, DefaultValuesAreReasonable) {
  Settings s;
  EXPECT_EQ(s.windowWidth, 1280);
  EXPECT_EQ(s.windowHeight, 720);
  EXPECT_TRUE(s.showMesh);
  EXPECT_TRUE(s.showSkeleton);
  EXPECT_TRUE(s.texturePath.empty());
  EXPECT_TRUE(s.lastBrowsedDirectory.empty());
}

TEST_F(SettingsTest, SaveAndLoadRoundTrip) {
  Settings original;
  original.texturePath = "/some/path/to/textures";
  original.lastBrowsedDirectory = "/another/path";
  original.windowWidth = 1920;
  original.windowHeight = 1080;
  original.showMesh = false;
  original.showSkeleton = true;

  ASSERT_TRUE(original.save(tempSettingsPath));
  ASSERT_TRUE(std::filesystem::exists(tempSettingsPath));

  Settings restored = Settings::load(tempSettingsPath);

  EXPECT_EQ(restored.texturePath, original.texturePath);
  EXPECT_EQ(restored.lastBrowsedDirectory, original.lastBrowsedDirectory);
  EXPECT_EQ(restored.windowWidth, original.windowWidth);
  EXPECT_EQ(restored.windowHeight, original.windowHeight);
  EXPECT_EQ(restored.showMesh, original.showMesh);
  EXPECT_EQ(restored.showSkeleton, original.showSkeleton);
}

TEST_F(SettingsTest, LoadNonexistentFileReturnsDefaults) {
  Settings s = Settings::load(tempDir / "nonexistent.json");
  EXPECT_EQ(s.windowWidth, 1280); // Default value
  EXPECT_EQ(s.windowHeight, 720); // Default value
}

TEST_F(SettingsTest, LoadMalformedJsonReturnsDefaults) {
  // Write invalid JSON
  std::ofstream file(tempSettingsPath);
  file << "{ invalid json content }}}";
  file.close();

  Settings s = Settings::load(tempSettingsPath);
  EXPECT_EQ(s.windowWidth, 1280); // Default value
}

TEST_F(SettingsTest, LoadPartialJsonUsesDefaultsForMissingFields) {
  // Write partial JSON (only window section)
  std::ofstream file(tempSettingsPath);
  file << R"({
    "window": {
      "width": 800,
      "height": 600
    }
  })";
  file.close();

  Settings s = Settings::load(tempSettingsPath);
  EXPECT_EQ(s.windowWidth, 800);
  EXPECT_EQ(s.windowHeight, 600);
  EXPECT_TRUE(s.texturePath.empty());    // Default
  EXPECT_TRUE(s.showMesh);               // Default
}

TEST_F(SettingsTest, SaveCreatesParentDirectories) {
  auto nestedPath = tempDir / "nested" / "dir" / "settings.json";
  Settings s;
  s.windowWidth = 999;

  ASSERT_TRUE(s.save(nestedPath));
  EXPECT_TRUE(std::filesystem::exists(nestedPath));

  Settings restored = Settings::load(nestedPath);
  EXPECT_EQ(restored.windowWidth, 999);
}

TEST_F(SettingsTest, SavedJsonIsHumanReadable) {
  Settings s;
  s.texturePath = "C:/Games/Generals/Textures";
  s.save(tempSettingsPath);

  std::ifstream file(tempSettingsPath);
  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());

  // Check that JSON is pretty-printed (has newlines and indentation)
  EXPECT_NE(content.find('\n'), std::string::npos);
  EXPECT_NE(content.find("  "), std::string::npos);
  EXPECT_NE(content.find("texture_path"), std::string::npos);
}
