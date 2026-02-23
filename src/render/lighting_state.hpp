#pragma once

#include "lib/formats/map/types.hpp"
#include "lib/gfx/pipeline.hpp"

#include <glm/glm.hpp>

namespace w3d {

/**
 * Manages the scene's active lighting state for Phase 6 – Lighting & Polish.
 *
 * Wraps a map::GlobalLighting struct and provides:
 *   - Time-of-day switching (Morning / Afternoon / Evening / Night).
 *   - Terrain push-constant generation (6.1 ambient/diffuse/direction +
 *     6.2 shadow color + 6.3 cloud shadow animation parameters).
 *   - Object lighting accessors for the UBO (separate from terrain lights).
 *   - Per-frame update() to advance cloud shadow animation time.
 *
 * When no GlobalLighting has been set (e.g. in model-viewer mode) the class
 * returns safe hard-coded defaults so the rest of the rendering code never
 * needs to branch on "do we have a map loaded?".
 */
class LightingState {
public:
  LightingState() = default;

  // ── GlobalLighting ────────────────────────────────────────────────────────

  /** Load a parsed GlobalLighting chunk and switch to its stored time-of-day. */
  void setGlobalLighting(const map::GlobalLighting &lighting);

  /** True once setGlobalLighting() has been called at least once. */
  bool hasLighting() const { return hasLighting_; }

  // ── Time-of-day ──────────────────────────────────────────────────────────

  /**
   * Change the active time-of-day slot.  Clamps to Morning when the
   * requested value is TimeOfDay::Invalid.
   */
  void setTimeOfDay(map::TimeOfDay tod);

  map::TimeOfDay timeOfDay() const { return timeOfDay_; }

  // ── Terrain push constant (6.1 + 6.2 + 6.3) ─────────────────────────────

  /**
   * Build a TerrainPushConstant for the current time-of-day that includes:
   *   - ambient / diffuse / lightDirection from terrainLights[0]
   *   - shadow color decoded from GlobalLighting::shadowColor (6.2)
   *   - cloud animation parameters (6.3)
   *
   * @param hasAtlas  Sets useTexture = 1 when an atlas is bound.
   */
  [[nodiscard]] gfx::TerrainPushConstant makeTerrainPushConstant(bool hasAtlas) const;

  // ── Object lighting (UBO / Phase 6.1) ────────────────────────────────────

  /** RGB ambient for the currently active objectLights[0] slot. */
  [[nodiscard]] glm::vec3 objectAmbient() const;

  /** RGB diffuse for the currently active objectLights[0] slot. */
  [[nodiscard]] glm::vec3 objectDiffuse() const;

  /** Light-source direction (lightPos) for the currently active objectLights[0] slot. */
  [[nodiscard]] glm::vec3 objectLightDirection() const;

  // ── Cloud shadows (Phase 6.3) ─────────────────────────────────────────────

  /**
   * Enable cloud shadows with the given scroll speeds and strength.
   *
   * @param scrollU  Horizontal scroll speed in UV units per second.
   * @param scrollV  Vertical scroll speed in UV units per second.
   * @param strength Shadow intensity [0 = none, 1 = full].
   */
  void setCloudShadow(float scrollU, float scrollV, float strength);

  /** Disable cloud shadows (sets strength to 0). */
  void disableCloudShadow();

  /**
   * Advance the cloud animation by deltaSeconds.
   * Call once per frame from the game/render loop.
   */
  void update(float deltaSeconds);

  // ── Defaults (used when no map is loaded) ────────────────────────────────

  /** Hard-coded diffuse-only directional light matching the pre-map viewer defaults. */
  static const glm::vec3 kDefaultLightDirection;
  static const glm::vec3 kDefaultAmbient;
  static const glm::vec3 kDefaultDiffuse;

private:
  /** Returns the currently active time-of-day slot index (0–3). */
  int32_t activeSlotIndex() const;

  map::GlobalLighting lighting_{};
  bool hasLighting_ = false;
  map::TimeOfDay timeOfDay_ = map::TimeOfDay::Morning;

  // Cloud shadow state
  float cloudScrollU_ = 0.0f;
  float cloudScrollV_ = 0.0f;
  float cloudTime_ = 0.0f;
  float cloudStrength_ = 0.0f;
};

} // namespace w3d
