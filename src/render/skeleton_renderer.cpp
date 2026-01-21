#include "skeleton_renderer.hpp"

#include "core/vulkan_context.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <map>
#include <stdexcept>

namespace w3d {

SkeletonRenderer::~SkeletonRenderer() {
  destroy();
}

void SkeletonRenderer::create(VulkanContext &context) {
  device_ = context.device();
  createDescriptorSetLayout(context);
  createPipeline(context);
}

void SkeletonRenderer::createDescriptorSetLayout(VulkanContext & /*context*/) {
  // Match the main pipeline layout (UBO + texture sampler) for descriptor set compatibility
  // Even though skeleton shader doesn't use textures, we need compatible layouts to share
  // descriptor sets
  std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {
      vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eUniformBuffer, 1,
                                     vk::ShaderStageFlagBits::eVertex},
      vk::DescriptorSetLayoutBinding{1, vk::DescriptorType::eCombinedImageSampler, 1,
                                     vk::ShaderStageFlagBits::eFragment}};

  vk::DescriptorSetLayoutCreateInfo layoutInfo{{}, bindings};
  descriptorSetLayout_ = device_.createDescriptorSetLayout(layoutInfo);
}

std::vector<char> readShaderFile(const std::string &filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("Failed to open shader file: " + filename);
  }

  size_t fileSize = static_cast<size_t>(file.tellg());
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), static_cast<std::streamsize>(fileSize));

  return buffer;
}

vk::ShaderModule createShaderModule(vk::Device device, const std::vector<char> &code) {
  vk::ShaderModuleCreateInfo createInfo{
      {}, code.size(), reinterpret_cast<const uint32_t *>(code.data())};
  return device.createShaderModule(createInfo);
}

void SkeletonRenderer::createPipeline(VulkanContext &context) {
  // Load skeleton shaders
  auto vertShaderCode = readShaderFile("shaders/skeleton.vert.spv");
  auto fragShaderCode = readShaderFile("shaders/skeleton.frag.spv");

  auto vertShaderModule = createShaderModule(device_, vertShaderCode);
  auto fragShaderModule = createShaderModule(device_, fragShaderCode);

  vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
      {}, vk::ShaderStageFlagBits::eVertex, vertShaderModule, "main"};
  vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
      {}, vk::ShaderStageFlagBits::eFragment, fragShaderModule, "main"};
  std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {vertShaderStageInfo,
                                                                   fragShaderStageInfo};

  // Vertex input - simplified for skeleton
  auto bindingDescription = SkeletonVertex::getBindingDescription();
  auto attributeDescriptions = SkeletonVertex::getAttributeDescriptions();

  vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
      {}, bindingDescription, attributeDescriptions};

  // Dynamic viewport and scissor
  std::array<vk::DynamicState, 2> dynamicStates = {vk::DynamicState::eViewport,
                                                   vk::DynamicState::eScissor};
  vk::PipelineDynamicStateCreateInfo dynamicState{{}, dynamicStates};

  vk::PipelineViewportStateCreateInfo viewportState{{}, 1, nullptr, 1, nullptr};

  // Multisampling
  vk::PipelineMultisampleStateCreateInfo multisampling{{}, vk::SampleCountFlagBits::e1, VK_FALSE};

  // Depth stencil - enable depth test but write slightly closer to avoid z-fighting
  vk::PipelineDepthStencilStateCreateInfo depthStencil{
      {},
      VK_TRUE,  // depthTestEnable
      VK_TRUE,  // depthWriteEnable
      vk::CompareOp::eLessOrEqual,
      VK_FALSE, // depthBoundsTestEnable
      VK_FALSE  // stencilTestEnable
  };

  // Color blending - opaque
  vk::PipelineColorBlendAttachmentState colorBlendAttachment{
      VK_FALSE,
      vk::BlendFactor::eOne,
      vk::BlendFactor::eZero,
      vk::BlendOp::eAdd,
      vk::BlendFactor::eOne,
      vk::BlendFactor::eZero,
      vk::BlendOp::eAdd,
      vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};
  vk::PipelineColorBlendStateCreateInfo colorBlending{
      {}, VK_FALSE, vk::LogicOp::eCopy, colorBlendAttachment};

  // Pipeline layout - use our descriptor set layout
  vk::PipelineLayoutCreateInfo pipelineLayoutInfo{{}, descriptorSetLayout_};
  pipelineLayout_ = device_.createPipelineLayout(pipelineLayoutInfo);

  // Create LINE pipeline
  {
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
        {}, vk::PrimitiveTopology::eLineList, VK_FALSE};

    vk::PipelineRasterizationStateCreateInfo rasterizer{
        {},
        VK_FALSE,                    // depthClampEnable
        VK_FALSE,                    // rasterizerDiscardEnable
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eNone, // No culling for lines
        vk::FrontFace::eCounterClockwise,
        VK_FALSE,                    // depthBiasEnable
        0.0f,
        0.0f,
        0.0f,
        1.0f // lineWidth - wideLines feature required for > 1.0
    };

    vk::GraphicsPipelineCreateInfo pipelineInfo{
        {},
        shaderStages,
        &vertexInputInfo,
        &inputAssembly,
        nullptr, // tessellation
        &viewportState,
        &rasterizer,
        &multisampling,
        &depthStencil,
        &colorBlending,
        &dynamicState,
        pipelineLayout_,
        context.renderPass(),
        0 // subpass
    };

    auto result = device_.createGraphicsPipeline(nullptr, pipelineInfo);
    if (result.result != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to create skeleton line pipeline");
    }
    linePipeline_ = result.value;
  }

  // Create POINT pipeline for joints
  {
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
        {}, vk::PrimitiveTopology::eTriangleList, VK_FALSE};

    vk::PipelineRasterizationStateCreateInfo rasterizer{
        {},
        VK_FALSE,                    // depthClampEnable
        VK_FALSE,                    // rasterizerDiscardEnable
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eNone, // No culling for small spheres
        vk::FrontFace::eCounterClockwise,
        VK_FALSE,                    // depthBiasEnable
        0.0f,
        0.0f,
        0.0f,
        1.0f // lineWidth
    };

    vk::GraphicsPipelineCreateInfo pipelineInfo{
        {},
        shaderStages,
        &vertexInputInfo,
        &inputAssembly,
        nullptr, // tessellation
        &viewportState,
        &rasterizer,
        &multisampling,
        &depthStencil,
        &colorBlending,
        &dynamicState,
        pipelineLayout_,
        context.renderPass(),
        0 // subpass
    };

    auto result = device_.createGraphicsPipeline(nullptr, pipelineInfo);
    if (result.result != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to create skeleton point pipeline");
    }
    pointPipeline_ = result.value;
  }

  // Cleanup shader modules
  device_.destroyShaderModule(vertShaderModule);
  device_.destroyShaderModule(fragShaderModule);
}

std::vector<SkeletonVertex> SkeletonRenderer::generateJointSphere(const glm::vec3 &center,
                                                                  float radius,
                                                                  const glm::vec3 &color) const {
  // Generate an icosphere (subdivided icosahedron) for joints
  // Start with icosahedron vertices
  const float t = (1.0f + std::sqrt(5.0f)) / 2.0f;

  std::vector<glm::vec3> vertices = {
      glm::normalize(glm::vec3(-1, t, 0)),  glm::normalize(glm::vec3(1, t, 0)),
      glm::normalize(glm::vec3(-1, -t, 0)), glm::normalize(glm::vec3(1, -t, 0)),
      glm::normalize(glm::vec3(0, -1, t)),  glm::normalize(glm::vec3(0, 1, t)),
      glm::normalize(glm::vec3(0, -1, -t)), glm::normalize(glm::vec3(0, 1, -t)),
      glm::normalize(glm::vec3(t, 0, -1)),  glm::normalize(glm::vec3(t, 0, 1)),
      glm::normalize(glm::vec3(-t, 0, -1)), glm::normalize(glm::vec3(-t, 0, 1))};

  // Icosahedron faces (triangles)
  std::vector<std::array<int, 3>> faces = {
      {0,  11, 5 },
      {0,  5,  1 },
      {0,  1,  7 },
      {0,  7,  10},
      {0,  10, 11},
      {1,  5,  9 },
      {5,  11, 4 },
      {11, 10, 2 },
      {10, 7,  6 },
      {7,  1,  8 },
      {3,  9,  4 },
      {3,  4,  2 },
      {3,  2,  6 },
      {3,  6,  8 },
      {3,  8,  9 },
      {4,  9,  5 },
      {2,  4,  11},
      {6,  2,  10},
      {8,  6,  7 },
      {9,  8,  1 }
  };

  // Subdivide for smoother appearance
  for (int i = 0; i < kJointSphereDetail; ++i) {
    std::vector<std::array<int, 3>> newFaces;
    std::map<std::pair<int, int>, int> midpointCache;

    auto getMidpoint = [&](int v1, int v2) -> int {
      auto key = std::minmax(v1, v2);
      auto it = midpointCache.find(key);
      if (it != midpointCache.end()) {
        return it->second;
      }
      glm::vec3 midpoint = glm::normalize((vertices[v1] + vertices[v2]) * 0.5f);
      int index = static_cast<int>(vertices.size());
      vertices.push_back(midpoint);
      midpointCache[key] = index;
      return index;
    };

    for (const auto &face : faces) {
      int a = getMidpoint(face[0], face[1]);
      int b = getMidpoint(face[1], face[2]);
      int c = getMidpoint(face[2], face[0]);

      newFaces.push_back({face[0], a, c});
      newFaces.push_back({face[1], b, a});
      newFaces.push_back({face[2], c, b});
      newFaces.push_back({a, b, c});
    }
    faces = std::move(newFaces);
  }

  // Generate output vertices
  std::vector<SkeletonVertex> result;
  result.reserve(faces.size() * 3);

  for (const auto &face : faces) {
    for (int idx : face) {
      SkeletonVertex v;
      v.position = center + vertices[idx] * radius;
      v.color = color;
      result.push_back(v);
    }
  }

  return result;
}

void SkeletonRenderer::updateFromPose(VulkanContext &context, const SkeletonPose &pose) {
  if (!pose.isValid()) {
    lineVertexCount_ = 0;
    jointVertexCount_ = 0;
    return;
  }

  // Calculate skeleton scale for joint size
  float minX = std::numeric_limits<float>::max();
  float maxX = std::numeric_limits<float>::lowest();
  float minY = std::numeric_limits<float>::max();
  float maxY = std::numeric_limits<float>::lowest();
  float minZ = std::numeric_limits<float>::max();
  float maxZ = std::numeric_limits<float>::lowest();

  for (size_t i = 0; i < pose.boneCount(); ++i) {
    glm::vec3 pos = pose.bonePosition(i);
    minX = std::min(minX, pos.x);
    maxX = std::max(maxX, pos.x);
    minY = std::min(minY, pos.y);
    maxY = std::max(maxY, pos.y);
    minZ = std::min(minZ, pos.z);
    maxZ = std::max(maxZ, pos.z);
  }

  float skeletonSize = std::max({maxX - minX, maxY - minY, maxZ - minZ});
  float jointRadius = skeletonSize * kJointSizeRatio;
  jointRadius = std::max(jointRadius, 0.01f); // Minimum size

  // Generate line vertices (bone connections)
  std::vector<SkeletonVertex> lineVertices;
  lineVertices.reserve(pose.boneCount() * 2);

  for (size_t i = 0; i < pose.boneCount(); ++i) {
    int parent = pose.parentIndex(i);
    if (parent >= 0) {
      // Draw line from parent to child
      SkeletonVertex v1, v2;
      v1.position = pose.bonePosition(static_cast<size_t>(parent));
      v1.color = boneColor_;
      v2.position = pose.bonePosition(i);
      v2.color = boneColor_;

      lineVertices.push_back(v1);
      lineVertices.push_back(v2);
    }
  }

  // Generate joint spheres
  std::vector<SkeletonVertex> jointVertices;

  for (size_t i = 0; i < pose.boneCount(); ++i) {
    glm::vec3 pos = pose.bonePosition(i);
    glm::vec3 color = (pose.parentIndex(i) < 0) ? rootColor_ : jointColor_;

    auto sphereVerts = generateJointSphere(pos, jointRadius, color);
    jointVertices.insert(jointVertices.end(), sphereVerts.begin(), sphereVerts.end());
  }

  // Upload line buffer
  if (!lineVertices.empty()) {
    lineBuffer_.destroy();
    lineBuffer_.create(context, lineVertices);
    lineVertexCount_ = static_cast<uint32_t>(lineVertices.size());
  } else {
    lineVertexCount_ = 0;
  }

  // Upload joint buffer
  if (!jointVertices.empty()) {
    jointBuffer_.destroy();
    jointBuffer_.create(context, jointVertices);
    jointVertexCount_ = static_cast<uint32_t>(jointVertices.size());
  } else {
    jointVertexCount_ = 0;
  }
}

void SkeletonRenderer::draw(vk::CommandBuffer cmd) const {
  if (!hasData()) {
    return;
  }

  // Draw bone lines
  if (lineVertexCount_ > 0) {
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, linePipeline_);
    vk::Buffer vertexBuffers[] = {lineBuffer_.buffer()};
    vk::DeviceSize offsets[] = {0};
    cmd.bindVertexBuffers(0, 1, vertexBuffers, offsets);
    cmd.draw(lineVertexCount_, 1, 0, 0);
  }

  // Draw joint spheres
  if (jointVertexCount_ > 0) {
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pointPipeline_);
    vk::Buffer vertexBuffers[] = {jointBuffer_.buffer()};
    vk::DeviceSize offsets[] = {0};
    cmd.bindVertexBuffers(0, 1, vertexBuffers, offsets);
    cmd.draw(jointVertexCount_, 1, 0, 0);
  }
}

void SkeletonRenderer::destroy() {
  lineBuffer_.destroy();
  jointBuffer_.destroy();
  lineVertexCount_ = 0;
  jointVertexCount_ = 0;

  if (device_) {
    if (linePipeline_) {
      device_.destroyPipeline(linePipeline_);
      linePipeline_ = nullptr;
    }
    if (pointPipeline_) {
      device_.destroyPipeline(pointPipeline_);
      pointPipeline_ = nullptr;
    }
    if (pipelineLayout_) {
      device_.destroyPipelineLayout(pipelineLayout_);
      pipelineLayout_ = nullptr;
    }
    if (descriptorSetLayout_) {
      device_.destroyDescriptorSetLayout(descriptorSetLayout_);
      descriptorSetLayout_ = nullptr;
    }
    device_ = nullptr;
  }
}

} // namespace w3d
