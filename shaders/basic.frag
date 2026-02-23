#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

// UBO – now includes scene lighting (Phase 6.1)
layout(set = 0, binding = 0) uniform UniformBufferObject {
  mat4 model;
  mat4 view;
  mat4 proj;
  vec4 lightDirection; // xyz = direction toward light source (not normalised here)
  vec4 ambientColor;   // xyz = RGB ambient
  vec4 diffuseColor;   // xyz = RGB diffuse
} ubo;

// Texture sampler
layout(set = 0, binding = 1) uniform sampler2D texSampler;

// Material push constants
layout(push_constant) uniform MaterialData {
  vec4 diffuseColor;   // RGB + alpha
  vec4 emissiveColor;  // RGB + intensity
  vec4 specularColor;  // RGB + shininess
  vec3 hoverTint;      // RGB tint for hover highlighting (1,1,1 = no tint)
  uint flags;          // Material flags
  float alphaThreshold;
  uint useTexture;     // 1 = sample texture
} material;

// Material flags
const uint FLAG_HAS_TEXTURE = 1u;
const uint FLAG_HAS_ALPHA_TEST = 2u;
const uint FLAG_TWO_SIDED = 4u;
const uint FLAG_UNLIT = 8u;

void main() {
  vec3 normal = normalize(fragNormal);

  // Get base color from texture or vertex color
  vec4 baseColor;
  if (material.useTexture == 1u) {
    // UVs already in correct coordinate system (V-flipped during W3D parsing)
    baseColor = texture(texSampler, fragTexCoord);
  } else {
    baseColor = vec4(fragColor, 1.0);
  }

  // Apply material diffuse color
  baseColor *= material.diffuseColor;

  // Alpha test
  if ((material.flags & FLAG_HAS_ALPHA_TEST) != 0u) {
    if (baseColor.a < material.alphaThreshold) {
      discard;
    }
  }

  vec3 result;

  // Check if unlit
  if ((material.flags & FLAG_UNLIT) != 0u) {
    result = baseColor.rgb + material.emissiveColor.rgb;
  } else {
    // Use scene lighting from UBO (populated by LightingState / Renderer).
    // lightDirection.xyz points *toward* the light, so negate for the
    // diffuse dot product (which expects a vector from surface to light).
    vec3 lightDir = normalize(ubo.lightDirection.xyz);

    // Ambient
    vec3 ambient = ubo.ambientColor.rgb * baseColor.rgb;

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = ubo.diffuseColor.rgb * diff * baseColor.rgb;

    // Combine lighting with base color
    result = ambient + diffuse;

    // Add emissive
    result += material.emissiveColor.rgb;
  }

  // Apply hover tint
  result *= material.hoverTint;

  outColor = vec4(result, baseColor.a);
}
