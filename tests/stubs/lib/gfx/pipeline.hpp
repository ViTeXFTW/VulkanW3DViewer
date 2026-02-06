#pragma once

// Stub pipeline.hpp for tests - provides Vertex struct without Vulkan dependencies

#include <glm/glm.hpp>

#include <cstdint>

namespace w3d::gfx {

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 texCoord;
  glm::vec3 color;
};

// Skinned vertex with bone index for GPU skinning
// W3D uses rigid skinning (one bone per vertex, no blend weights)
struct SkinnedVertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 texCoord;
  glm::vec3 color;
  uint32_t boneIndex; // Single bone per vertex (W3D rigid skinning)
};

} // namespace w3d::gfx
