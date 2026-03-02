#pragma once

#include <memory>
#include <vector>

#include "lib/gfx/frustum.hpp"
#include "lib/scene/quadtree.hpp"
#include "lib/scene/scene_node.hpp"

namespace w3d::scene {

class SceneGraph {
public:
  SceneGraph(float worldMinX, float worldMinZ, float worldMaxX, float worldMaxZ);
  ~SceneGraph() = default;

  SceneGraph(const SceneGraph &) = delete;
  SceneGraph &operator=(const SceneGraph &) = delete;

  [[nodiscard]] SceneNode *addNode(std::unique_ptr<SceneNode> node);

  void clear();

  [[nodiscard]] size_t nodeCount() const { return nodes_.size(); }

  void queryVisible(const gfx::Frustum &frustum, std::vector<SceneNode *> &result) const;

  void queryAll(std::vector<SceneNode *> &result) const;

private:
  std::vector<std::unique_ptr<SceneNode>> nodes_;
  Quadtree quadtree_;
};

} // namespace w3d::scene
