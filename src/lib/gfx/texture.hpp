#pragma once

#define VMA_STATIC_VULKAN_FUNCTIONS  0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include <vk_mem_alloc.h>

namespace w3d::gfx {

class VulkanContext;

} // namespace w3d::gfx

namespace w3d::big {
class AssetRegistry;
class BigArchiveManager;
} // namespace w3d::big

namespace w3d::gfx {

struct GPUTexture {
  vk::Image image;
  VmaAllocation allocation = nullptr;
  vk::ImageView view;
  vk::Sampler sampler;
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t mipLevels = 1;
  std::string name;
  vk::DeviceMemory memory;

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

  /// Set asset registry for path resolution
  void setAssetRegistry(big::AssetRegistry *registry) { assetRegistry_ = registry; }

  /// Set BIG archive manager for texture extraction
  void setBigArchiveManager(big::BigArchiveManager *manager) { bigArchiveManager_ = manager; }

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
                   VmaAllocation &allocation, uint32_t mipLevels = 1);

  vk::ImageView createImageView(vk::Image image, vk::Format format, uint32_t mipLevels = 1);
  vk::Sampler createSampler(uint32_t mipLevels = 1);

  void transitionImageLayout(vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                             uint32_t mipLevels = 1);

  void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);

  void generateMipmaps(vk::Image image, vk::Format format, uint32_t width, uint32_t height,
                       uint32_t mipLevels);

  uint32_t calculateMipLevels(uint32_t width, uint32_t height) const;

  uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

  VulkanContext *context_ = nullptr;
  std::filesystem::path texturePath_;
  std::vector<GPUTexture> textures_;
  std::unordered_map<std::string, uint32_t> textureNameMap_;
  big::AssetRegistry *assetRegistry_ = nullptr;
  big::BigArchiveManager *bigArchiveManager_ = nullptr;
};

} // namespace w3d::gfx
