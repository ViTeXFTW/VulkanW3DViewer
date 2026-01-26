#include "bone_buffer.hpp"

#include "core/vulkan_context.hpp"

#include <algorithm>
#include <cstring>

namespace w3d {

BoneMatrixBuffer::~BoneMatrixBuffer() {
  destroy();
}

void BoneMatrixBuffer::create(VulkanContext &context, size_t maxBones) {
  destroy();

  maxBones_ = maxBones;

  // Create storage buffer for bone matrices
  // Use host-visible memory for easy updates (could optimize with staging later)
  vk::DeviceSize bufferSize = sizeof(glm::mat4) * maxBones;

  buffer_.create(context, bufferSize, vk::BufferUsageFlagBits::eStorageBuffer,
                 vk::MemoryPropertyFlagBits::eHostVisible |
                     vk::MemoryPropertyFlagBits::eHostCoherent);

  // Initialize with identity matrices
  std::vector<glm::mat4> identities(maxBones, glm::mat4(1.0f));
  buffer_.upload(identities.data(), bufferSize);
}

void BoneMatrixBuffer::update(const std::vector<glm::mat4> &skinningMatrices) {
  if (!buffer_.buffer() || skinningMatrices.empty()) {
    return;
  }

  boneCount_ = std::min(skinningMatrices.size(), maxBones_);
  vk::DeviceSize dataSize = sizeof(glm::mat4) * boneCount_;

  buffer_.upload(skinningMatrices.data(), dataSize);
}

void BoneMatrixBuffer::destroy() {
  buffer_.destroy();
  maxBones_ = 0;
  boneCount_ = 0;
}

vk::DescriptorBufferInfo BoneMatrixBuffer::descriptorInfo() const {
  return vk::DescriptorBufferInfo{buffer_.buffer(), 0, sizeof(glm::mat4) * maxBones_};
}

} // namespace w3d
