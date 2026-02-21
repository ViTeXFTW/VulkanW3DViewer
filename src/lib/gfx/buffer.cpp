#include "lib/gfx/buffer.hpp"

#include "lib/gfx/vulkan_context.hpp"

namespace w3d::gfx {

Buffer::~Buffer() {
  destroy();
}

Buffer::Buffer(Buffer &&other) noexcept
    : allocator_(other.allocator_), buffer_(other.buffer_), allocation_(other.allocation_),
      size_(other.size_), mappedData_(other.mappedData_), memory_(other.memory_) {
  other.allocator_ = nullptr;
  other.buffer_ = nullptr;
  other.allocation_ = nullptr;
  other.size_ = 0;
  other.mappedData_ = nullptr;
  other.memory_ = nullptr;
}

Buffer &Buffer::operator=(Buffer &&other) noexcept {
  if (this != &other) {
    destroy();
    allocator_ = other.allocator_;
    buffer_ = other.buffer_;
    allocation_ = other.allocation_;
    size_ = other.size_;
    mappedData_ = other.mappedData_;
    memory_ = other.memory_;
    other.allocator_ = nullptr;
    other.buffer_ = nullptr;
    other.allocation_ = nullptr;
    other.size_ = 0;
    other.mappedData_ = nullptr;
    other.memory_ = nullptr;
  }
  return *this;
}

void Buffer::create(VulkanContext &context, vk::DeviceSize size, vk::BufferUsageFlags usage,
                    vk::MemoryPropertyFlags properties) {
  allocator_ = context.allocator();
  size_ = size;

  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = static_cast<VkBufferUsageFlags>(usage);
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocInfo{};
  if (properties & vk::MemoryPropertyFlagBits::eHostVisible) {
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags =
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
  } else {
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
  }

  VkBuffer vkBuffer;
  if (vmaCreateBuffer(allocator_, &bufferInfo, &allocInfo, &vkBuffer, &allocation_, nullptr) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create buffer with VMA");
  }
  buffer_ = vkBuffer;
}

void Buffer::destroy() {
  if (allocator_ && buffer_) {
    if (mappedData_) {
      mappedData_ = nullptr;
    }
    vmaDestroyBuffer(allocator_, buffer_, allocation_);
    buffer_ = nullptr;
    allocation_ = nullptr;
    allocator_ = nullptr;
  }
}

void *Buffer::map() {
  if (!mappedData_ && allocator_ && allocation_) {
    vmaMapMemory(allocator_, allocation_, &mappedData_);
  }
  return mappedData_;
}

void Buffer::unmap() {
  if (mappedData_ && allocator_ && allocation_) {
    vmaUnmapMemory(allocator_, allocation_);
    mappedData_ = nullptr;
  }
}

void Buffer::upload(const void *data, vk::DeviceSize size) {
  void *mapped = map();
  std::memcpy(mapped, data, static_cast<size_t>(size));
  unmap();
}

void StagedBuffer::create(VulkanContext &context, const void *data, vk::DeviceSize size,
                          vk::BufferUsageFlags usage) {
  Buffer stagingBuffer;
  stagingBuffer.create(context, size, vk::BufferUsageFlagBits::eTransferSrc,
                       vk::MemoryPropertyFlagBits::eHostVisible |
                           vk::MemoryPropertyFlagBits::eHostCoherent);

  stagingBuffer.upload(data, size);

  buffer_.create(context, size, usage | vk::BufferUsageFlagBits::eTransferDst,
                 vk::MemoryPropertyFlagBits::eDeviceLocal);

  auto cmd = context.beginSingleTimeCommands();

  vk::BufferCopy copyRegion{0, 0, size};
  cmd.copyBuffer(stagingBuffer.buffer(), buffer_.buffer(), copyRegion);

  context.endSingleTimeCommands(cmd);
}

void StagedBuffer::destroy() {
  buffer_.destroy();
}

void IndexBuffer::create(VulkanContext &context, const std::vector<uint32_t> &indices) {
  stagedBuffer_.create(context, indices.data(), sizeof(uint32_t) * indices.size(),
                       vk::BufferUsageFlagBits::eIndexBuffer);
  indexCount_ = static_cast<uint32_t>(indices.size());
}

} // namespace w3d::gfx
