#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

// Texture sampler
layout(set = 0, binding = 1) uniform sampler2D texSampler;

// Material push constants
layout(push_constant) uniform MaterialData {
  vec4 diffuseColor;   // RGB + alpha
  vec4 emissiveColor;  // RGB + intensity
  vec4 specularColor;  // RGB + shininess
  uint flags;          // Material flags
  float alphaThreshold;
  uint useTexture;     // 1 = sample texture
  float padding;
} material;

// Material flags
const uint FLAG_HAS_TEXTURE = 1u;
const uint FLAG_HAS_ALPHA_TEST = 2u;
const uint FLAG_TWO_SIDED = 4u;
const uint FLAG_UNLIT = 8u;

// Simple directional light
const vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
const vec3 lightColor = vec3(1.0, 1.0, 1.0);
const float ambientStrength = 0.3;

void main() {
  vec3 normal = normalize(fragNormal);

  // Get base color from texture or vertex color
  vec4 baseColor;
  if (material.useTexture == 1u) {
    // Flip V coordinate - W3D uses OpenGL convention (0,0 at bottom-left)
    // while DDS/Vulkan expects (0,0 at top-left)
    vec2 flippedUV = vec2(fragTexCoord.x, 1.0 - fragTexCoord.y);
    baseColor = texture(texSampler, flippedUV);
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
    // Ambient
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Combine lighting with base color
    result = (ambient + diffuse) * baseColor.rgb;

    // Add emissive
    result += material.emissiveColor.rgb;
  }

  outColor = vec4(result, baseColor.a);
}
