#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec2 fragAtlasCoord;
layout(location = 4) in vec2 fragCloudCoord; // Phase 6.3: scrolled cloud UV

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D texSampler;

layout(push_constant) uniform TerrainMaterial {
  vec4 ambientColor;
  vec4 diffuseColor;
  vec3 lightDirection;
  uint useTexture;
  // Phase 6.2 – shadow colour decoded from GlobalLighting::shadowColor (ARGB)
  vec4 shadowColor;
  // Phase 6.3 – cloud shadow animation (scroll speeds + time stored in .vert)
  float cloudScrollU;
  float cloudScrollV;
  float cloudTime;
  float cloudStrength; // 0 = disabled, 1 = full shadow
} material;

// ---------------------------------------------------------------------------
// Simple hash-based 2D noise for procedural cloud shadows.
// Produces smooth values in [0, 1].
// ---------------------------------------------------------------------------
float hash21(vec2 p) {
  p = fract(p * vec2(127.1, 311.7));
  p += dot(p, p + 19.19);
  return fract(p.x * p.y);
}

float smoothNoise(vec2 uv) {
  vec2 i = floor(uv);
  vec2 f = fract(uv);
  vec2 u = f * f * (3.0 - 2.0 * f); // smoothstep

  float a = hash21(i);
  float b = hash21(i + vec2(1.0, 0.0));
  float c = hash21(i + vec2(0.0, 1.0));
  float d = hash21(i + vec2(1.0, 1.0));

  return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

// Two-octave FBM for a more cloud-like pattern.
float cloudPattern(vec2 uv) {
  float v = smoothNoise(uv) * 0.6 + smoothNoise(uv * 2.1 + 4.7) * 0.4;
  return v;
}

void main() {
  vec3 normal = normalize(fragNormal);

  vec3 baseColor;
  if (material.useTexture == 1u) {
    baseColor = texture(texSampler, fragAtlasCoord).rgb;
  } else {
    float height = fragWorldPos.y;
    float t = clamp(height / 100.0, 0.0, 1.0);
    vec3 lowColor = vec3(0.35, 0.55, 0.25);
    vec3 highColor = vec3(0.65, 0.55, 0.40);
    baseColor = mix(lowColor, highColor, t);
  }

  vec3 lightDir = normalize(-material.lightDirection);

  // Ambient
  vec3 ambient = material.ambientColor.rgb * baseColor;

  // Diffuse
  float diff = max(dot(normal, lightDir), 0.0);
  vec3 diffuse = material.diffuseColor.rgb * diff * baseColor;

  vec3 result = ambient + diffuse;

  // Phase 6.2 – shadow colour tint.
  // Apply the shadow colour as a lerp based on its alpha when the surface is
  // facing away from the light (diff == 0 → fully in shadow).
  if (material.shadowColor.a > 0.0) {
    float shadowFactor = (1.0 - diff) * material.shadowColor.a;
    result = mix(result, result * material.shadowColor.rgb, shadowFactor);
  }

  // Phase 6.3 – cloud shadow overlay.
  // Sample a procedural cloud pattern using scrolled world-space UVs and
  // darken the lit surface proportionally to cloudStrength.
  if (material.cloudStrength > 0.0) {
    float cloud = cloudPattern(fragCloudCoord);
    // cloud ∈ [0, 1]; values > 0.5 are "under cloud", values ≤ 0.5 are "in sun".
    float shadow = smoothstep(0.45, 0.65, cloud) * material.cloudStrength;
    result *= (1.0 - shadow * 0.6); // attenuate by up to 60 % (matches original look)
  }

  outColor = vec4(result, 1.0);
}
