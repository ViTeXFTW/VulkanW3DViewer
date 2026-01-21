#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <string>

namespace w3d {

// Blend mode for transparent materials
enum class BlendMode : uint8_t {
  Opaque = 0,     // No blending
  AlphaBlend = 1, // Standard alpha blending (src_alpha, 1-src_alpha)
  Additive = 2,   // Additive blending (one, one)
  AlphaTest = 3   // Alpha testing (discard below threshold)
};

// GPU material data (matches shader uniform)
struct GPUMaterial {
  alignas(16) glm::vec4 diffuseColor;  // RGB + alpha
  alignas(16) glm::vec4 emissiveColor; // RGB + intensity
  alignas(16) glm::vec4 specularColor; // RGB + shininess
  alignas(4) uint32_t textureIndex;    // Index into texture array (0 = no texture)
  alignas(4) uint32_t flags;           // Bit flags for material options
  alignas(4) float alphaThreshold;     // For alpha testing
  alignas(4) float padding;
};

// Material flags
namespace MaterialFlags {
constexpr uint32_t HasTexture = 1 << 0;
constexpr uint32_t HasAlphaTest = 1 << 1;
constexpr uint32_t TwoSided = 1 << 2;
constexpr uint32_t Unlit = 1 << 3;
} // namespace MaterialFlags

// CPU-side material definition
struct Material {
  std::string name;

  // Colors (0-1 range)
  glm::vec3 diffuse{0.8f, 0.8f, 0.8f};
  glm::vec3 emissive{0.0f, 0.0f, 0.0f};
  glm::vec3 specular{0.2f, 0.2f, 0.2f};
  glm::vec3 ambient{0.1f, 0.1f, 0.1f};

  float opacity = 1.0f;
  float shininess = 32.0f;

  // Texture name (empty = no texture)
  std::string textureName;
  uint32_t textureIndex = 0;

  // Rendering mode
  BlendMode blendMode = BlendMode::Opaque;
  float alphaThreshold = 0.5f;

  // Flags
  bool twoSided = false;
  bool unlit = false;

  // Convert to GPU format
  GPUMaterial toGPU() const {
    GPUMaterial gpu;
    gpu.diffuseColor = glm::vec4(diffuse, opacity);
    gpu.emissiveColor = glm::vec4(emissive, 1.0f);
    gpu.specularColor = glm::vec4(specular, shininess);
    gpu.textureIndex = textureIndex;
    gpu.alphaThreshold = alphaThreshold;

    gpu.flags = 0;
    if (textureIndex > 0) {
      gpu.flags |= MaterialFlags::HasTexture;
    }
    if (blendMode == BlendMode::AlphaTest) {
      gpu.flags |= MaterialFlags::HasAlphaTest;
    }
    if (twoSided) {
      gpu.flags |= MaterialFlags::TwoSided;
    }
    if (unlit) {
      gpu.flags |= MaterialFlags::Unlit;
    }

    gpu.padding = 0.0f;
    return gpu;
  }
};

// Create a default material
inline Material createDefaultMaterial() {
  Material mat;
  mat.name = "__default__";
  mat.diffuse = glm::vec3(0.8f, 0.8f, 0.8f);
  mat.specular = glm::vec3(0.2f, 0.2f, 0.2f);
  mat.shininess = 32.0f;
  mat.opacity = 1.0f;
  return mat;
}

} // namespace w3d
