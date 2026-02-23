#pragma once

#include <vector>

#include "lib/gfx/bounding_box.hpp"
#include "lib/gfx/frustum.hpp"
#include "lib/scene/scene_node.hpp"

namespace w3d::scene {

class Quadtree {
public:
  struct Rect {
    float minX = 0.0f;
    float minZ = 0.0f;
    float maxX = 0.0f;
    float maxZ = 0.0f;

    [[nodiscard]] bool intersects(const Rect &other) const {
      return minX <= other.maxX && maxX >= other.minX && minZ <= other.maxZ && maxZ >= other.minZ;
    }

    [[nodiscard]] bool contains(float x, float z) const {
      return x >= minX && x <= maxX && z >= minZ && z <= maxZ;
    }
  };

  Quadtree(float minX, float minZ, float maxX, float maxZ, int maxDepth = 6,
           int maxPerNode = 8);

  void insert(SceneNode *node);
  void clear();

  void query(const Rect &rect, std::vector<SceneNode *> &result) const;
  void query(const gfx::Frustum &frustum, std::vector<SceneNode *> &result) const;

private:
  struct Entry {
    SceneNode *node = nullptr;
    Rect bounds;
  };

  struct Node {
    Rect bounds;
    std::vector<Entry> entries;
    int children[4] = {-1, -1, -1, -1};
    bool isLeaf = true;
  };

  [[nodiscard]] Rect nodeWorldRect(const SceneNode *node) const;
  void insertInto(int nodeIndex, const Entry &entry, int depth);
  void subdivide(int nodeIndex);
  void queryNode(int nodeIndex, const Rect &rect, std::vector<SceneNode *> &result) const;
  void queryNodeFrustum(int nodeIndex, const gfx::Frustum &frustum,
                        std::vector<SceneNode *> &result) const;
  [[nodiscard]] static bool rectIntersectsFrustum(const Rect &rect,
                                                  const gfx::Frustum &frustum);

  std::vector<Node> nodes_;
  int maxDepth_;
  int maxPerNode_;
};

} // namespace w3d::scene
