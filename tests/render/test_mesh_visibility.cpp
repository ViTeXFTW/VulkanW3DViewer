#include <gtest/gtest.h>
#include <vector>

// Unit tests for mesh visibility logic
// Note: Full HLodModel tests require VulkanContext, so we test the state logic patterns

namespace {

// Simulate the visibility state management logic from HLodModel
class MockVisibilityState {
public:
  void initialize(size_t count) { visibility_.assign(count, true); }

  bool isHidden(size_t index) const {
    if (index >= visibility_.size()) {
      return false;
    }
    return !visibility_[index];
  }

  void setHidden(size_t index, bool hidden) {
    if (index >= visibility_.size()) {
      return;
    }
    visibility_[index] = !hidden;
  }

  void setAllHidden(bool hidden) {
    std::fill(visibility_.begin(), visibility_.end(), !hidden);
  }

  size_t size() const { return visibility_.size(); }
  bool isVisible(size_t index) const { return index < visibility_.size() && visibility_[index]; }

private:
  std::vector<bool> visibility_;
};

} // namespace

// =============================================================================
// Visibility State Initialization Tests
// =============================================================================

TEST(MeshVisibilityTest, InitializationAllVisible) {
  MockVisibilityState state;
  state.initialize(5);

  // All meshes should be visible after initialization
  for (size_t i = 0; i < 5; ++i) {
    EXPECT_FALSE(state.isHidden(i)) << "Mesh " << i << " should not be hidden";
    EXPECT_TRUE(state.isVisible(i)) << "Mesh " << i << " should be visible";
  }
}

TEST(MeshVisibilityTest, InitializationEmptyModel) {
  MockVisibilityState state;
  state.initialize(0);

  EXPECT_EQ(state.size(), 0);
  // Out of bounds should return false for isHidden
  EXPECT_FALSE(state.isHidden(0));
}

// =============================================================================
// Individual Mesh Visibility Toggle Tests
// =============================================================================

TEST(MeshVisibilityTest, HideSingleMesh) {
  MockVisibilityState state;
  state.initialize(5);

  state.setHidden(2, true);

  EXPECT_FALSE(state.isHidden(0));
  EXPECT_FALSE(state.isHidden(1));
  EXPECT_TRUE(state.isHidden(2));
  EXPECT_FALSE(state.isHidden(3));
  EXPECT_FALSE(state.isHidden(4));
}

TEST(MeshVisibilityTest, ShowHiddenMesh) {
  MockVisibilityState state;
  state.initialize(5);

  state.setHidden(2, true);
  EXPECT_TRUE(state.isHidden(2));

  state.setHidden(2, false);
  EXPECT_FALSE(state.isHidden(2));
}

TEST(MeshVisibilityTest, HideOutOfBoundsMesh) {
  MockVisibilityState state;
  state.initialize(3);

  // Should not crash, and isHidden should return false for out of bounds
  state.setHidden(10, true);
  EXPECT_FALSE(state.isHidden(10));
}

// =============================================================================
// Bulk Visibility Toggle Tests
// =============================================================================

TEST(MeshVisibilityTest, HideAllMeshes) {
  MockVisibilityState state;
  state.initialize(5);

  state.setAllHidden(true);

  for (size_t i = 0; i < 5; ++i) {
    EXPECT_TRUE(state.isHidden(i)) << "Mesh " << i << " should be hidden";
  }
}

TEST(MeshVisibilityTest, ShowAllMeshes) {
  MockVisibilityState state;
  state.initialize(5);

  // First hide some
  state.setHidden(1, true);
  state.setHidden(3, true);

  // Then show all
  state.setAllHidden(false);

  for (size_t i = 0; i < 5; ++i) {
    EXPECT_FALSE(state.isHidden(i)) << "Mesh " << i << " should be visible";
  }
}

TEST(MeshVisibilityTest, HideAllThenShowSome) {
  MockVisibilityState state;
  state.initialize(5);

  state.setAllHidden(true);
  state.setHidden(0, false);
  state.setHidden(4, false);

  EXPECT_FALSE(state.isHidden(0));
  EXPECT_TRUE(state.isHidden(1));
  EXPECT_TRUE(state.isHidden(2));
  EXPECT_TRUE(state.isHidden(3));
  EXPECT_FALSE(state.isHidden(4));
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST(MeshVisibilityTest, RepeatedToggle) {
  MockVisibilityState state;
  state.initialize(3);

  // Toggle same mesh multiple times
  state.setHidden(1, true);
  state.setHidden(1, true);  // Already hidden
  state.setHidden(1, false);
  state.setHidden(1, false); // Already visible
  state.setHidden(1, true);

  EXPECT_TRUE(state.isHidden(1));
}

TEST(MeshVisibilityTest, SingleMeshModel) {
  MockVisibilityState state;
  state.initialize(1);

  EXPECT_FALSE(state.isHidden(0));

  state.setHidden(0, true);
  EXPECT_TRUE(state.isHidden(0));

  state.setAllHidden(false);
  EXPECT_FALSE(state.isHidden(0));
}
