#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "lib/scene/scene_graph.hpp"
#include "lib/scene/scene_node.hpp"

#include <gtest/gtest.h>

using namespace w3d::scene;
using namespace w3d::gfx;

namespace {

class ConcreteNode : public SceneNode {
public:
  explicit ConcreteNode(const char *name) : name_(name) {}
  const char *typeName() const override { return name_; }

private:
  const char *name_;
};

BoundingBox makeBox(float cx, float cy, float cz, float half = 5.0f) {
  BoundingBox b;
  b.expand(glm::vec3(cx - half, cy - half, cz - half));
  b.expand(glm::vec3(cx + half, cy + half, cz + half));
  return b;
}

} // namespace

class SceneGraphTest : public ::testing::Test {
protected:
  SceneGraph graph_{0.0f, 0.0f, 2000.0f, 2000.0f};
};

TEST_F(SceneGraphTest, EmptyGraphHasZeroNodes) {
  EXPECT_EQ(graph_.nodeCount(), 0u);
}

TEST_F(SceneGraphTest, AddNodeIncrementsCount) {
  auto node = std::make_unique<ConcreteNode>("A");
  (void)graph_.addNode(std::move(node));
  EXPECT_EQ(graph_.nodeCount(), 1u);
}

TEST_F(SceneGraphTest, AddMultipleNodesIncrementsCount) {
  (void)graph_.addNode(std::make_unique<ConcreteNode>("A"));
  (void)graph_.addNode(std::make_unique<ConcreteNode>("B"));
  (void)graph_.addNode(std::make_unique<ConcreteNode>("C"));
  EXPECT_EQ(graph_.nodeCount(), 3u);
}

TEST_F(SceneGraphTest, AddNodeReturnsNonNullPointer) {
  auto node = std::make_unique<ConcreteNode>("A");
  SceneNode *ptr = graph_.addNode(std::move(node));
  EXPECT_NE(ptr, nullptr);
}

TEST_F(SceneGraphTest, ClearRemovesAllNodes) {
  (void)graph_.addNode(std::make_unique<ConcreteNode>("A"));
  (void)graph_.addNode(std::make_unique<ConcreteNode>("B"));
  graph_.clear();
  EXPECT_EQ(graph_.nodeCount(), 0u);
}

TEST_F(SceneGraphTest, QueryAllReturnsAllNodes) {
  (void)graph_.addNode(std::make_unique<ConcreteNode>("A"));
  (void)graph_.addNode(std::make_unique<ConcreteNode>("B"));
  (void)graph_.addNode(std::make_unique<ConcreteNode>("C"));

  std::vector<SceneNode *> result;
  graph_.queryAll(result);
  EXPECT_EQ(result.size(), 3u);
}

TEST_F(SceneGraphTest, QueryAllOnEmptyReturnsEmpty) {
  std::vector<SceneNode *> result;
  graph_.queryAll(result);
  EXPECT_TRUE(result.empty());
}

TEST_F(SceneGraphTest, QueryVisibleReturnsOnlyVisibleNodes) {
  glm::mat4 view = glm::lookAt(glm::vec3(1000.0f, 500.0f, 1000.0f),
                               glm::vec3(1000.0f, 0.0f, 900.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  glm::mat4 proj = glm::perspective(glm::radians(60.0f), 1.77f, 1.0f, 2000.0f);
  Frustum frustum;
  frustum.extractFromVP(proj * view);

  auto nearNode = std::make_unique<ConcreteNode>("near");
  nearNode->setPosition({1000.0f, 0.0f, 950.0f});
  nearNode->setLocalBounds(makeBox(0.0f, 0.0f, 0.0f, 10.0f));
  (void)graph_.addNode(std::move(nearNode));

  auto farNode = std::make_unique<ConcreteNode>("far");
  farNode->setPosition({1900.0f, 0.0f, 1900.0f});
  farNode->setLocalBounds(makeBox(0.0f, 0.0f, 0.0f, 10.0f));
  (void)graph_.addNode(std::move(farNode));

  std::vector<SceneNode *> visible;
  graph_.queryVisible(frustum, visible);
  EXPECT_GE(visible.size(), 1u);
  EXPECT_LT(visible.size(), 3u);
}

TEST_F(SceneGraphTest, InvisibleNodesExcludedFromQuery) {
  glm::mat4 view = glm::lookAt(glm::vec3(500.0f, 200.0f, 500.0f), glm::vec3(500.0f, 0.0f, 400.0f),
                               glm::vec3(0.0f, 1.0f, 0.0f));
  glm::mat4 proj = glm::perspective(glm::radians(60.0f), 1.77f, 1.0f, 2000.0f);
  Frustum frustum;
  frustum.extractFromVP(proj * view);

  auto visNode = std::make_unique<ConcreteNode>("vis");
  visNode->setPosition({500.0f, 0.0f, 450.0f});
  visNode->setLocalBounds(makeBox(0.0f, 0.0f, 0.0f, 10.0f));
  SceneNode *visPtr = graph_.addNode(std::move(visNode));

  auto hidNode = std::make_unique<ConcreteNode>("hid");
  hidNode->setPosition({500.0f, 0.0f, 450.0f});
  hidNode->setLocalBounds(makeBox(0.0f, 0.0f, 0.0f, 10.0f));
  hidNode->setVisible(false);
  (void)graph_.addNode(std::move(hidNode));

  std::vector<SceneNode *> visible;
  graph_.queryVisible(frustum, visible);

  bool foundVis = false;
  bool foundHid = false;
  for (auto *n : visible) {
    if (n == visPtr)
      foundVis = true;
    if (!n->isVisible())
      foundHid = true;
  }
  EXPECT_FALSE(foundHid);
  (void)foundVis;
}

TEST_F(SceneGraphTest, QueryAllExcludesHiddenNodes) {
  auto visible = std::make_unique<ConcreteNode>("vis");
  auto hidden = std::make_unique<ConcreteNode>("hid");
  hidden->setVisible(false);

  (void)graph_.addNode(std::move(visible));
  (void)graph_.addNode(std::move(hidden));

  std::vector<SceneNode *> result;
  graph_.queryAll(result);

  for (auto *n : result) {
    EXPECT_TRUE(n->isVisible());
  }
}
