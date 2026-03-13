#include "scene_graph.hpp"

namespace w3d::scene {

SceneGraph::SceneGraph(float worldMinX, float worldMinZ, float worldMaxX, float worldMaxZ)
    : quadtree_(worldMinX, worldMinZ, worldMaxX, worldMaxZ) {}

SceneNode *SceneGraph::addNode(std::unique_ptr<SceneNode> node) {
  SceneNode *ptr = node.get();
  quadtree_.insert(ptr);
  nodes_.push_back(std::move(node));
  return ptr;
}

void SceneGraph::clear() {
  nodes_.clear();
  quadtree_.clear();
}

void SceneGraph::queryVisible(const gfx::Frustum &frustum, std::vector<SceneNode *> &result) const {
  std::vector<SceneNode *> candidates;
  quadtree_.query(frustum, candidates);
  for (SceneNode *n : candidates) {
    if (n->isVisible()) {
      result.push_back(n);
    }
  }
}

void SceneGraph::queryAll(std::vector<SceneNode *> &result) const {
  for (const auto &node : nodes_) {
    if (node->isVisible()) {
      result.push_back(node.get());
    }
  }
}

} // namespace w3d::scene
