#version 450

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

// Push constant for hover tint
layout(push_constant) uniform HoverData {
  vec3 hoverTint;  // RGB tint for hover highlighting (1,1,1 = no tint)
} hover;

void main() {
  vec3 color = fragColor * hover.hoverTint;
  outColor = vec4(color, 1.0);
}
