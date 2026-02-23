#include "render/lighting_state.hpp"

#include <algorithm>
#include <cstdint>

namespace w3d {

// ── Static defaults ───────────────────────────────────────────────────────────
// Match the hard-coded values previously in basic.frag so that model-viewer
// mode looks the same before any map is loaded.

const glm::vec3 LightingState::kDefaultLightDirection{1.0f, 1.0f, 1.0f}; // toward camera-right/up
const glm::vec3 LightingState::kDefaultAmbient{0.3f, 0.3f, 0.3f};
const glm::vec3 LightingState::kDefaultDiffuse{1.0f, 1.0f, 1.0f};

// ── Helpers ───────────────────────────────────────────────────────────────────

int32_t LightingState::activeSlotIndex() const {
  int32_t idx = static_cast<int32_t>(timeOfDay_) - 1; // TimeOfDay enum: Morning=1 → slot 0
  return std::clamp(idx, 0, map::NUM_TIME_OF_DAY_SLOTS - 1);
}

static glm::vec4 decodeShadowColor(uint32_t argb) {
  float a = static_cast<float>((argb >> 24) & 0xFFu) / 255.0f;
  float r = static_cast<float>((argb >> 16) & 0xFFu) / 255.0f;
  float g = static_cast<float>((argb >> 8) & 0xFFu) / 255.0f;
  float b = static_cast<float>((argb) & 0xFFu) / 255.0f;
  return {r, g, b, a};
}

// ── Public interface ──────────────────────────────────────────────────────────

void LightingState::setGlobalLighting(const map::GlobalLighting &lighting) {
  lighting_ = lighting;
  hasLighting_ = true;
  // Pick the time-of-day stored in the map, but fall back to Morning for Invalid.
  timeOfDay_ = (lighting.currentTimeOfDay != map::TimeOfDay::Invalid) ? lighting.currentTimeOfDay
                                                                       : map::TimeOfDay::Morning;
}

void LightingState::setTimeOfDay(map::TimeOfDay tod) {
  timeOfDay_ = (tod != map::TimeOfDay::Invalid) ? tod : map::TimeOfDay::Morning;
}

gfx::TerrainPushConstant LightingState::makeTerrainPushConstant(bool hasAtlas) const {
  gfx::TerrainPushConstant pc{};

  if (hasLighting_) {
    const auto &slot = lighting_.timeOfDaySlots[activeSlotIndex()];
    const auto &light = slot.terrainLights[0];

    pc.ambientColor = glm::vec4(light.ambient, 1.0f);
    pc.diffuseColor = glm::vec4(light.diffuse, 1.0f);
    pc.lightDirection = light.lightPos;
    pc.shadowColor = decodeShadowColor(lighting_.shadowColor);
  } else {
    // Hard-coded fallback matching the old shader constants
    pc.ambientColor = glm::vec4(kDefaultAmbient, 1.0f);
    pc.diffuseColor = glm::vec4(kDefaultDiffuse, 1.0f);
    pc.lightDirection = kDefaultLightDirection;
    pc.shadowColor = glm::vec4(0.0f);
  }

  pc.useTexture = hasAtlas ? 1u : 0u;

  // Cloud shadow animation
  pc.cloudScrollU = cloudScrollU_;
  pc.cloudScrollV = cloudScrollV_;
  pc.cloudTime = cloudTime_;
  pc.cloudStrength = cloudStrength_;

  return pc;
}

glm::vec3 LightingState::objectAmbient() const {
  if (!hasLighting_) {
    return kDefaultAmbient;
  }
  return lighting_.timeOfDaySlots[activeSlotIndex()].objectLights[0].ambient;
}

glm::vec3 LightingState::objectDiffuse() const {
  if (!hasLighting_) {
    return kDefaultDiffuse;
  }
  return lighting_.timeOfDaySlots[activeSlotIndex()].objectLights[0].diffuse;
}

glm::vec3 LightingState::objectLightDirection() const {
  if (!hasLighting_) {
    return kDefaultLightDirection;
  }
  return lighting_.timeOfDaySlots[activeSlotIndex()].objectLights[0].lightPos;
}

void LightingState::setCloudShadow(float scrollU, float scrollV, float strength) {
  cloudScrollU_ = scrollU;
  cloudScrollV_ = scrollV;
  cloudStrength_ = strength;
}

void LightingState::disableCloudShadow() {
  cloudStrength_ = 0.0f;
}

void LightingState::update(float deltaSeconds) {
  cloudTime_ += deltaSeconds;
}

} // namespace w3d
