#pragma once

#include "core/buffer.hpp"

#include <glm/glm.hpp>

#include <array>
#include <vector>

namespace w3d {

class VulkanContext;

// Storage buffer for bone matrices (SSBO)
// Used for GPU skinning - equivalent to legacy HTreeClass::Get_Transform()
// Double-buffered to allow CPU updates while GPU is reading previous frame.
class BoneMatrixBuffer {
public:
  static constexpr size_t MAX_BONES = 256;
  static constexpr uint32_t FRAME_COUNT = 2; // Double-buffering

  BoneMatrixBuffer() = default;
  ~BoneMatrixBuffer();

  BoneMatrixBuffer(const BoneMatrixBuffer &) = delete;
  BoneMatrixBuffer &operator=(const BoneMatrixBuffer &) = delete;

  // Create the buffers with space for maxBones matrices
  void create(VulkanContext &context, size_t maxBones = MAX_BONES);

  // Update bone matrices for a specific frame
  void update(uint32_t frameIndex, const std::vector<glm::mat4> &skinningMatrices);

  // Free GPU resources
  void destroy();

  // Check if buffers are created
  bool isCreated() const { return buffers_[0].buffer(); }

  // Get Vulkan buffer handle for a specific frame
  vk::Buffer buffer(uint32_t frameIndex) const { return buffers_[frameIndex].buffer(); }

  // Get descriptor info for binding a specific frame
  vk::DescriptorBufferInfo descriptorInfo(uint32_t frameIndex) const;

  // Get current bone count
  size_t boneCount() const { return boneCount_; }

  // Get maximum bone count
  size_t maxBones() const { return maxBones_; }

private:
  std::array<Buffer, FRAME_COUNT> buffers_;
  size_t maxBones_ = 0;
  size_t boneCount_ = 0;
};

} // namespace w3d
