#include "object_node.hpp"

namespace w3d {

ObjectNode::ObjectNode(HLodModel *model) : model_(model) {
  if (model_ && model_->isValid()) {
    setLocalBounds(model_->bounds());
  }
}

std::unique_ptr<ObjectNode> ObjectNode::fromMapObject(const map::MapObject &mapObj,
                                                      HLodModel *model) {
  if (!model)
    return nullptr;

  auto node = std::make_unique<ObjectNode>(model);
  node->setPosition(ObjectResolver::mapPositionToVulkan(mapObj.position));
  node->setRotationY(mapObj.angle);
  return node;
}

} // namespace w3d
