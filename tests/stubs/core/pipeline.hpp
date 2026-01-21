#pragma once

// Stub pipeline.hpp for tests - provides Vertex struct without Vulkan dependencies

#include <glm/glm.hpp>

namespace w3d {

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 texCoord;
  glm::vec3 color;
};

} // namespace w3d
