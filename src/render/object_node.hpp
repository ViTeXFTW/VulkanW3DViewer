#pragma once

#include <glm/glm.hpp>

#include <functional>
#include <memory>

#include "lib/formats/w3d/hlod_model.hpp"
#include "lib/scene/scene_node.hpp"
#include "render/object_resolver.hpp"
#include "render/skeleton.hpp"

namespace w3d {

class ObjectNode : public scene::SceneNode {
public:
  explicit ObjectNode(HLodModel *model);
  ~ObjectNode() override = default;

  ObjectNode(const ObjectNode &) = delete;
  ObjectNode &operator=(const ObjectNode &) = delete;

  const char *typeName() const override { return "ObjectNode"; }

  HLodModel *model() { return model_; }
  const HLodModel *model() const { return model_; }

  bool isValid() const { return model_ != nullptr && model_->isValid(); }

  template <typename UpdateModelMatrixFunc>
  void draw(vk::CommandBuffer cmd, UpdateModelMatrixFunc updateModelMatrix) const;

  void setPose(const SkeletonPose *pose) { pose_ = pose; }
  const SkeletonPose *pose() const { return pose_; }

  static std::unique_ptr<ObjectNode> fromMapObject(const map::MapObject &mapObj, HLodModel *model);

private:
  HLodModel *model_ = nullptr;
  const SkeletonPose *pose_ = nullptr;
};

template <typename UpdateModelMatrixFunc>
void ObjectNode::draw(vk::CommandBuffer cmd, UpdateModelMatrixFunc updateModelMatrix) const {
  if (!model_ || !model_->isValid())
    return;

  glm::mat4 world = worldTransform();

  model_->drawWithBoneTransforms(cmd, pose_, [&](const glm::mat4 &boneTransform) {
    updateModelMatrix(world * boneTransform);
  });
}

} // namespace w3d
