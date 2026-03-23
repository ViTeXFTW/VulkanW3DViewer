#include "ui/map_browser.hpp"
#include "ui/ui_manager.hpp"

#include <gtest/gtest.h>

// Stub implementations for UIManager methods defined in ui_manager.cpp
// (we don't compile ui_manager.cpp to avoid heavy ImGui/SettingsWindow deps)
namespace w3d {
void UIManager::draw(UIContext &) {}
void UIManager::drawDockspace() {}
void UIManager::drawMenuBar(UIContext &) {}
} // namespace w3d

using namespace w3d;

// ── MapBrowser unit tests ──────────────────────────────────────────────────

class MapBrowserTest : public ::testing::Test {
protected:
  MapBrowser browser;
};

TEST_F(MapBrowserTest, DefaultState) {
  EXPECT_FALSE(browser.isVisible());
  EXPECT_EQ(browser.selectedIndex(), -1);
  EXPECT_TRUE(browser.searchText().empty());
  EXPECT_FALSE(browser.isBigArchiveMode());
}

TEST_F(MapBrowserTest, NameIsMapBrowser) {
  EXPECT_STREQ(browser.name(), "Map Browser");
}

TEST_F(MapBrowserTest, SetBigArchiveMode) {
  browser.setBigArchiveMode(true);
  EXPECT_TRUE(browser.isBigArchiveMode());
  browser.setBigArchiveMode(false);
  EXPECT_FALSE(browser.isBigArchiveMode());
}

TEST_F(MapBrowserTest, SetAvailableMaps) {
  std::vector<std::string> maps = {"maps/alpine/alpine", "maps/desert/desert"};
  browser.setAvailableMaps(maps);
  EXPECT_EQ(browser.selectedIndex(), -1);
}

TEST_F(MapBrowserTest, CallbackIsStored) {
  std::string selectedMap;
  browser.setMapSelectedCallback([&](const std::string &name) { selectedMap = name; });
  EXPECT_TRUE(selectedMap.empty());
}

TEST_F(MapBrowserTest, VisibilityToggle) {
  EXPECT_FALSE(browser.isVisible());
  browser.setVisible(true);
  EXPECT_TRUE(browser.isVisible());
  browser.toggleVisible();
  EXPECT_FALSE(browser.isVisible());
}

TEST_F(MapBrowserTest, ShowInViewMenu) {
  EXPECT_TRUE(browser.showInViewMenu());
}

// ── UIManager addWindowInstance tests ──────────────────────────────────────

class UIManagerTest : public ::testing::Test {
protected:
  UIManager manager;
};

TEST_F(UIManagerTest, AddWindowInstance) {
  auto browser = std::make_unique<MapBrowser>();
  auto *ptr = manager.addWindowInstance(std::move(browser));
  ASSERT_NE(ptr, nullptr);
  EXPECT_STREQ(ptr->name(), "Map Browser");
  EXPECT_EQ(manager.windowCount(), 1);
}

TEST_F(UIManagerTest, AddWindowInstanceDoesNotRegisterByType) {
  auto browser = std::make_unique<MapBrowser>();
  manager.addWindowInstance(std::move(browser));

  auto *found = manager.getWindow<MapBrowser>();
  EXPECT_EQ(found, nullptr);
}

TEST_F(UIManagerTest, AddWindowByTemplateRegistersType) {
  auto *ptr = manager.addWindow<MapBrowser>();
  ASSERT_NE(ptr, nullptr);

  auto *found = manager.getWindow<MapBrowser>();
  EXPECT_EQ(found, ptr);
}

TEST_F(UIManagerTest, MultipleInstancesSameType) {
  auto *first = manager.addWindow<MapBrowser>();

  auto second = std::make_unique<MapBrowser>();
  auto *secondPtr = static_cast<MapBrowser *>(manager.addWindowInstance(std::move(second)));

  EXPECT_NE(first, secondPtr);
  EXPECT_EQ(manager.windowCount(), 2);
  EXPECT_EQ(manager.getWindow<MapBrowser>(), first);
}

// ── RenderState tests ──────────────────────────────────────────────────────

#include "core/render_state.hpp"

TEST(RenderStateTest, DefaultMode) {
  RenderState state;
  EXPECT_EQ(state.mode, ViewerMode::ModelViewer);
}

TEST(RenderStateTest, DefaultMapLayerToggles) {
  RenderState state;
  EXPECT_TRUE(state.showTerrain);
  EXPECT_TRUE(state.showWater);
  EXPECT_TRUE(state.showObjects);
  EXPECT_FALSE(state.showTriggers);
}

TEST(RenderStateTest, SwitchToMapMode) {
  RenderState state;
  state.mode = ViewerMode::MapViewer;
  EXPECT_EQ(state.mode, ViewerMode::MapViewer);
}

// ── UIContext tests ────────────────────────────────────────────────────────

#include "ui/ui_context.hpp"

TEST(UIContextTest, HasMapFalseByDefault) {
  UIContext ctx;
  EXPECT_FALSE(ctx.hasMap());
}

TEST(UIContextTest, IsMapModeFalseByDefault) {
  UIContext ctx;
  EXPECT_FALSE(ctx.isMapMode());
}

TEST(UIContextTest, IsMapModeWithRenderState) {
  RenderState state;
  state.mode = ViewerMode::MapViewer;

  UIContext ctx;
  ctx.renderState = &state;

  EXPECT_TRUE(ctx.isMapMode());
}
