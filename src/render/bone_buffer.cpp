#include "bone_buffer.hpp"

#include "lib/gfx/vulkan_context.hpp"

#include <algorithm>
#include <cstring>

namespace w3d {

BoneMatrixBuffer::~BoneMatrixBuffer() {
  destroy();
}

void BoneMatrixBuffer::create(gfx::VulkanContext &context, size_t maxBones) {
  destroy();

  maxBones_ = maxBones;

  // Create storage buffers for bone matrices (one per frame in flight)
  // Use host-visible memory for easy updates
  vk::DeviceSize bufferSize = sizeof(glm::mat4) * maxBones;

  // Initialize identity matrices for all buffers
  std::vector<glm::mat4> identities(maxBones, glm::mat4(1.0f));

  for (uint32_t i = 0; i < FRAME_COUNT; ++i) {
    buffers_[i].create(context, bufferSize, vk::BufferUsageFlagBits::eStorageBuffer,
                       vk::MemoryPropertyFlagBits::eHostVisible |
                           vk::MemoryPropertyFlagBits::eHostCoherent);
    buffers_[i].upload(identities.data(), bufferSize);
  }
}

void BoneMatrixBuffer::update(uint32_t frameIndex, const std::vector<glm::mat4> &skinningMatrices) {
  if (frameIndex >= FRAME_COUNT || !buffers_[frameIndex].buffer() || skinningMatrices.empty()) {
    return;
  }

  boneCount_ = std::min(skinningMatrices.size(), maxBones_);
  vk::DeviceSize dataSize = sizeof(glm::mat4) * boneCount_;

  buffers_[frameIndex].upload(skinningMatrices.data(), dataSize);
}

void BoneMatrixBuffer::destroy() {
  for (auto &buffer : buffers_) {
    buffer.destroy();
  }
  maxBones_ = 0;
  boneCount_ = 0;
}

vk::DescriptorBufferInfo BoneMatrixBuffer::descriptorInfo(uint32_t frameIndex) const {
  return vk::DescriptorBufferInfo{buffers_[frameIndex].buffer(), 0, sizeof(glm::mat4) * maxBones_};
}

} // namespace w3d
