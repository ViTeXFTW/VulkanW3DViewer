#include <glm/glm.hpp>

#include "lib/scene/quadtree.hpp"
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

BoundingBox makeBox(float cx, float cy, float cz, float half = 1.0f) {
  BoundingBox b;
  b.expand(glm::vec3(cx - half, cy - half, cz - half));
  b.expand(glm::vec3(cx + half, cy + half, cz + half));
  return b;
}

} // namespace

class QuadtreeTest : public ::testing::Test {
protected:
  Quadtree tree_{0.0f, 0.0f, 1000.0f, 1000.0f};
};

TEST_F(QuadtreeTest, EmptyTreeQueryReturnsEmpty) {
  std::vector<SceneNode *> result;
  tree_.query(Quadtree::Rect{100.0f, 100.0f, 200.0f, 200.0f}, result);
  EXPECT_TRUE(result.empty());
}

TEST_F(QuadtreeTest, InsertedNodeIsFoundByOverlappingQuery) {
  ConcreteNode node("A");
  node.setPosition({500.0f, 0.0f, 500.0f});
  node.setLocalBounds(makeBox(0.0f, 0.0f, 0.0f, 5.0f));

  tree_.insert(&node);

  std::vector<SceneNode *> result;
  tree_.query(Quadtree::Rect{490.0f, 490.0f, 510.0f, 510.0f}, result);
  ASSERT_EQ(result.size(), 1u);
  EXPECT_EQ(result[0], &node);
}

TEST_F(QuadtreeTest, InsertedNodeIsNotFoundByNonOverlappingQuery) {
  ConcreteNode node("A");
  node.setPosition({500.0f, 0.0f, 500.0f});
  node.setLocalBounds(makeBox(0.0f, 0.0f, 0.0f, 5.0f));

  tree_.insert(&node);

  std::vector<SceneNode *> result;
  tree_.query(Quadtree::Rect{0.0f, 0.0f, 100.0f, 100.0f}, result);
  EXPECT_TRUE(result.empty());
}

TEST_F(QuadtreeTest, MultipleNodesCanBeInserted) {
  ConcreteNode a("A"), b("B"), c("C");
  a.setPosition({100.0f, 0.0f, 100.0f});
  a.setLocalBounds(makeBox(0.0f, 0.0f, 0.0f, 5.0f));
  b.setPosition({500.0f, 0.0f, 500.0f});
  b.setLocalBounds(makeBox(0.0f, 0.0f, 0.0f, 5.0f));
  c.setPosition({900.0f, 0.0f, 900.0f});
  c.setLocalBounds(makeBox(0.0f, 0.0f, 0.0f, 5.0f));

  tree_.insert(&a);
  tree_.insert(&b);
  tree_.insert(&c);

  std::vector<SceneNode *> all;
  tree_.query(Quadtree::Rect{0.0f, 0.0f, 1000.0f, 1000.0f}, all);
  EXPECT_EQ(all.size(), 3u);
}

TEST_F(QuadtreeTest, QueryReturnsOnlyOverlappingNodes) {
  ConcreteNode a("A"), b("B");
  a.setPosition({100.0f, 0.0f, 100.0f});
  a.setLocalBounds(makeBox(0.0f, 0.0f, 0.0f, 5.0f));
  b.setPosition({900.0f, 0.0f, 900.0f});
  b.setLocalBounds(makeBox(0.0f, 0.0f, 0.0f, 5.0f));

  tree_.insert(&a);
  tree_.insert(&b);

  std::vector<SceneNode *> result;
  tree_.query(Quadtree::Rect{50.0f, 50.0f, 200.0f, 200.0f}, result);
  ASSERT_EQ(result.size(), 1u);
  EXPECT_EQ(result[0], &a);
}

TEST_F(QuadtreeTest, ClearRemovesAllNodes) {
  ConcreteNode node("A");
  node.setPosition({500.0f, 0.0f, 500.0f});
  node.setLocalBounds(makeBox(0.0f, 0.0f, 0.0f, 5.0f));
  tree_.insert(&node);

  tree_.clear();

  std::vector<SceneNode *> result;
  tree_.query(Quadtree::Rect{0.0f, 0.0f, 1000.0f, 1000.0f}, result);
  EXPECT_TRUE(result.empty());
}

TEST_F(QuadtreeTest, NodesWithoutLocalBoundsUseFallbackBounds) {
  ConcreteNode node("A");
  node.setPosition({500.0f, 0.0f, 500.0f});

  tree_.insert(&node);

  std::vector<SceneNode *> result;
  tree_.query(Quadtree::Rect{490.0f, 490.0f, 510.0f, 510.0f}, result);
  EXPECT_EQ(result.size(), 1u);
}

TEST_F(QuadtreeTest, ManyNodesCanBeInserted) {
  std::vector<std::unique_ptr<ConcreteNode>> nodes;
  for (int i = 0; i < 100; ++i) {
    auto node = std::make_unique<ConcreteNode>("N");
    float x = static_cast<float>(i % 10) * 100.0f + 50.0f;
    float z = static_cast<float>(i / 10) * 100.0f + 50.0f;
    node->setPosition({x, 0.0f, z});
    node->setLocalBounds(makeBox(0.0f, 0.0f, 0.0f, 10.0f));
    tree_.insert(node.get());
    nodes.push_back(std::move(node));
  }

  std::vector<SceneNode *> result;
  tree_.query(Quadtree::Rect{0.0f, 0.0f, 1000.0f, 1000.0f}, result);
  EXPECT_EQ(result.size(), 100u);
}

TEST_F(QuadtreeTest, RectIntersectsTest) {
  Quadtree::Rect a{0.0f, 0.0f, 100.0f, 100.0f};
  Quadtree::Rect b{50.0f, 50.0f, 150.0f, 150.0f};
  Quadtree::Rect c{200.0f, 200.0f, 300.0f, 300.0f};

  EXPECT_TRUE(a.intersects(b));
  EXPECT_FALSE(a.intersects(c));
}
