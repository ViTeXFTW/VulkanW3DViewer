#include "lib/gfx/texture.hpp"

#include "lib/gfx/vulkan_context.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "lib/formats/big/asset_registry.hpp"
#include "lib/formats/big/big_archive_manager.hpp"

namespace w3d::gfx {

namespace {

std::string toLower(const std::string &str) {
  std::string result = str;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return result;
}

std::string removeExtension(const std::string &filename) {
  size_t lastDot = filename.find_last_of('.');
  if (lastDot == std::string::npos) {
    return filename;
  }
  return filename.substr(0, lastDot);
}

} // namespace

TextureManager::~TextureManager() {
  destroy();
}

void TextureManager::init(VulkanContext &context) {
  context_ = &context;
  createDefaultTexture();
}

void TextureManager::destroy() {
  if (!context_) {
    return;
  }

  vk::Device device = context_->device();

  for (auto &tex : textures_) {
    if (tex.sampler) {
      device.destroySampler(tex.sampler);
    }
    if (tex.view) {
      device.destroyImageView(tex.view);
    }
    if (tex.image) {
      device.destroyImage(tex.image);
    }
    if (tex.memory) {
      device.freeMemory(tex.memory);
    }
  }

  textures_.clear();
  textureNameMap_.clear();
  context_ = nullptr;
}

void TextureManager::createDefaultTexture() {
  uint8_t whitePixel[4] = {255, 255, 255, 255};
  createTexture("__default__", 1, 1, whitePixel);
}

std::filesystem::path TextureManager::resolveTexturePath(const std::string &w3dName) {
  if (texturePath_.empty()) {
    return {};
  }

  std::string baseName = toLower(removeExtension(w3dName));

  std::vector<std::string> extensions = {".dds", ".tga", ".DDS", ".TGA"};

  for (const auto &ext : extensions) {
    std::filesystem::path path = texturePath_ / (baseName + ext);
    if (std::filesystem::exists(path)) {
      return path;
    }

    std::string origBase = removeExtension(w3dName);
    path = texturePath_ / (origBase + ext);
    if (std::filesystem::exists(path)) {
      return path;
    }
  }

  return {};
}

uint32_t TextureManager::loadTexture(const std::string &w3dName) {
  if (!context_) {
    return 0;
  }

  std::string normalizedName = toLower(w3dName);
  auto it = textureNameMap_.find(normalizedName);
  if (it != textureNameMap_.end()) {
    return it->second;
  }

  std::filesystem::path path = resolveTexturePath(w3dName);

  // If not found on disk and we have BIG archive support, try extraction
  if (path.empty() && bigArchiveManager_ && bigArchiveManager_->isInitialized()) {
    std::string baseName = toLower(removeExtension(w3dName));

    // Try to get archive path from registry
    std::string archivePath;
    if (assetRegistry_ && assetRegistry_->isScanned()) {
      archivePath = assetRegistry_->getTextureArchivePath(baseName);
    }

    // If not in registry, try standard archive paths
    if (archivePath.empty()) {
      // Try .dds first, then .tga
      archivePath = "Art/Textures/" + baseName + ".dds";
      auto cachedPath = bigArchiveManager_->extractToCache(archivePath, nullptr);
      if (!cachedPath) {
        archivePath = "Art/Textures/" + baseName + ".tga";
        cachedPath = bigArchiveManager_->extractToCache(archivePath, nullptr);
        if (cachedPath) {
          path = *cachedPath;
        }
      } else {
        path = *cachedPath;
      }
    } else {
      // Extract from known archive path
      auto cachedPath = bigArchiveManager_->extractToCache(archivePath, nullptr);
      if (cachedPath) {
        path = *cachedPath;
      }
    }

    if (!path.empty()) {
#ifdef W3D_DEBUG
      std::cerr << "Texture extracted from BIG archive: " << w3dName << " -> " << path.string()
                << "\n";
#endif
    }
  }

  if (path.empty()) {
    std::cerr << "Texture not found: " << w3dName << " (searched in " << texturePath_.string()
              << ")\n";
    return 0;
  }
#ifdef W3D_DEBUG
  std::cerr << "Loading texture: " << w3dName << " -> " << path.string() << "\n";
#endif

  std::vector<uint8_t> data;
  uint32_t width = 0, height = 0;
  vk::Format format = vk::Format::eR8G8B8A8Srgb;

  std::string ext = toLower(path.extension().string());
  bool loaded = false;

  if (ext == ".dds") {
    loaded = loadDDS(path, data, width, height, format);
#ifdef W3D_DEBUG
    if (!loaded) {
      std::cerr << "  DDS load failed for: " << path.string() << "\n";
    }
#endif
  } else if (ext == ".tga") {
    loaded = loadTGA(path, data, width, height);
#ifdef W3D_DEBUG
    if (!loaded) {
      std::cerr << "  TGA load failed for: " << path.string() << "\n";
    }
#endif
  }

  if (!loaded || data.empty()) {
#ifdef W3D_DEBUG
    std::cerr << "  loaded=" << loaded << " data.size=" << data.size() << "\n";
#endif
    return 0;
  }

#ifdef W3D_DEBUG
  std::cerr << "  Creating texture: " << width << "x" << height
            << " format=" << static_cast<int>(format) << " dataSize=" << data.size() << "\n";
#endif

  uint32_t index;
  try {
    if (format == vk::Format::eR8G8B8A8Srgb) {
      index = createTexture(normalizedName, width, height, data.data());
    } else {
      index =
          createTextureWithFormat(normalizedName, width, height, data.data(), data.size(), format);
    }
  } catch (const std::exception &e) {
#ifdef W3D_DEBUG
    std::cerr << "  GPU texture creation failed: " << e.what() << "\n";
#else
    (void)e;
#endif
    return 0;
  }
  return index;
}

bool TextureManager::loadTGA(const std::filesystem::path &path, std::vector<uint8_t> &data,
                             uint32_t &width, uint32_t &height) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return false;
  }

  uint8_t header[18];
  file.read(reinterpret_cast<char *>(header), 18);

  uint8_t idLength = header[0];
  uint8_t colorMapType = header[1];
  uint8_t imageType = header[2];
  width = header[12] | (header[13] << 8);
  height = header[14] | (header[15] << 8);
  uint8_t bpp = header[16];

  if (colorMapType != 0 || (imageType != 2 && imageType != 3)) {
    return false;
  }

  file.seekg(idLength, std::ios::cur);

  size_t pixelCount = width * height;
  size_t bytesPerPixel = bpp / 8;
  std::vector<uint8_t> rawData(pixelCount * bytesPerPixel);
  file.read(reinterpret_cast<char *>(rawData.data()), static_cast<std::streamsize>(rawData.size()));

  data.resize(pixelCount * 4);

  for (size_t i = 0; i < pixelCount; ++i) {
    size_t srcIdx = i * bytesPerPixel;
    size_t dstIdx = i * 4;

    if (bpp == 32) {
      data[dstIdx + 0] = rawData[srcIdx + 2];
      data[dstIdx + 1] = rawData[srcIdx + 1];
      data[dstIdx + 2] = rawData[srcIdx + 0];
      data[dstIdx + 3] = rawData[srcIdx + 3];
    } else if (bpp == 24) {
      data[dstIdx + 0] = rawData[srcIdx + 2];
      data[dstIdx + 1] = rawData[srcIdx + 1];
      data[dstIdx + 2] = rawData[srcIdx + 0];
      data[dstIdx + 3] = 255;
    } else if (bpp == 8) {
      data[dstIdx + 0] = rawData[srcIdx];
      data[dstIdx + 1] = rawData[srcIdx];
      data[dstIdx + 2] = rawData[srcIdx];
      data[dstIdx + 3] = 255;
    }
  }

  bool flipVertical = (header[17] & 0x20) == 0;
  if (flipVertical) {
    std::vector<uint8_t> flipped(data.size());
    size_t rowSize = width * 4;
    for (uint32_t y = 0; y < height; ++y) {
      std::memcpy(&flipped[y * rowSize], &data[(height - 1 - y) * rowSize], rowSize);
    }
    data = std::move(flipped);
  }

  return true;
}

bool TextureManager::loadDDS(const std::filesystem::path &path, std::vector<uint8_t> &data,
                             uint32_t &width, uint32_t &height, vk::Format &format) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return false;
  }

  uint32_t magic;
  file.read(reinterpret_cast<char *>(&magic), 4);
  if (magic != 0x20534444) {
    return false;
  }

  uint32_t headerData[31];
  file.read(reinterpret_cast<char *>(headerData), 124);

  height = headerData[2];
  width = headerData[3];
  uint32_t mipMapCount = headerData[6];
  (void)mipMapCount;

  uint32_t pfFlags = headerData[19];
  uint32_t fourCC = headerData[20];
  uint32_t rgbBitCount = headerData[21];
  uint32_t rMask = headerData[22];
  uint32_t gMask = headerData[23];
  uint32_t bMask = headerData[24];
  uint32_t aMask = headerData[25];

#ifdef W3D_DEBUG
  std::cerr << "    DDS: " << width << "x" << height << " pfFlags=0x" << std::hex << pfFlags
            << " fourCC=0x" << fourCC << std::dec << "\n";
#endif

  bool compressed = (pfFlags & 0x4) != 0;

  if (compressed) {
    size_t blockSize = 0;

    if (fourCC == 0x31545844) {
      format = vk::Format::eBc1RgbaSrgbBlock;
      blockSize = 8;
    } else if (fourCC == 0x33545844) {
      format = vk::Format::eBc2SrgbBlock;
      blockSize = 16;
    } else if (fourCC == 0x35545844) {
      format = vk::Format::eBc3SrgbBlock;
      blockSize = 16;
    } else {
      std::cerr << "    Unsupported DDS fourCC: 0x" << std::hex << fourCC << std::dec << "\n";
      return false;
    }

    uint32_t blocksX = (width + 3) / 4;
    uint32_t blocksY = (height + 3) / 4;
    size_t dataSize = blocksX * blocksY * blockSize;

    data.resize(dataSize);
    file.read(reinterpret_cast<char *>(data.data()), static_cast<std::streamsize>(dataSize));

    return true;
  }

  size_t pixelCount = width * height;
  uint32_t bytesPerPixel = rgbBitCount / 8;

  std::vector<uint8_t> rawData(pixelCount * bytesPerPixel);
  file.read(reinterpret_cast<char *>(rawData.data()), static_cast<std::streamsize>(rawData.size()));

  data.resize(pixelCount * 4);
  format = vk::Format::eR8G8B8A8Srgb;

  for (size_t i = 0; i < pixelCount; ++i) {
    uint32_t pixel = 0;
    for (uint32_t b = 0; b < bytesPerPixel; ++b) {
      pixel |= rawData[i * bytesPerPixel + b] << (b * 8);
    }

    auto extractChannel = [](uint32_t pixel, uint32_t mask) -> uint8_t {
      if (mask == 0)
        return 255;
      uint32_t shift = 0;
      uint32_t m = mask;
      while ((m & 1) == 0) {
        m >>= 1;
        shift++;
      }
      uint32_t value = (pixel & mask) >> shift;
      uint32_t maxVal = mask >> shift;
      if (maxVal > 0 && maxVal != 255) {
        value = (value * 255) / maxVal;
      }
      return static_cast<uint8_t>(value);
    };

    size_t dstIdx = i * 4;
    data[dstIdx + 0] = extractChannel(pixel, rMask);
    data[dstIdx + 1] = extractChannel(pixel, gMask);
    data[dstIdx + 2] = extractChannel(pixel, bMask);
    data[dstIdx + 3] = (aMask != 0) ? extractChannel(pixel, aMask) : 255;
  }

  return true;
}

uint32_t TextureManager::createTexture(const std::string &name, uint32_t width, uint32_t height,
                                       const uint8_t *data) {
  if (!context_) {
    return 0;
  }

  auto it = textureNameMap_.find(name);
  if (it != textureNameMap_.end()) {
    return it->second;
  }

  vk::Device device = context_->device();
  vk::DeviceSize imageSize = width * height * 4;

  vk::BufferCreateInfo bufferInfo{{}, imageSize, vk::BufferUsageFlagBits::eTransferSrc};
  vk::Buffer stagingBuffer = device.createBuffer(bufferInfo);

  vk::MemoryRequirements memRequirements = device.getBufferMemoryRequirements(stagingBuffer);
  vk::MemoryAllocateInfo allocInfo{memRequirements.size,
                                   findMemoryType(memRequirements.memoryTypeBits,
                                                  vk::MemoryPropertyFlagBits::eHostVisible |
                                                      vk::MemoryPropertyFlagBits::eHostCoherent)};

  vk::DeviceMemory stagingMemory = device.allocateMemory(allocInfo);
  device.bindBufferMemory(stagingBuffer, stagingMemory, 0);

  void *mapped = device.mapMemory(stagingMemory, 0, imageSize);
  std::memcpy(mapped, data, static_cast<size_t>(imageSize));
  device.unmapMemory(stagingMemory);

  GPUTexture tex;
  tex.name = name;
  tex.width = width;
  tex.height = height;

  createImage(width, height, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal,
              vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
              vk::MemoryPropertyFlagBits::eDeviceLocal, tex.image, tex.memory);

  transitionImageLayout(tex.image, vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eTransferDstOptimal);
  copyBufferToImage(stagingBuffer, tex.image, width, height);
  transitionImageLayout(tex.image, vk::ImageLayout::eTransferDstOptimal,
                        vk::ImageLayout::eShaderReadOnlyOptimal);

  device.destroyBuffer(stagingBuffer);
  device.freeMemory(stagingMemory);

  tex.view = createImageView(tex.image, vk::Format::eR8G8B8A8Srgb);
  tex.sampler = createSampler();

  uint32_t index = static_cast<uint32_t>(textures_.size());
  textures_.push_back(std::move(tex));
  textureNameMap_[name] = index;

  return index;
}

uint32_t TextureManager::createTextureWithFormat(const std::string &name, uint32_t width,
                                                 uint32_t height, const uint8_t *data,
                                                 size_t dataSize, vk::Format format) {
  if (!context_) {
    return 0;
  }

  auto it = textureNameMap_.find(name);
  if (it != textureNameMap_.end()) {
    return it->second;
  }

  vk::Device device = context_->device();

  vk::BufferCreateInfo bufferInfo{{}, dataSize, vk::BufferUsageFlagBits::eTransferSrc};
  vk::Buffer stagingBuffer = device.createBuffer(bufferInfo);

  vk::MemoryRequirements memRequirements = device.getBufferMemoryRequirements(stagingBuffer);
  vk::MemoryAllocateInfo allocInfo{memRequirements.size,
                                   findMemoryType(memRequirements.memoryTypeBits,
                                                  vk::MemoryPropertyFlagBits::eHostVisible |
                                                      vk::MemoryPropertyFlagBits::eHostCoherent)};

  vk::DeviceMemory stagingMemory = device.allocateMemory(allocInfo);
  device.bindBufferMemory(stagingBuffer, stagingMemory, 0);

  void *mapped = device.mapMemory(stagingMemory, 0, dataSize);
  std::memcpy(mapped, data, dataSize);
  device.unmapMemory(stagingMemory);

  GPUTexture tex;
  tex.name = name;
  tex.width = width;
  tex.height = height;

  createImage(width, height, format, vk::ImageTiling::eOptimal,
              vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
              vk::MemoryPropertyFlagBits::eDeviceLocal, tex.image, tex.memory);

  transitionImageLayout(tex.image, vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eTransferDstOptimal);
  copyBufferToImage(stagingBuffer, tex.image, width, height);
  transitionImageLayout(tex.image, vk::ImageLayout::eTransferDstOptimal,
                        vk::ImageLayout::eShaderReadOnlyOptimal);

  device.destroyBuffer(stagingBuffer);
  device.freeMemory(stagingMemory);

  tex.view = createImageView(tex.image, format);
  tex.sampler = createSampler();

  uint32_t index = static_cast<uint32_t>(textures_.size());
  textures_.push_back(std::move(tex));
  textureNameMap_[name] = index;

  return index;
}

const GPUTexture &TextureManager::texture(uint32_t index) const {
  if (index < textures_.size()) {
    return textures_[index];
  }
  return textures_[0];
}

uint32_t TextureManager::findTexture(const std::string &name) const {
  auto it = textureNameMap_.find(name);
  if (it != textureNameMap_.end()) {
    return it->second;
  }

  std::string normalizedName = toLower(name);
  it = textureNameMap_.find(normalizedName);
  if (it != textureNameMap_.end()) {
    return it->second;
  }

  std::string baseName = toLower(removeExtension(name));
  it = textureNameMap_.find(baseName);
  if (it != textureNameMap_.end()) {
    return it->second;
  }

  return 0;
}

vk::DescriptorImageInfo TextureManager::descriptorInfo(uint32_t index) const {
  const auto &tex = texture(index);
  return vk::DescriptorImageInfo{tex.sampler, tex.view, vk::ImageLayout::eShaderReadOnlyOptimal};
}

void TextureManager::createImage(uint32_t width, uint32_t height, vk::Format format,
                                 vk::ImageTiling tiling, vk::ImageUsageFlags usage,
                                 vk::MemoryPropertyFlags properties, vk::Image &image,
                                 vk::DeviceMemory &memory) {
  vk::Device device = context_->device();

  vk::ImageCreateInfo imageInfo{
      {},
      vk::ImageType::e2D,
      format,
      {width, height, 1},
      1,
      1,
      vk::SampleCountFlagBits::e1,
      tiling,
      usage,
      vk::SharingMode::eExclusive
  };

  image = device.createImage(imageInfo);

  vk::MemoryRequirements memRequirements = device.getImageMemoryRequirements(image);
  vk::MemoryAllocateInfo allocInfo{memRequirements.size,
                                   findMemoryType(memRequirements.memoryTypeBits, properties)};

  memory = device.allocateMemory(allocInfo);
  device.bindImageMemory(image, memory, 0);
}

vk::ImageView TextureManager::createImageView(vk::Image image, vk::Format format) {
  vk::ImageViewCreateInfo viewInfo{
      {},
      image, vk::ImageViewType::e2D, format, {},
      {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
  };

  return context_->device().createImageView(viewInfo);
}

vk::Sampler TextureManager::createSampler() {
  vk::PhysicalDeviceFeatures features = context_->physicalDevice().getFeatures();
  bool anisotropyEnabled = features.samplerAnisotropy == VK_TRUE;

  vk::SamplerCreateInfo samplerInfo{{},
                                    vk::Filter::eLinear,
                                    vk::Filter::eLinear,
                                    vk::SamplerMipmapMode::eLinear,
                                    vk::SamplerAddressMode::eRepeat,
                                    vk::SamplerAddressMode::eRepeat,
                                    vk::SamplerAddressMode::eRepeat,
                                    0.0f,
                                    anisotropyEnabled ? VK_TRUE : VK_FALSE,
                                    anisotropyEnabled ? 16.0f : 1.0f,
                                    VK_FALSE,
                                    vk::CompareOp::eAlways,
                                    0.0f,
                                    0.0f,
                                    vk::BorderColor::eIntOpaqueBlack,
                                    VK_FALSE};

  return context_->device().createSampler(samplerInfo);
}

void TextureManager::transitionImageLayout(vk::Image image, vk::ImageLayout oldLayout,
                                           vk::ImageLayout newLayout) {
  vk::CommandBuffer cmd = context_->beginSingleTimeCommands();

  vk::ImageMemoryBarrier barrier{
      {},
      {},
      oldLayout,
      newLayout,
      VK_QUEUE_FAMILY_IGNORED,
      VK_QUEUE_FAMILY_IGNORED,
      image,
      {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
  };

  vk::PipelineStageFlags srcStage;
  vk::PipelineStageFlags dstStage;

  if (oldLayout == vk::ImageLayout::eUndefined &&
      newLayout == vk::ImageLayout::eTransferDstOptimal) {
    barrier.srcAccessMask = {};
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
    srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
    dstStage = vk::PipelineStageFlagBits::eTransfer;
  } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
             newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
    srcStage = vk::PipelineStageFlagBits::eTransfer;
    dstStage = vk::PipelineStageFlagBits::eFragmentShader;
  } else {
    throw std::runtime_error("Unsupported layout transition");
  }

  cmd.pipelineBarrier(srcStage, dstStage, {}, {}, {}, barrier);

  context_->endSingleTimeCommands(cmd);
}

void TextureManager::copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width,
                                       uint32_t height) {
  vk::CommandBuffer cmd = context_->beginSingleTimeCommands();

  vk::BufferImageCopy region{
      0, 0, 0, {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
         {0, 0, 0},
         {width, height, 1}
  };

  cmd.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);

  context_->endSingleTimeCommands(cmd);
}

uint32_t TextureManager::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
  vk::PhysicalDeviceMemoryProperties memProperties =
      context_->physicalDevice().getMemoryProperties();

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) &&
        (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }

  throw std::runtime_error("Failed to find suitable memory type");
}

} // namespace w3d::gfx
