#include <filesystem>
#include <fstream>

#include "ui/file_browser.hpp"

#include <gtest/gtest.h>

using namespace w3d;

class FileBrowserTest : public ::testing::Test {
protected:
  std::filesystem::path tempDir;
  FileBrowser browser;

  void SetUp() override {
    // Create a temporary directory structure for testing
    tempDir = std::filesystem::temp_directory_path() / "w3d_file_browser_test";
    std::filesystem::remove_all(tempDir); // Clean up any previous test run
    std::filesystem::create_directories(tempDir);

    // Create test directories
    std::filesystem::create_directories(tempDir / "subdir1");
    std::filesystem::create_directories(tempDir / "subdir2");
    std::filesystem::create_directories(tempDir / "empty_dir");

    // Create test files
    std::ofstream(tempDir / "test.w3d").close();
    std::ofstream(tempDir / "test.txt").close();
    std::ofstream(tempDir / "another.w3d").close();
    std::ofstream(tempDir / "subdir1" / "nested.w3d").close();
  }

  void TearDown() override {
    std::error_code ec;
    std::filesystem::remove_all(tempDir, ec);
  }
};

// BrowseMode tests

TEST_F(FileBrowserTest, DefaultModeIsFile) {
  EXPECT_EQ(browser.browseMode(), BrowseMode::File);
}

TEST_F(FileBrowserTest, CanSetDirectoryMode) {
  browser.setBrowseMode(BrowseMode::Directory);
  EXPECT_EQ(browser.browseMode(), BrowseMode::Directory);
}

// Navigation tests

TEST_F(FileBrowserTest, NavigateToValidDirectory) {
  browser.navigateTo(tempDir);
  EXPECT_EQ(browser.currentPath(), tempDir);
}

TEST_F(FileBrowserTest, NavigateToNonexistentDirectoryDoesNothing) {
  auto originalPath = browser.currentPath();
  browser.navigateTo(tempDir / "nonexistent");
  EXPECT_EQ(browser.currentPath(), originalPath);
}

TEST_F(FileBrowserTest, NavigateUpFromSubdirectory) {
  browser.navigateTo(tempDir / "subdir1");
  browser.navigateUp();
  EXPECT_EQ(browser.currentPath(), tempDir);
}

TEST_F(FileBrowserTest, NavigateUpFromRootDoesNothing) {
  auto rootPath = tempDir.root_path();
  browser.navigateTo(rootPath);
  browser.navigateUp();
  EXPECT_EQ(browser.currentPath(), rootPath);
}

TEST_F(FileBrowserTest, OpenAtDirectoryNavigatesToIt) {
  browser.openAt(tempDir / "subdir1");
  EXPECT_EQ(browser.currentPath(), tempDir / "subdir1");
}

TEST_F(FileBrowserTest, OpenAtFileNavigatesToParentDirectory) {
  browser.openAt(tempDir / "test.w3d");
  EXPECT_EQ(browser.currentPath(), tempDir);
}

// Directory listing tests

TEST_F(FileBrowserTest, RefreshDirectoryListsContents) {
  browser.navigateTo(tempDir);

  const auto &entries = browser.entries();
  EXPECT_FALSE(entries.empty());

  // Should have 3 directories and 3 files
  int dirCount = 0;
  int fileCount = 0;
  for (const auto &entry : entries) {
    if (entry.isDirectory) {
      dirCount++;
    } else {
      fileCount++;
    }
  }
  EXPECT_EQ(dirCount, 3); // subdir1, subdir2, empty_dir
  EXPECT_EQ(fileCount, 3); // test.w3d, test.txt, another.w3d
}

TEST_F(FileBrowserTest, DirectoriesListedBeforeFiles) {
  browser.navigateTo(tempDir);

  const auto &entries = browser.entries();
  ASSERT_FALSE(entries.empty());

  // Find first non-directory
  bool foundFile = false;
  for (const auto &entry : entries) {
    if (!entry.isDirectory) {
      foundFile = true;
    } else if (foundFile) {
      // Directory after a file - should not happen
      FAIL() << "Directory found after file in listing";
    }
  }
}

TEST_F(FileBrowserTest, EntriesAreSortedAlphabetically) {
  browser.navigateTo(tempDir);

  const auto &entries = browser.entries();

  // Check directories are sorted
  std::string prevDirName;
  for (const auto &entry : entries) {
    if (entry.isDirectory) {
      if (!prevDirName.empty()) {
        EXPECT_LT(prevDirName, entry.name) << "Directories not sorted";
      }
      prevDirName = entry.name;
    }
  }

  // Check files are sorted
  std::string prevFileName;
  for (const auto &entry : entries) {
    if (!entry.isDirectory) {
      if (!prevFileName.empty()) {
        EXPECT_LT(prevFileName, entry.name) << "Files not sorted";
      }
      prevFileName = entry.name;
    }
  }
}

// Filter tests (File mode only)

TEST_F(FileBrowserTest, FilterShowsOnlyMatchingFiles) {
  browser.setFilter(".w3d");
  browser.navigateTo(tempDir);

  const auto &entries = browser.entries();

  int w3dCount = 0;
  int otherFileCount = 0;
  for (const auto &entry : entries) {
    if (!entry.isDirectory) {
      if (entry.name.find(".w3d") != std::string::npos) {
        w3dCount++;
      } else {
        otherFileCount++;
      }
    }
  }

  EXPECT_EQ(w3dCount, 2); // test.w3d, another.w3d
  EXPECT_EQ(otherFileCount, 0); // test.txt should be filtered out
}

TEST_F(FileBrowserTest, FilterIsCaseInsensitive) {
  // Create a file with uppercase extension
  std::ofstream(tempDir / "upper.W3D").close();

  browser.setFilter(".w3d");
  browser.navigateTo(tempDir);

  const auto &entries = browser.entries();

  bool foundUpper = false;
  for (const auto &entry : entries) {
    if (entry.name == "upper.W3D") {
      foundUpper = true;
      break;
    }
  }

  EXPECT_TRUE(foundUpper) << "Case-insensitive filter should match .W3D";
}

TEST_F(FileBrowserTest, EmptyFilterShowsAllFiles) {
  browser.setFilter("");
  browser.navigateTo(tempDir);

  const auto &entries = browser.entries();

  int fileCount = 0;
  for (const auto &entry : entries) {
    if (!entry.isDirectory) {
      fileCount++;
    }
  }

  EXPECT_EQ(fileCount, 3); // All files
}

// Directory mode tests

TEST_F(FileBrowserTest, DirectoryModeShowsOnlyDirectories) {
  browser.setBrowseMode(BrowseMode::Directory);
  browser.navigateTo(tempDir);

  const auto &entries = browser.entries();

  for (const auto &entry : entries) {
    EXPECT_TRUE(entry.isDirectory) << "Non-directory found in Directory mode: " << entry.name;
  }

  EXPECT_EQ(entries.size(), 3); // subdir1, subdir2, empty_dir
}

TEST_F(FileBrowserTest, DirectoryModeIgnoresFileFilter) {
  browser.setBrowseMode(BrowseMode::Directory);
  browser.setFilter(".w3d"); // Should be ignored
  browser.navigateTo(tempDir);

  const auto &entries = browser.entries();

  // Should still show all directories
  EXPECT_EQ(entries.size(), 3);
}

// Selection tests

TEST_F(FileBrowserTest, InitialSelectionIsNone) {
  EXPECT_EQ(browser.selectedIndex(), -1);
}

TEST_F(FileBrowserTest, SelectEntryUpdatesIndex) {
  browser.navigateTo(tempDir);
  browser.selectEntry(0);
  EXPECT_EQ(browser.selectedIndex(), 0);
}

TEST_F(FileBrowserTest, SelectEntryOutOfRangeDoesNothing) {
  browser.navigateTo(tempDir);
  browser.selectEntry(100);
  EXPECT_EQ(browser.selectedIndex(), -1);
}

TEST_F(FileBrowserTest, NavigationResetsSelection) {
  browser.navigateTo(tempDir);
  browser.selectEntry(0);
  browser.navigateTo(tempDir / "subdir1");
  EXPECT_EQ(browser.selectedIndex(), -1);
}

// Callback tests

TEST_F(FileBrowserTest, SelectCurrentDirectoryTriggersCallback) {
  std::filesystem::path selectedPath;
  browser.setPathSelectedCallback([&selectedPath](const std::filesystem::path &path) {
    selectedPath = path;
  });

  browser.navigateTo(tempDir);
  browser.selectCurrentDirectory();

  EXPECT_EQ(selectedPath, tempDir);
}

TEST_F(FileBrowserTest, NoCallbackDoesNotCrash) {
  // No callback set
  browser.navigateTo(tempDir);
  browser.selectCurrentDirectory(); // Should not crash
}

// Title tests

TEST_F(FileBrowserTest, DefaultTitle) {
  EXPECT_STREQ(browser.name(), "File Browser");
}

TEST_F(FileBrowserTest, CustomTitle) {
  browser.setTitle("Select Texture Directory");
  EXPECT_STREQ(browser.name(), "Select Texture Directory");
}

// Edge case tests

TEST_F(FileBrowserTest, EmptyDirectoryHasNoEntries) {
  browser.navigateTo(tempDir / "empty_dir");
  EXPECT_TRUE(browser.entries().empty());
}

TEST_F(FileBrowserTest, NavigateToNonexistentPathPreservesCurrentPath) {
  browser.navigateTo(tempDir);
  auto path = browser.currentPath();
  browser.navigateTo("/nonexistent/path/that/does/not/exist");
  EXPECT_EQ(browser.currentPath(), path);
}
