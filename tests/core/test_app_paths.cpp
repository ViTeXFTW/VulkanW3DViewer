#include <gtest/gtest.h>

#include "core/app_paths.hpp"

#include <filesystem>

using namespace w3d;

TEST(AppPathsTest, AppDataDirReturnsValidPath) {
  auto path = AppPaths::appDataDir();
  ASSERT_TRUE(path.has_value());
  EXPECT_TRUE(path->is_absolute());
  EXPECT_TRUE(path->string().find("VulkanW3DViewer") != std::string::npos);
}

TEST(AppPathsTest, ImguiIniPathHasCorrectFilename) {
  auto path = AppPaths::imguiIniPath();
  ASSERT_TRUE(path.has_value());
  EXPECT_EQ(path->filename(), "imgui.ini");
}

TEST(AppPathsTest, SettingsFilePathHasCorrectFilename) {
  auto path = AppPaths::settingsFilePath();
  ASSERT_TRUE(path.has_value());
  EXPECT_EQ(path->filename(), "settings.json");
}

TEST(AppPathsTest, EnsureAppDataDirCreatesDirectory) {
  ASSERT_TRUE(AppPaths::ensureAppDataDir());
  auto path = AppPaths::appDataDir();
  ASSERT_TRUE(path.has_value());
  EXPECT_TRUE(std::filesystem::exists(*path));
}

TEST(AppPathsTest, AppNameIsCorrect) {
  EXPECT_STREQ(AppPaths::kAppName, "VulkanW3DViewer");
}
