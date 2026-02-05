#pragma once

#include <glm/glm.hpp>

#include <memory>
#include <vector>

#include "lib/gfx/renderable.hpp"

namespace w3d::scene {

class Scene {
public:
  Scene() = default;
  ~Scene() = default;

  Scene(const Scene &) = delete;
  Scene &operator=(const Scene &) = delete;

  void addRenderable(gfx::IRenderable *renderable);
  void removeRenderable(gfx::IRenderable *renderable);

  const std::vector<gfx::IRenderable *> &renderables() const { return renderables_; }

  size_t renderableCount() const { return renderables_.size(); }

  void clear();

private:
  std::vector<gfx::IRenderable *> renderables_;
};

} // namespace w3d::scene
