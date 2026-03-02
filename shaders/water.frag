#version 450

// ──────────────────────────────────────────────────────────────────────────────
// water.frag  –  Water surface fragment shader
//
// Samples a water texture at two scrolling UV offsets and blends them for a
// ripple effect.  The final alpha is controlled by the waterColor push constant
// (alpha channel) and the WaterTransparency INI settings forwarded via the
// minOpacity push constant.
// ──────────────────────────────────────────────────────────────────────────────

layout(location = 0) in vec2 fragUV1;
layout(location = 1) in vec2 fragUV2;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D waterTexture;

layout(push_constant) uniform WaterParams {
  vec4  waterColor;    // rgba; a = base opacity
  float time;
  float uScrollRate;
  float vScrollRate;
  float uvScale;
} params;

void main() {
  // Sample the water texture at two scrolling offsets.
  vec4 sample1 = texture(waterTexture, fragUV1);
  vec4 sample2 = texture(waterTexture, fragUV2);

  // Average the two layers for a cross-ripple look.
  vec4 blended = mix(sample1, sample2, 0.5);

  // Tint by the configured water diffuse color.
  vec3 color = blended.rgb * params.waterColor.rgb;

  // Use the push-constant alpha for overall water transparency.
  float alpha = params.waterColor.a;

  outColor = vec4(color, alpha);
}
