#include "lib/gfx/pipeline.hpp"

#include <gtest/gtest.h>

using namespace w3d::gfx;

TEST(PipelineCreateInfoTest, StandardPresetHasCorrectDefaults) {
  auto info = PipelineCreateInfo::standard();

  EXPECT_EQ(info.vertShaderPath, "shaders/basic.vert.spv");
  EXPECT_EQ(info.fragShaderPath, "shaders/basic.frag.spv");
  EXPECT_EQ(info.topology, vk::PrimitiveTopology::eTriangleList);

  EXPECT_EQ(info.vertexInput.binding.binding, 0);
  EXPECT_EQ(info.vertexInput.binding.stride, sizeof(Vertex));
  EXPECT_EQ(info.vertexInput.attributes.size(), 4);

  EXPECT_EQ(info.descriptorBindings.size(), 2);
  EXPECT_EQ(info.descriptorBindings[0].binding, 0);
  EXPECT_EQ(info.descriptorBindings[0].descriptorType, vk::DescriptorType::eUniformBuffer);
  EXPECT_EQ(info.descriptorBindings[1].binding, 1);
  EXPECT_EQ(info.descriptorBindings[1].descriptorType, vk::DescriptorType::eCombinedImageSampler);

  EXPECT_EQ(info.pushConstants.size(), 1);
  EXPECT_EQ(info.pushConstants[0].size, sizeof(MaterialPushConstant));
}

TEST(PipelineCreateInfoTest, SkinnedPresetHasCorrectDefaults) {
  auto info = PipelineCreateInfo::skinned();

  EXPECT_EQ(info.vertShaderPath, "shaders/skinned.vert.spv");
  EXPECT_EQ(info.fragShaderPath, "shaders/basic.frag.spv");
  EXPECT_EQ(info.topology, vk::PrimitiveTopology::eTriangleList);

  EXPECT_EQ(info.vertexInput.binding.binding, 0);
  EXPECT_EQ(info.vertexInput.binding.stride, sizeof(SkinnedVertex));
  EXPECT_EQ(info.vertexInput.attributes.size(), 5);

  EXPECT_EQ(info.descriptorBindings.size(), 3);
  EXPECT_EQ(info.descriptorBindings[0].binding, 0);
  EXPECT_EQ(info.descriptorBindings[0].descriptorType, vk::DescriptorType::eUniformBuffer);
  EXPECT_EQ(info.descriptorBindings[1].binding, 1);
  EXPECT_EQ(info.descriptorBindings[1].descriptorType, vk::DescriptorType::eCombinedImageSampler);
  EXPECT_EQ(info.descriptorBindings[2].binding, 2);
  EXPECT_EQ(info.descriptorBindings[2].descriptorType, vk::DescriptorType::eStorageBuffer);

  EXPECT_EQ(info.pushConstants.size(), 1);
  EXPECT_EQ(info.pushConstants[0].size, sizeof(MaterialPushConstant));
}

TEST(PipelineCreateInfoTest, CanModifyConfiguration) {
  auto info = PipelineCreateInfo::standard();

  info.config.enableBlending = true;
  info.config.alphaBlend = true;
  info.config.depthWrite = false;
  info.config.twoSided = true;

  EXPECT_TRUE(info.config.enableBlending);
  EXPECT_TRUE(info.config.alphaBlend);
  EXPECT_FALSE(info.config.depthWrite);
  EXPECT_TRUE(info.config.twoSided);
}

TEST(PipelineCreateInfoTest, CanChangeTopology) {
  auto info = PipelineCreateInfo::standard();

  info.topology = vk::PrimitiveTopology::eLineList;

  EXPECT_EQ(info.topology, vk::PrimitiveTopology::eLineList);
}
