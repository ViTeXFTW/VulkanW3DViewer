#include "quadtree.hpp"

#include <algorithm>

namespace w3d::scene {

Quadtree::Quadtree(float minX, float minZ, float maxX, float maxZ, int maxDepth, int maxPerNode)
    : maxDepth_(maxDepth), maxPerNode_(maxPerNode) {
  Node root;
  root.bounds = {minX, minZ, maxX, maxZ};
  nodes_.push_back(root);
}

Quadtree::Rect Quadtree::nodeWorldRect(const SceneNode *node) const {
  gfx::BoundingBox wb = node->worldBounds();
  if (wb.valid()) {
    return {wb.min.x, wb.min.z, wb.max.x, wb.max.z};
  }
  const glm::vec3 &pos = node->position();
  constexpr float kFallbackHalf = 1.0f;
  return {pos.x - kFallbackHalf, pos.z - kFallbackHalf, pos.x + kFallbackHalf,
          pos.z + kFallbackHalf};
}

void Quadtree::insert(SceneNode *node) {
  Entry entry;
  entry.node = node;
  entry.bounds = nodeWorldRect(node);
  insertInto(0, entry, 0);
}

void Quadtree::insertInto(int nodeIndex, const Entry &entry, int depth) {
  Node &n = nodes_[nodeIndex];

  if (n.isLeaf) {
    n.entries.push_back(entry);
    if (static_cast<int>(n.entries.size()) > maxPerNode_ && depth < maxDepth_) {
      subdivide(nodeIndex);
    }
    return;
  }

  float cx = (entry.bounds.minX + entry.bounds.maxX) * 0.5f;
  float cz = (entry.bounds.minZ + entry.bounds.maxZ) * 0.5f;

  for (int ci : n.children) {
    if (ci < 0)
      continue;
    if (nodes_[ci].bounds.contains(cx, cz)) {
      insertInto(ci, entry, depth + 1);
      return;
    }
  }

  for (int ci : n.children) {
    if (ci < 0)
      continue;
    if (nodes_[ci].bounds.intersects(entry.bounds)) {
      insertInto(ci, entry, depth + 1);
      return;
    }
  }
}

void Quadtree::subdivide(int nodeIndex) {
  float midX = (nodes_[nodeIndex].bounds.minX + nodes_[nodeIndex].bounds.maxX) * 0.5f;
  float midZ = (nodes_[nodeIndex].bounds.minZ + nodes_[nodeIndex].bounds.maxZ) * 0.5f;

  Rect quads[4] = {
      {nodes_[nodeIndex].bounds.minX, nodes_[nodeIndex].bounds.minZ, midX,                          midZ                         },
      {midX,                          nodes_[nodeIndex].bounds.minZ, nodes_[nodeIndex].bounds.maxX, midZ                         },
      {nodes_[nodeIndex].bounds.minX, midZ,                          midX,                          nodes_[nodeIndex].bounds.maxZ},
      {midX,                          midZ,                          nodes_[nodeIndex].bounds.maxX, nodes_[nodeIndex].bounds.maxZ},
  };

  int baseIndex = static_cast<int>(nodes_.size());
  nodes_[nodeIndex].children[0] = baseIndex;
  nodes_[nodeIndex].children[1] = baseIndex + 1;
  nodes_[nodeIndex].children[2] = baseIndex + 2;
  nodes_[nodeIndex].children[3] = baseIndex + 3;
  nodes_[nodeIndex].isLeaf = false;

  nodes_.reserve(nodes_.size() + 4);
  for (int i = 0; i < 4; ++i) {
    Node child;
    child.bounds = quads[i];
    nodes_.push_back(child);
  }

  std::vector<Entry> entries = std::move(nodes_[nodeIndex].entries);
  nodes_[nodeIndex].entries.clear();

  for (const auto &entry : entries) {
    insertInto(nodeIndex, entry, 1);
  }
}

void Quadtree::clear() {
  float minX = nodes_[0].bounds.minX;
  float minZ = nodes_[0].bounds.minZ;
  float maxX = nodes_[0].bounds.maxX;
  float maxZ = nodes_[0].bounds.maxZ;
  nodes_.clear();
  Node root;
  root.bounds = {minX, minZ, maxX, maxZ};
  nodes_.push_back(root);
}

void Quadtree::query(const Rect &rect, std::vector<SceneNode *> &result) const {
  queryNode(0, rect, result);
}

void Quadtree::query(const gfx::Frustum &frustum, std::vector<SceneNode *> &result) const {
  queryNodeFrustum(0, frustum, result);
}

void Quadtree::queryNode(int nodeIndex, const Rect &rect, std::vector<SceneNode *> &result) const {
  const Node &n = nodes_[nodeIndex];

  if (!n.bounds.intersects(rect))
    return;

  for (const auto &entry : n.entries) {
    if (entry.bounds.intersects(rect)) {
      result.push_back(entry.node);
    }
  }

  if (!n.isLeaf) {
    for (int ci : n.children) {
      if (ci >= 0) {
        queryNode(ci, rect, result);
      }
    }
  }
}

bool Quadtree::rectIntersectsFrustum(const Rect &rect, const gfx::Frustum &frustum) {
  gfx::BoundingBox box;
  box.expand(glm::vec3(rect.minX, -1e6f, rect.minZ));
  box.expand(glm::vec3(rect.maxX, 1e6f, rect.maxZ));
  return frustum.isBoxVisible(box);
}

void Quadtree::queryNodeFrustum(int nodeIndex, const gfx::Frustum &frustum,
                                std::vector<SceneNode *> &result) const {
  const Node &n = nodes_[nodeIndex];

  if (!rectIntersectsFrustum(n.bounds, frustum))
    return;

  for (const auto &entry : n.entries) {
    gfx::BoundingBox entryBox;
    entryBox.expand(glm::vec3(entry.bounds.minX, -1e6f, entry.bounds.minZ));
    entryBox.expand(glm::vec3(entry.bounds.maxX, 1e6f, entry.bounds.maxZ));
    if (frustum.isBoxVisible(entry.node->worldBounds().valid() ? entry.node->worldBounds()
                                                               : entryBox)) {
      result.push_back(entry.node);
    }
  }

  if (!n.isLeaf) {
    for (int ci : n.children) {
      if (ci >= 0) {
        queryNodeFrustum(ci, frustum, result);
      }
    }
  }
}

} // namespace w3d::scene
