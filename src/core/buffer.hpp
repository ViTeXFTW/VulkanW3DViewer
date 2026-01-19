#pragma once

#include <cstring>

#include <vulkan/vulkan.hpp>

namespace w3d {

class VulkanContext;

class Buffer {
public:
  Buffer() = default;
  ~Buffer();

  Buffer(const Buffer &) = delete;
  Buffer &operator=(const Buffer &) = delete;

  Buffer(Buffer &&other) noexcept;
  Buffer &operator=(Buffer &&other) noexcept;

  void create(VulkanContext &context, vk::DeviceSize size, vk::BufferUsageFlags usage,
              vk::MemoryPropertyFlags properties);

  void destroy();

  void *map();
  void unmap();
  void upload(const void *data, vk::DeviceSize size);

  vk::Buffer buffer() const { return buffer_; }
  vk::DeviceMemory memory() const { return memory_; }
  vk::DeviceSize size() const { return size_; }

private:
  vk::Device device_;
  vk::Buffer buffer_;
  vk::DeviceMemory memory_;
  vk::DeviceSize size_ = 0;
  void *mappedData_ = nullptr;
};

// Helper to create a device-local buffer with staging
class StagedBuffer {
public:
  StagedBuffer() = default;

  void create(VulkanContext &context, const void *data, vk::DeviceSize size,
              vk::BufferUsageFlags usage);

  void destroy();

  vk::Buffer buffer() const { return buffer_.buffer(); }
  vk::DeviceSize size() const { return buffer_.size(); }

private:
  Buffer buffer_;
};

// Vertex buffer helper
template <typename Vertex>
class VertexBuffer {
public:
  void create(VulkanContext &context, const std::vector<Vertex> &vertices) {
    stagedBuffer_.create(context, vertices.data(), sizeof(Vertex) * vertices.size(),
                         vk::BufferUsageFlagBits::eVertexBuffer);
    vertexCount_ = static_cast<uint32_t>(vertices.size());
  }

  void destroy() { stagedBuffer_.destroy(); }

  vk::Buffer buffer() const { return stagedBuffer_.buffer(); }
  uint32_t vertexCount() const { return vertexCount_; }

private:
  StagedBuffer stagedBuffer_;
  uint32_t vertexCount_ = 0;
};

// Index buffer helper
class IndexBuffer {
public:
  void create(VulkanContext &context, const std::vector<uint32_t> &indices);
  void destroy() { stagedBuffer_.destroy(); }

  vk::Buffer buffer() const { return stagedBuffer_.buffer(); }
  uint32_t indexCount() const { return indexCount_; }

private:
  StagedBuffer stagedBuffer_;
  uint32_t indexCount_ = 0;
};

// Uniform buffer with per-frame copies
template <typename T>
class UniformBuffer {
public:
  void create(VulkanContext &context, uint32_t frameCount) {
    buffers_.resize(frameCount);
    for (auto &buffer : buffers_) {
      buffer.create(context, sizeof(T), vk::BufferUsageFlagBits::eUniformBuffer,
                    vk::MemoryPropertyFlagBits::eHostVisible |
                        vk::MemoryPropertyFlagBits::eHostCoherent);
    }
  }

  void destroy() {
    for (auto &buffer : buffers_) {
      buffer.destroy();
    }
    buffers_.clear();
  }

  void update(uint32_t frameIndex, const T &data) { buffers_[frameIndex].upload(&data, sizeof(T)); }

  vk::Buffer buffer(uint32_t frameIndex) const { return buffers_[frameIndex].buffer(); }

  size_t frameCount() const { return buffers_.size(); }

private:
  std::vector<Buffer> buffers_;
};

} // namespace w3d
