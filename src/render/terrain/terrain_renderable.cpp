#include "render/terrain/terrain_renderable.hpp"

#include "lib/gfx/vulkan_context.hpp"

namespace w3d::terrain {

TerrainRenderable::~TerrainRenderable() {
  destroy();
}

void TerrainRenderable::load(gfx::VulkanContext &context, const map::HeightMap &heightMap,
                             const map::GlobalLighting &lighting) {
  destroy();

  auto meshData = generateTerrainMesh(heightMap);
  if (meshData.chunks.empty()) {
    return;
  }

  bounds_ = meshData.totalBounds;
  setLighting(lighting);
  uploadChunks(context, meshData);
}

void TerrainRenderable::draw(vk::CommandBuffer cmd) {
  visibleChunkCount_ = 0;

  for (auto &chunk : gpuChunks_) {
    if (chunk.indexCount == 0) {
      continue;
    }

    if (frustumValid_ && !frustum_.isBoxVisible(chunk.bounds)) {
      continue;
    }

    vk::Buffer vb = chunk.vertexBuffer.buffer();
    vk::DeviceSize offset = 0;
    cmd.bindVertexBuffers(0, vb, offset);
    cmd.bindIndexBuffer(chunk.indexBuffer.buffer(), 0, vk::IndexType::eUint32);
    cmd.drawIndexed(chunk.indexCount, 1, 0, 0, 0);
    ++visibleChunkCount_;
  }
}

void TerrainRenderable::updateFrustum(const glm::mat4 &viewProjection) {
  frustum_.extractFromVP(viewProjection);
  frustumValid_ = true;
}

void TerrainRenderable::loadWithBlendData(gfx::VulkanContext &context,
                                          const map::HeightMap &heightMap,
                                          const map::BlendTileData &blendTileData,
                                          const std::vector<TileUV> &tileUVs,
                                          const map::GlobalLighting &lighting) {
  destroy();

  auto meshData = generateTerrainMeshFromBlendData(heightMap, blendTileData, tileUVs);
  if (meshData.chunks.empty()) {
    return;
  }

  bounds_ = meshData.totalBounds;
  setLighting(lighting);
  uploadChunks(context, meshData);
}

void TerrainRenderable::destroy() {
  for (auto &chunk : gpuChunks_) {
    chunk.destroy();
  }
  gpuChunks_.clear();
  bounds_ = gfx::BoundingBox{};
  frustumValid_ = false;
  visibleChunkCount_ = 0;
  atlasTextureIndex_ = ~0u;

  descriptorManager_.destroy();
  pipeline_.destroy();
}

void TerrainRenderable::setLighting(const map::GlobalLighting &lighting) {
  const auto &current = lighting.getCurrentLighting();
  const auto &light = current.terrainLights[0];

  pushConstant_.ambientColor = glm::vec4(light.ambient, 1.0f);
  pushConstant_.diffuseColor = glm::vec4(light.diffuse, 1.0f);
  pushConstant_.lightDirection = light.lightPos;
  pushConstant_.useTexture = hasAtlas() ? 1u : 0u;
}

void TerrainRenderable::initPipeline(gfx::VulkanContext &context,
                                     gfx::TextureManager &textureManager, uint32_t frameCount) {
  pipeline_.create(context, gfx::PipelineCreateInfo::terrain());

  descriptorManager_.create(context, pipeline_.descriptorSetLayout(), frameCount);

  const auto &defaultTex = textureManager.texture(0);
  for (uint32_t i = 0; i < frameCount; ++i) {
    descriptorManager_.updateTexture(i, defaultTex.view, defaultTex.sampler);
  }
}

void TerrainRenderable::initPipelineWithAtlas(gfx::VulkanContext &context,
                                              gfx::TextureManager &textureManager,
                                              const TerrainAtlasData &atlasData,
                                              uint32_t frameCount) {
  pipeline_.create(context, gfx::PipelineCreateInfo::terrain());
  descriptorManager_.create(context, pipeline_.descriptorSetLayout(), frameCount);

  if (atlasData.isValid()) {
    atlasTextureIndex_ = textureManager.createTexture(
        "terrain_atlas", static_cast<uint32_t>(atlasData.atlasWidth),
        static_cast<uint32_t>(atlasData.atlasHeight), atlasData.pixels.data());

    const auto &atlasTex = textureManager.texture(atlasTextureIndex_);
    for (uint32_t i = 0; i < frameCount; ++i) {
      descriptorManager_.updateTexture(i, atlasTex.view, atlasTex.sampler);
    }
    pushConstant_.useTexture = 1u;
  } else {
    const auto &defaultTex = textureManager.texture(0);
    for (uint32_t i = 0; i < frameCount; ++i) {
      descriptorManager_.updateTexture(i, defaultTex.view, defaultTex.sampler);
    }
  }
}

void TerrainRenderable::updateDescriptors(uint32_t frameIndex, vk::Buffer uniformBuffer,
                                          vk::DeviceSize uboSize) {
  descriptorManager_.updateUniformBuffer(frameIndex, uniformBuffer, uboSize);
}

void TerrainRenderable::drawWithPipeline(vk::CommandBuffer cmd, uint32_t frameIndex) {
  if (!hasData()) {
    return;
  }

  cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_.pipeline());
  cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_.layout(), 0,
                         descriptorManager_.descriptorSet(frameIndex), {});

  cmd.pushConstants(pipeline_.layout(), vk::ShaderStageFlagBits::eFragment, 0,
                    sizeof(gfx::TerrainPushConstant), &pushConstant_);

  draw(cmd);
}

void TerrainRenderable::uploadChunks(gfx::VulkanContext &context, const TerrainMeshData &meshData) {
  gpuChunks_.resize(meshData.chunks.size());

  for (size_t i = 0; i < meshData.chunks.size(); ++i) {
    const auto &src = meshData.chunks[i];
    auto &dst = gpuChunks_[i];

    if (src.vertices.empty() || src.indices.empty()) {
      continue;
    }

    dst.vertexBuffer.create(context, src.vertices.data(),
                            sizeof(TerrainVertex) * src.vertices.size(),
                            vk::BufferUsageFlagBits::eVertexBuffer);

    dst.indexBuffer.create(context, src.indices.data(), sizeof(uint32_t) * src.indices.size(),
                           vk::BufferUsageFlagBits::eIndexBuffer);

    dst.indexCount = static_cast<uint32_t>(src.indices.size());
    dst.bounds = src.bounds;
  }
}

} // namespace w3d::terrain
