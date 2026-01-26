#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
  mat4 model;
  mat4 view;
  mat4 proj;
} ubo;

// Bone matrices storage buffer (SSBO)
layout(set = 0, binding = 2) readonly buffer BoneMatrices {
  mat4 bones[];
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inColor;
layout(location = 4) in uint inBoneIndex;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragWorldPos;

void main() {
  // Rigid skinning (matches legacy Matrix3D::Transform_Vector)
  // Each vertex is influenced by exactly one bone
  mat4 boneMatrix = bones[inBoneIndex];

  // Transform position by bone matrix, then by model matrix
  vec4 skinnedPos = boneMatrix * vec4(inPosition, 1.0);
  vec4 worldPos = ubo.model * skinnedPos;

  gl_Position = ubo.proj * ubo.view * worldPos;

  fragColor = inColor;
  fragTexCoord = inTexCoord;

  // Transform normal: rotation only (legacy clears translation before transforming normals)
  // Extract rotation from bone matrix (upper 3x3)
  mat3 boneRotation = mat3(boneMatrix);
  // Apply bone rotation then model normal matrix
  mat3 normalMatrix = mat3(transpose(inverse(ubo.model)));
  fragNormal = normalMatrix * (boneRotation * inNormal);

  fragWorldPos = worldPos.xyz;
}
