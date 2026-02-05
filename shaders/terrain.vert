#version 450

// Uniform buffer object for transformation matrices
layout(set = 0, binding = 0) uniform UniformBufferObject {
  mat4 model;
  mat4 view;
  mat4 proj;
} ubo;

// Vertex input attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inColor;

// Output to fragment shader
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragWorldPos;

void main() {
  // Transform vertex position to world space
  vec4 worldPos = ubo.model * vec4(inPosition, 1.0);

  // Transform to clip space
  gl_Position = ubo.proj * ubo.view * worldPos;

  // Pass data to fragment shader
  fragColor = inColor;
  fragTexCoord = inTexCoord;

  // Transform normal to world space (using normal matrix)
  // For terrain, normals are typically pointing up, but we transform properly
  fragNormal = mat3(transpose(inverse(ubo.model))) * inNormal;
  fragWorldPos = worldPos.xyz;
}
