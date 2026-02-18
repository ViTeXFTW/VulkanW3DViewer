#include "lib/gfx/buffer.hpp"
#include "lib/gfx/vulkan_context.hpp"


namespace w3d::gfx {

Buffer::~Buffer() {
  destroy();
}

Buffer::Buffer(Buffer &&other) noexcept
    : device_(other.device_), buffer_(other.buffer_), memory_(other.memory_), size_(other.size_),
      mappedData_(other.mappedData_) {
  other.device_ = nullptr;
  other.buffer_ = nullptr;
  other.memory_ = nullptr;
  other.size_ = 0;
  other.mappedData_ = nullptr;
}

Buffer &Buffer::operator=(Buffer &&other) noexcept {
  if (this != &other) {
    destroy();
    device_ = other.device_;
    buffer_ = other.buffer_;
    memory_ = other.memory_;
    size_ = other.size_;
    mappedData_ = other.mappedData_;
    other.device_ = nullptr;
    other.buffer_ = nullptr;
    other.memory_ = nullptr;
    other.size_ = 0;
    other.mappedData_ = nullptr;
  }
  return *this;
}

void Buffer::create(VulkanContext &context, vk::DeviceSize size, vk::BufferUsageFlags usage,
                    vk::MemoryPropertyFlags properties) {
  device_ = context.device();
  size_ = size;

  vk::BufferCreateInfo bufferInfo{{}, size, usage, vk::SharingMode::eExclusive};

  buffer_ = device_.createBuffer(bufferInfo);

  auto memRequirements = device_.getBufferMemoryRequirements(buffer_);

  vk::MemoryAllocateInfo allocInfo{
      memRequirements.size, context.findMemoryType(memRequirements.memoryTypeBits, properties)};

  memory_ = device_.allocateMemory(allocInfo);
  device_.bindBufferMemory(buffer_, memory_, 0);
}

void Buffer::destroy() {
  if (device_) {
    if (mappedData_) {
      unmap();
    }
    if (buffer_) {
      device_.destroyBuffer(buffer_);
      buffer_ = nullptr;
    }
    if (memory_) {
      device_.freeMemory(memory_);
      memory_ = nullptr;
    }
    device_ = nullptr;
  }
}

void *Buffer::map() {
  if (!mappedData_) {
    mappedData_ = device_.mapMemory(memory_, 0, size_);
  }
  return mappedData_;
}

void Buffer::unmap() {
  if (mappedData_) {
    device_.unmapMemory(memory_);
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
