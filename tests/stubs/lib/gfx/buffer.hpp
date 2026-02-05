#pragma once

// Stub buffer.hpp for tests - provides buffer types without Vulkan dependencies

#include <glm/glm.hpp>
#include <vector>

namespace w3d::gfx {

class VulkanContext;

// Stub vertex buffer - no actual GPU operations
template <typename T>
class VertexBuffer {
public:
  void create(VulkanContext & /*context*/, const std::vector<T> & /*data*/) {}
  void destroy() {}
};

// Stub index buffer - no actual GPU operations
class IndexBuffer {
public:
  void create(VulkanContext & /*context*/, const std::vector<uint32_t> & /*data*/) {}
  void destroy() {}
  uint32_t indexCount() const { return 0; }
};

// Stub buffer class
class Buffer {
public:
  void destroy() {}
};

// Stub bounding box
struct BoundingBox {
  glm::vec3 min{};
  glm::vec3 max{};
};

} // namespace w3d::gfx
