#pragma once

#include <vulkan/vulkan.hpp>

#include "lib/gfx/bounding_box.hpp"

namespace w3d::gfx {

class IRenderable {
public:
  virtual ~IRenderable() = default;

  virtual void draw(vk::CommandBuffer cmd) = 0;

  virtual const BoundingBox &bounds() const = 0;

  virtual const char *typeName() const = 0;

  virtual bool isValid() const = 0;
};

} // namespace w3d::gfx
