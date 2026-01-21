#pragma once

#include <vulkan/vulkan.hpp>

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace w3d {

class VulkanContext;

// A single GPU texture
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

// Manages texture loading and GPU resources
class TextureManager {
public:
  TextureManager() = default;
  ~TextureManager();

  TextureManager(const TextureManager &) = delete;
  TextureManager &operator=(const TextureManager &) = delete;

  // Initialize the texture manager
  void init(VulkanContext &context);

  // Set the base path for texture files
  void setTexturePath(const std::filesystem::path &path) { texturePath_ = path; }

  // Get the base path for texture files
  const std::filesystem::path &texturePath() const { return texturePath_; }

  // Destroy all textures and resources
  void destroy();

  // Create a default white texture (1x1 pixel)
  void createDefaultTexture();

  // Load texture from file (supports TGA and DDS)
  // w3dName: texture name from W3D file (e.g., "ATMetal03.tga")
  // Returns texture index, or 0 (default texture) if load fails
  uint32_t loadTexture(const std::string &w3dName);

  // Load texture from raw RGBA data
  uint32_t createTexture(const std::string &name, uint32_t width, uint32_t height,
                         const uint8_t *data);

  // Load texture from raw data with specific format (for compressed textures)
  uint32_t createTextureWithFormat(const std::string &name, uint32_t width, uint32_t height,
                                   const uint8_t *data, size_t dataSize, vk::Format format);

  // Get texture by index
  const GPUTexture &texture(uint32_t index) const;

  // Get texture count
  size_t textureCount() const { return textures_.size(); }

  // Find texture by name (returns 0 if not found)
  uint32_t findTexture(const std::string &name) const;

  // Get the descriptor image info for a texture
  vk::DescriptorImageInfo descriptorInfo(uint32_t index) const;

private:
  // Try to find a texture file on disk given a W3D texture name
  std::filesystem::path resolveTexturePath(const std::string &w3dName);

  // Load TGA file (uncompressed RGBA)
  bool loadTGA(const std::filesystem::path &path, std::vector<uint8_t> &data, uint32_t &width,
               uint32_t &height);

  // Load DDS file (uncompressed or DXT compressed)
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

} // namespace w3d
