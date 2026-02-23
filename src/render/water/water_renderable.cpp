#include "render/water/water_renderable.hpp"

#include "lib/gfx/vulkan_context.hpp"

namespace w3d::water {

WaterRenderable::~WaterRenderable() {
  destroy();
}

void WaterRenderable::load(gfx::VulkanContext &context,
                            const std::vector<map::PolygonTrigger> &triggers) {
  destroy();

  auto meshData = generateWaterMeshes(triggers);
  if (meshData.polygons.empty()) {
    return;
  }

  bounds_ = meshData.totalBounds;
  uploadPolygons(context, meshData);
}

void WaterRenderable::update(float deltaSeconds) {
  pushConstant_.time += deltaSeconds;
}

void WaterRenderable::applyWaterSettings(const ini::WaterSettings &settings, ini::TimeOfDay tod) {
  const auto &ws = settings.getForTimeOfDay(tod);
  const auto &tr = settings.transparency;

  // Diffuse tint from the standing-water vertex color (average of 4 corners).
  // RGBAColorInt stores components as int32 in [0, 255].
  auto toF = [](int32_t v) { return static_cast<float>(v) / 255.0f; };

  glm::vec4 avg{0.0f};
  avg.r = (toF(ws.vertex00Diffuse.r) + toF(ws.vertex10Diffuse.r) + toF(ws.vertex11Diffuse.r) +
           toF(ws.vertex01Diffuse.r)) *
          0.25f;
  avg.g = (toF(ws.vertex00Diffuse.g) + toF(ws.vertex10Diffuse.g) + toF(ws.vertex11Diffuse.g) +
           toF(ws.vertex01Diffuse.g)) *
          0.25f;
  avg.b = (toF(ws.vertex00Diffuse.b) + toF(ws.vertex10Diffuse.b) + toF(ws.vertex11Diffuse.b) +
           toF(ws.vertex01Diffuse.b)) *
          0.25f;
  avg.a = tr.minWaterOpacity;

  pushConstant_.waterColor = avg;
  pushConstant_.uScrollRate = ws.uScrollPerMs * 1000.0f; // convert ms→s
  pushConstant_.vScrollRate = ws.vScrollPerMs * 1000.0f;
  // UV scale: waterRepeatCount tiles across the water texture.
  pushConstant_.uvScale = (ws.waterRepeatCount > 0) ? static_cast<float>(ws.waterRepeatCount)
                                                     : 8.0f;
}

void WaterRenderable::initPipeline(gfx::VulkanContext &context,
                                    gfx::TextureManager &textureManager, uint32_t frameCount) {
  pipeline_.create(context, gfx::PipelineCreateInfo::water());
  descriptorManager_.create(context, pipeline_.descriptorSetLayout(), frameCount);

  // Use default white texture until a real water texture is loaded.
  const auto &defaultTex = textureManager.texture(0);
  for (uint32_t i = 0; i < frameCount; ++i) {
    descriptorManager_.updateTexture(i, defaultTex.view, defaultTex.sampler);
  }

  // Sensible defaults so water is visible without INI.
  pushConstant_.waterColor  = glm::vec4{0.35f, 0.55f, 0.85f, 0.75f};
  pushConstant_.uScrollRate = 0.05f;
  pushConstant_.vScrollRate = 0.03f;
  pushConstant_.uvScale     = 8.0f;
  pushConstant_.time        = 0.0f;
}

void WaterRenderable::updateDescriptors(uint32_t frameIndex, vk::Buffer uniformBuffer,
                                         vk::DeviceSize uboSize) {
  descriptorManager_.updateUniformBuffer(frameIndex, uniformBuffer, uboSize);
}

void WaterRenderable::draw(vk::CommandBuffer cmd) {
  for (const auto &poly : gpuPolygons_) {
    if (poly.indexCount == 0) {
      continue;
    }
    vk::Buffer vb = poly.vertexBuffer.buffer();
    vk::DeviceSize offset = 0;
    cmd.bindVertexBuffers(0, vb, offset);
    cmd.bindIndexBuffer(poly.indexBuffer.buffer(), 0, vk::IndexType::eUint32);
    cmd.drawIndexed(poly.indexCount, 1, 0, 0, 0);
  }
}

void WaterRenderable::drawWithPipeline(vk::CommandBuffer cmd, uint32_t frameIndex) {
  if (!hasData()) {
    return;
  }

  cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_.pipeline());
  cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_.layout(), 0,
                          descriptorManager_.descriptorSet(frameIndex), {});

  cmd.pushConstants(pipeline_.layout(),
                    vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0,
                    sizeof(gfx::WaterPushConstant), &pushConstant_);

  draw(cmd);
}

void WaterRenderable::destroy() {
  for (auto &poly : gpuPolygons_) {
    poly.destroy();
  }
  gpuPolygons_.clear();
  bounds_ = gfx::BoundingBox{};

  descriptorManager_.destroy();
  pipeline_.destroy();

  waterTextureIndex_ = ~0u;
  pushConstant_      = gfx::WaterPushConstant{};
}

void WaterRenderable::uploadPolygons(gfx::VulkanContext &context, const WaterMeshData &meshData) {
  gpuPolygons_.resize(meshData.polygons.size());

  for (size_t i = 0; i < meshData.polygons.size(); ++i) {
    const auto &src = meshData.polygons[i];
    auto &dst       = gpuPolygons_[i];

    if (src.vertices.empty() || src.indices.empty()) {
      continue;
    }

    dst.vertexBuffer.create(context, src.vertices.data(),
                             sizeof(WaterVertex) * src.vertices.size(),
                             vk::BufferUsageFlagBits::eVertexBuffer);

    dst.indexBuffer.create(context, src.indices.data(),
                            sizeof(uint32_t) * src.indices.size(),
                            vk::BufferUsageFlagBits::eIndexBuffer);

    dst.indexCount = static_cast<uint32_t>(src.indices.size());
    dst.bounds     = src.bounds;
  }
}

} // namespace w3d::water
