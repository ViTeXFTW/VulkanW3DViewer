#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec2 fragAtlasCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D texSampler;

layout(push_constant) uniform TerrainMaterial {
  vec4 ambientColor;
  vec4 diffuseColor;
  vec3 lightDirection;
  uint useTexture;
} material;

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

  vec3 ambient = material.ambientColor.rgb * baseColor;

  float diff = max(dot(normal, lightDir), 0.0);
  vec3 diffuse = material.diffuseColor.rgb * diff * baseColor;

  vec3 result = ambient + diffuse;

  outColor = vec4(result, 1.0);
}
