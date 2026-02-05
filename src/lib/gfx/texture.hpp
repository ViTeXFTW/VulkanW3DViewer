#pragma once

#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace w3d::gfx {

class VulkanContext;

struct GPUTexture {
  vk::Image image;
  vk::DeviceMemory memory;
  vk::ImageView view;
  vk::Sampler sampler;
  uint32_t width = 0;
  uint32_t height = 0;
  std::string name;

  bool valid() const { return image && view && sampler; }
};

class TextureManager {
public:
  TextureManager() = default;
  ~TextureManager();

  TextureManager(const TextureManager &) = delete;
  TextureManager &operator=(const TextureManager &) = delete;

  void init(VulkanContext &context);

  void setTexturePath(const std::filesystem::path &path) { texturePath_ = path; }

  const std::filesystem::path &texturePath() const { return texturePath_; }

  void destroy();

  void createDefaultTexture();

  uint32_t loadTexture(const std::string &w3dName);

  uint32_t createTexture(const std::string &name, uint32_t width, uint32_t height,
                         const uint8_t *data);

  uint32_t createTextureWithFormat(const std::string &name, uint32_t width, uint32_t height,
                                   const uint8_t *data, size_t dataSize, vk::Format format);

  const GPUTexture &texture(uint32_t index) const;

  size_t textureCount() const { return textures_.size(); }

  uint32_t findTexture(const std::string &name) const;

  vk::DescriptorImageInfo descriptorInfo(uint32_t index) const;

private:
  std::filesystem::path resolveTexturePath(const std::string &w3dName);

  bool loadTGA(const std::filesystem::path &path, std::vector<uint8_t> &data, uint32_t &width,
               uint32_t &height);

  bool loadDDS(const std::filesystem::path &path, std::vector<uint8_t> &data, uint32_t &width,
               uint32_t &height, vk::Format &format);

  void createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling,
                   vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image &image,
                   vk::DeviceMemory &memory);

  vk::ImageView createImageView(vk::Image image, vk::Format format);
  vk::Sampler createSampler();

  void transitionImageLayout(vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);

  void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);

  uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

  VulkanContext *context_ = nullptr;
  std::filesystem::path texturePath_;
  std::vector<GPUTexture> textures_;
  std::unordered_map<std::string, uint32_t> textureNameMap_;
};

} // namespace w3d::gfx
