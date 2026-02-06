#pragma once

// Stub buffer.hpp for tests - provides buffer types without Vulkan dependencies

#include <vector>

namespace w3d {

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

} // namespace w3d
