#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include <embedded_shaders.hpp>

namespace w3d {

/**
 * Loads shader code from embedded SPIR-V data
 * @param shaderName The name of the shader (e.g., "basic.vert.spv")
 * @return Vector containing the shader bytecode
 * @throws std::runtime_error if shader not found
 */
inline std::vector<char> loadEmbeddedShader(const std::string &shaderName) {
  auto shaderData = shaders::getShader(shaderName);

  if (shaderData.empty()) {
    throw std::runtime_error("Failed to load embedded shader: " + shaderName);
  }

  // Convert std::span<const uint8_t> to std::vector<char>
  return std::vector<char>(reinterpret_cast<const char *>(shaderData.data()),
                           reinterpret_cast<const char *>(shaderData.data()) + shaderData.size());
}

} // namespace w3d
