#include "scene.hpp"

namespace w3d::scene {

void Scene::addRenderable(gfx::IRenderable *renderable) {
  if (renderable) {
    renderables_.push_back(renderable);
  }
}

void Scene::removeRenderable(gfx::IRenderable *renderable) {
  auto it = std::remove(renderables_.begin(), renderables_.end(), renderable);
  renderables_.erase(it, renderables_.end());
}

void Scene::clear() {
  renderables_.clear();
}

} // namespace w3d::scene
