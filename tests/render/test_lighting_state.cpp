// tests/render/test_lighting_state.cpp
// Unit tests for the LightingState class (Phase 6.1 - time-of-day lighting).
//
// No Vulkan dependency – LightingState is pure CPU-side data management.

#include "render/lighting_state.hpp"

#include <gtest/gtest.h>

using namespace w3d;
using namespace map;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static GlobalLighting makeTestLighting() {
  GlobalLighting gl;
  gl.currentTimeOfDay = TimeOfDay::Morning;

  // Morning slot (index 0): bright yellow-white sun from upper-left
  auto &morning = gl.timeOfDaySlots[0];
  morning.terrainLights[0].ambient = glm::vec3(0.3f, 0.3f, 0.25f);
  morning.terrainLights[0].diffuse = glm::vec3(0.9f, 0.85f, 0.7f);
  morning.terrainLights[0].lightPos = glm::vec3(-1.0f, -1.0f, 0.5f);
  morning.objectLights[0].ambient = glm::vec3(0.25f, 0.25f, 0.2f);
  morning.objectLights[0].diffuse = glm::vec3(0.85f, 0.80f, 0.65f);
  morning.objectLights[0].lightPos = glm::vec3(-0.9f, -1.0f, 0.4f);

  // Afternoon slot (index 1): bright white midday sun from above
  auto &afternoon = gl.timeOfDaySlots[1];
  afternoon.terrainLights[0].ambient = glm::vec3(0.4f, 0.4f, 0.4f);
  afternoon.terrainLights[0].diffuse = glm::vec3(1.0f, 1.0f, 0.95f);
  afternoon.terrainLights[0].lightPos = glm::vec3(0.0f, -1.0f, 0.0f);
  afternoon.objectLights[0].ambient = glm::vec3(0.35f, 0.35f, 0.35f);
  afternoon.objectLights[0].diffuse = glm::vec3(0.95f, 0.95f, 0.9f);
  afternoon.objectLights[0].lightPos = glm::vec3(0.0f, -1.0f, 0.0f);

  // Evening slot (index 2): warm orange-red low sun
  auto &evening = gl.timeOfDaySlots[2];
  evening.terrainLights[0].ambient = glm::vec3(0.2f, 0.15f, 0.1f);
  evening.terrainLights[0].diffuse = glm::vec3(1.0f, 0.5f, 0.2f);
  evening.terrainLights[0].lightPos = glm::vec3(1.0f, -0.5f, 0.0f);
  evening.objectLights[0].ambient = glm::vec3(0.15f, 0.12f, 0.08f);
  evening.objectLights[0].diffuse = glm::vec3(0.9f, 0.45f, 0.18f);
  evening.objectLights[0].lightPos = glm::vec3(1.0f, -0.5f, 0.0f);

  // Night slot (index 3): dark blue moonlight
  auto &night = gl.timeOfDaySlots[3];
  night.terrainLights[0].ambient = glm::vec3(0.05f, 0.05f, 0.1f);
  night.terrainLights[0].diffuse = glm::vec3(0.1f, 0.1f, 0.2f);
  night.terrainLights[0].lightPos = glm::vec3(0.0f, -1.0f, 0.5f);
  night.objectLights[0].ambient = glm::vec3(0.04f, 0.04f, 0.08f);
  night.objectLights[0].diffuse = glm::vec3(0.08f, 0.08f, 0.16f);
  night.objectLights[0].lightPos = glm::vec3(0.0f, -1.0f, 0.5f);

  // Shadow color: semi-transparent grey-blue (ARGB 0x80_40_50_60)
  gl.shadowColor = 0x80405060u;

  return gl;
}

// ---------------------------------------------------------------------------
// Default state (no GlobalLighting set)
// ---------------------------------------------------------------------------

TEST(LightingState, DefaultHasNoLighting) {
  LightingState ls;
  EXPECT_FALSE(ls.hasLighting());
}

TEST(LightingState, DefaultTimeOfDayIsMorning) {
  LightingState ls;
  EXPECT_EQ(ls.timeOfDay(), TimeOfDay::Morning);
}

TEST(LightingState, DefaultTerrainPushConstantUsesHardcodedFallback) {
  LightingState ls;
  auto pc = ls.makeTerrainPushConstant(false);
  // With no lighting set the fallback values must be usable (non-zero ambient)
  EXPECT_GT(pc.ambientColor.r + pc.ambientColor.g + pc.ambientColor.b, 0.0f);
}

TEST(LightingState, DefaultObjectAmbientIsNonZero) {
  LightingState ls;
  auto a = ls.objectAmbient();
  EXPECT_GT(a.r + a.g + a.b, 0.0f);
}

// ---------------------------------------------------------------------------
// Setting GlobalLighting
// ---------------------------------------------------------------------------

TEST(LightingState, SetGlobalLightingEnablesLighting) {
  LightingState ls;
  ls.setGlobalLighting(makeTestLighting());
  EXPECT_TRUE(ls.hasLighting());
}

TEST(LightingState, SetGlobalLightingPicksCurrentTimeOfDay) {
  LightingState ls;
  auto gl = makeTestLighting();
  gl.currentTimeOfDay = TimeOfDay::Evening;
  ls.setGlobalLighting(gl);
  EXPECT_EQ(ls.timeOfDay(), TimeOfDay::Evening);
}

// ---------------------------------------------------------------------------
// Time-of-day switching
// ---------------------------------------------------------------------------

TEST(LightingState, SwitchToMorningReturnsMorningLighting) {
  LightingState ls;
  ls.setGlobalLighting(makeTestLighting());
  ls.setTimeOfDay(TimeOfDay::Morning);

  auto pc = ls.makeTerrainPushConstant(false);
  // Morning ambient is (0.3, 0.3, 0.25)
  EXPECT_NEAR(pc.ambientColor.r, 0.3f, 1e-5f);
  EXPECT_NEAR(pc.ambientColor.g, 0.3f, 1e-5f);
  EXPECT_NEAR(pc.ambientColor.b, 0.25f, 1e-5f);
}

TEST(LightingState, SwitchToAfternoonReturnsAfternoonLighting) {
  LightingState ls;
  ls.setGlobalLighting(makeTestLighting());
  ls.setTimeOfDay(TimeOfDay::Afternoon);

  auto pc = ls.makeTerrainPushConstant(false);
  // Afternoon ambient is (0.4, 0.4, 0.4)
  EXPECT_NEAR(pc.ambientColor.r, 0.4f, 1e-5f);
  EXPECT_NEAR(pc.ambientColor.g, 0.4f, 1e-5f);
  EXPECT_NEAR(pc.ambientColor.b, 0.4f, 1e-5f);
}

TEST(LightingState, SwitchToEveningReturnsEveningDiffuse) {
  LightingState ls;
  ls.setGlobalLighting(makeTestLighting());
  ls.setTimeOfDay(TimeOfDay::Evening);

  auto pc = ls.makeTerrainPushConstant(false);
  // Evening diffuse is (1.0, 0.5, 0.2) – orange/red warm sun
  EXPECT_NEAR(pc.diffuseColor.r, 1.0f, 1e-5f);
  EXPECT_NEAR(pc.diffuseColor.g, 0.5f, 1e-5f);
  EXPECT_NEAR(pc.diffuseColor.b, 0.2f, 1e-5f);
}

TEST(LightingState, SwitchToNightReturnsDimLighting) {
  LightingState ls;
  ls.setGlobalLighting(makeTestLighting());
  ls.setTimeOfDay(TimeOfDay::Night);

  auto pc = ls.makeTerrainPushConstant(false);
  // Night ambient is (0.05, 0.05, 0.1) – very dim blue
  EXPECT_NEAR(pc.ambientColor.r, 0.05f, 1e-5f);
  EXPECT_NEAR(pc.ambientColor.g, 0.05f, 1e-5f);
  EXPECT_NEAR(pc.ambientColor.b, 0.10f, 1e-5f);
}

TEST(LightingState, SwitchingTimeOfDayUpdatesCachedValue) {
  LightingState ls;
  ls.setGlobalLighting(makeTestLighting());
  ls.setTimeOfDay(TimeOfDay::Morning);
  EXPECT_EQ(ls.timeOfDay(), TimeOfDay::Morning);
  ls.setTimeOfDay(TimeOfDay::Night);
  EXPECT_EQ(ls.timeOfDay(), TimeOfDay::Night);
}

// ---------------------------------------------------------------------------
// Terrain push constant generation
// ---------------------------------------------------------------------------

TEST(LightingState, MakeTerrainPushConstantUseTextureFlag) {
  LightingState ls;
  ls.setGlobalLighting(makeTestLighting());
  ls.setTimeOfDay(TimeOfDay::Morning);

  auto pcNo = ls.makeTerrainPushConstant(false);
  auto pcYes = ls.makeTerrainPushConstant(true);
  EXPECT_EQ(pcNo.useTexture, 0u);
  EXPECT_EQ(pcYes.useTexture, 1u);
}

TEST(LightingState, MakeTerrainPushConstantLightDirectionMatchesLightPos) {
  LightingState ls;
  ls.setGlobalLighting(makeTestLighting());
  ls.setTimeOfDay(TimeOfDay::Morning);

  auto pc = ls.makeTerrainPushConstant(false);
  // Morning terrainLights[0].lightPos = (-1, -1, 0.5)
  EXPECT_NEAR(pc.lightDirection.x, -1.0f, 1e-5f);
  EXPECT_NEAR(pc.lightDirection.y, -1.0f, 1e-5f);
  EXPECT_NEAR(pc.lightDirection.z, 0.5f, 1e-5f);
}

// ---------------------------------------------------------------------------
// Shadow color
// ---------------------------------------------------------------------------

TEST(LightingState, ShadowColorDecodedCorrectly) {
  LightingState ls;
  auto gl = makeTestLighting();
  // shadowColor = 0x80405060 → A=0x80(128), R=0x40(64), G=0x50(80), B=0x60(96)
  gl.shadowColor = 0x80405060u;
  ls.setGlobalLighting(gl);

  auto pc = ls.makeTerrainPushConstant(false);
  // Decoded to [0,1] range: R=64/255, G=80/255, B=96/255, A=128/255
  EXPECT_NEAR(pc.shadowColor.r, 64.0f / 255.0f, 1e-3f);
  EXPECT_NEAR(pc.shadowColor.g, 80.0f / 255.0f, 1e-3f);
  EXPECT_NEAR(pc.shadowColor.b, 96.0f / 255.0f, 1e-3f);
  EXPECT_NEAR(pc.shadowColor.a, 128.0f / 255.0f, 1e-3f);
}

TEST(LightingState, ZeroShadowColorProducesBlackAlphaZero) {
  LightingState ls;
  auto gl = makeTestLighting();
  gl.shadowColor = 0u;
  ls.setGlobalLighting(gl);

  auto pc = ls.makeTerrainPushConstant(false);
  EXPECT_NEAR(pc.shadowColor.r, 0.0f, 1e-5f);
  EXPECT_NEAR(pc.shadowColor.g, 0.0f, 1e-5f);
  EXPECT_NEAR(pc.shadowColor.b, 0.0f, 1e-5f);
  EXPECT_NEAR(pc.shadowColor.a, 0.0f, 1e-5f);
}

// ---------------------------------------------------------------------------
// Object lighting (separate from terrain)
// ---------------------------------------------------------------------------

TEST(LightingState, ObjectAmbientMatchesMorningObjectSlot) {
  LightingState ls;
  ls.setGlobalLighting(makeTestLighting());
  ls.setTimeOfDay(TimeOfDay::Morning);

  auto a = ls.objectAmbient();
  // Morning objectLights[0].ambient = (0.25, 0.25, 0.2)
  EXPECT_NEAR(a.r, 0.25f, 1e-5f);
  EXPECT_NEAR(a.g, 0.25f, 1e-5f);
  EXPECT_NEAR(a.b, 0.20f, 1e-5f);
}

TEST(LightingState, ObjectDiffuseMatchesAfternoonObjectSlot) {
  LightingState ls;
  ls.setGlobalLighting(makeTestLighting());
  ls.setTimeOfDay(TimeOfDay::Afternoon);

  auto d = ls.objectDiffuse();
  // Afternoon objectLights[0].diffuse = (0.95, 0.95, 0.9)
  EXPECT_NEAR(d.r, 0.95f, 1e-5f);
  EXPECT_NEAR(d.g, 0.95f, 1e-5f);
  EXPECT_NEAR(d.b, 0.90f, 1e-5f);
}

TEST(LightingState, ObjectLightDirectionMatchesNightSlot) {
  LightingState ls;
  ls.setGlobalLighting(makeTestLighting());
  ls.setTimeOfDay(TimeOfDay::Night);

  auto dir = ls.objectLightDirection();
  // Night objectLights[0].lightPos = (0, -1, 0.5)
  EXPECT_NEAR(dir.x, 0.0f, 1e-5f);
  EXPECT_NEAR(dir.y, -1.0f, 1e-5f);
  EXPECT_NEAR(dir.z, 0.5f, 1e-5f);
}

TEST(LightingState, ObjectAndTerrainLightingCanDifferPerSlot) {
  LightingState ls;
  ls.setGlobalLighting(makeTestLighting());
  ls.setTimeOfDay(TimeOfDay::Morning);

  auto pc = ls.makeTerrainPushConstant(false);
  auto objA = ls.objectAmbient();

  // Terrain ambient (0.3, 0.3, 0.25) != object ambient (0.25, 0.25, 0.2)
  EXPECT_NE(pc.ambientColor.r, objA.r);
}

// ---------------------------------------------------------------------------
// Cloud shadow parameters
// ---------------------------------------------------------------------------

TEST(LightingState, CloudShadowDefaultsToZeroStrength) {
  LightingState ls;
  auto pc = ls.makeTerrainPushConstant(false);
  EXPECT_NEAR(pc.cloudStrength, 0.0f, 1e-5f);
}

TEST(LightingState, SetCloudShadowParamsReflectedInPushConstant) {
  LightingState ls;
  ls.setCloudShadow(0.05f, 0.02f, 0.6f);

  auto pc = ls.makeTerrainPushConstant(false);
  EXPECT_NEAR(pc.cloudScrollU, 0.05f, 1e-5f);
  EXPECT_NEAR(pc.cloudScrollV, 0.02f, 1e-5f);
  EXPECT_NEAR(pc.cloudStrength, 0.6f, 1e-5f);
}

TEST(LightingState, CloudTimeAdvancedByUpdate) {
  LightingState ls;
  ls.setCloudShadow(1.0f, 0.0f, 0.5f);

  ls.update(2.5f);
  auto pc = ls.makeTerrainPushConstant(false);
  EXPECT_NEAR(pc.cloudTime, 2.5f, 1e-5f);
}

TEST(LightingState, CloudTimeAccumulatesAcrossMultipleUpdates) {
  LightingState ls;
  ls.setCloudShadow(1.0f, 0.0f, 0.5f);

  ls.update(1.0f);
  ls.update(0.5f);
  ls.update(0.25f);
  auto pc = ls.makeTerrainPushConstant(false);
  EXPECT_NEAR(pc.cloudTime, 1.75f, 1e-4f);
}

TEST(LightingState, DisabledCloudShadowHasZeroStrength) {
  LightingState ls;
  ls.setCloudShadow(0.1f, 0.1f, 0.8f);
  ls.disableCloudShadow();

  auto pc = ls.makeTerrainPushConstant(false);
  EXPECT_NEAR(pc.cloudStrength, 0.0f, 1e-5f);
}
