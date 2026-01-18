#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

// Simple directional light
const vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
const vec3 lightColor = vec3(1.0, 1.0, 1.0);
const float ambientStrength = 0.3;

void main() {
  vec3 normal = normalize(fragNormal);

  // Ambient
  vec3 ambient = ambientStrength * lightColor;

  // Diffuse
  float diff = max(dot(normal, lightDir), 0.0);
  vec3 diffuse = diff * lightColor;

  vec3 result = (ambient + diffuse) * fragColor;
  outColor = vec4(result, 1.0);
}
