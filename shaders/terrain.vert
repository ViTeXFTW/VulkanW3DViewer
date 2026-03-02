#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
  mat4 model;
  mat4 view;
  mat4 proj;
  // Scene lighting fields (Phase 6.1, present in UBO but not used by the
  // terrain vertex shader – consumed by the fragment shader via push constant).
  vec4 lightDirection;
  vec4 ambientColor;
  vec4 diffuseColor;
} ubo;

layout(push_constant) uniform TerrainMaterial {
  vec4 ambientColor;
  vec4 diffuseColor;
  vec3 lightDirection;
  uint useTexture;
  vec4 shadowColor;
  // Phase 6.3 – cloud shadow animation parameters.
  float cloudScrollU;
  float cloudScrollV;
  float cloudTime;
  float cloudStrength;
  // Phase 2 – map dimensions for SSBO cell index computation.
  uint mapWidth;
  uint mapHeight;
  float mapXYFactor;
  uint useBlendData;
} material;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec2 inAtlasCoord;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragWorldPos;
layout(location = 3) out vec2 fragAtlasCoord;
layout(location = 4) out vec2 fragCloudCoord; // Phase 6.3: animated cloud UV

// World-space scale for the cloud UV.  Smaller values tile the cloud pattern
// more coarsely, matching the original engine's feel.
const float kCloudUVScale = 0.002;

void main() {
  vec4 worldPos = ubo.model * vec4(inPosition, 1.0);
  gl_Position = ubo.proj * ubo.view * worldPos;

  fragNormal = mat3(transpose(inverse(ubo.model))) * inNormal;
  fragTexCoord = inTexCoord;
  fragWorldPos = worldPos.xyz;
  fragAtlasCoord = inAtlasCoord;

  // Phase 6.3: derive cloud UV from world-space X/Z then animate with time.
  vec2 cloudBase = worldPos.xz * kCloudUVScale;
  fragCloudCoord = cloudBase + vec2(material.cloudScrollU * material.cloudTime,
                                    material.cloudScrollV * material.cloudTime);
}
