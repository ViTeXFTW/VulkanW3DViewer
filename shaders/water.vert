#version 450

// ──────────────────────────────────────────────────────────────────────────────
// water.vert  –  Water surface vertex shader
//
// Generates two sets of scrolling UV coordinates for the two-layer water
// texture effect used in C&C Generals: Zero Hour.
// ──────────────────────────────────────────────────────────────────────────────

layout(set = 0, binding = 0) uniform UniformBufferObject {
  mat4 model;
  mat4 view;
  mat4 proj;
} ubo;

// Push constants (matched with WaterPushConstant in pipeline.hpp)
layout(push_constant) uniform WaterParams {
  vec4  waterColor;    // rgba; a = base opacity
  float time;          // elapsed seconds for UV animation
  float uScrollRate;   // primary layer scroll speed in U
  float vScrollRate;   // primary layer scroll speed in V
  float uvScale;       // world-units-to-UV scale (tiles per MAP_XY_FACTOR)
} params;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord; // normalised world-space XZ (1 unit = 1 map cell)

layout(location = 0) out vec2 fragUV1; // primary scroll layer
layout(location = 1) out vec2 fragUV2; // secondary scroll layer (opposite direction)

void main() {
  vec4 worldPos = ubo.model * vec4(inPosition, 1.0);
  gl_Position   = ubo.proj * ubo.view * worldPos;

  vec2 base = inTexCoord * params.uvScale;

  // Primary layer scrolls at (uScrollRate, vScrollRate).
  fragUV1 = base + vec2(params.uScrollRate * params.time,
                         params.vScrollRate * params.time);

  // Secondary layer scrolls at 70 % speed in the perpendicular direction to
  // give a cross-ripple appearance (matches the original SAGE water look).
  float s = params.uScrollRate * 0.7;
  float t = params.vScrollRate * 0.7;
  fragUV2 = base + vec2(-t * params.time,
                          s * params.time);
}
