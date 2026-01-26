#pragma once

#include "core/buffer.hpp"

#include <glm/glm.hpp>

#include <vector>

namespace w3d {

class VulkanContext;

// Storage buffer for bone matrices (SSBO)
// Used for GPU skinning - equivalent to legacy HTreeClass::Get_Transform()
class BoneMatrixBuffer {
public:
  static constexpr size_t MAX_BONES = 256;

  BoneMatrixBuffer() = default;
  ~BoneMatrixBuffer();

  BoneMatrixBuffer(const BoneMatrixBuffer &) = delete;
  BoneMatrixBuffer &operator=(const BoneMatrixBuffer &) = delete;

  // Create the buffer with space for maxBones matrices
  void create(VulkanContext &context, size_t maxBones = MAX_BONES);

  // Update bone matrices from skinning matrices
  void update(const std::vector<glm::mat4> &skinningMatrices);

  // Free GPU resources
  void destroy();

  // Check if buffer is created
  bool isCreated() const { return buffer_.buffer(); }

  // Get Vulkan buffer handle
  vk::Buffer buffer() const { return buffer_.buffer(); }

  // Get descriptor info for binding
  vk::DescriptorBufferInfo descriptorInfo() const;

  // Get current bone count
  size_t boneCount() const { return boneCount_; }

  // Get maximum bone count
  size_t maxBones() const { return maxBones_; }

private:
  Buffer buffer_;
  size_t maxBones_ = 0;
  size_t boneCount_ = 0;
};

} // namespace w3d
