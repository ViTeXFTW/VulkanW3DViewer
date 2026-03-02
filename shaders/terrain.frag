#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec2 fragAtlasCoord;
layout(location = 4) in vec2 fragCloudCoord; // Phase 6.3: scrolled cloud UV

layout(location = 0) out vec4 outColor;

// Phase 1.4 – texture array: one layer per 64x64 terrain tile.
// Falls back gracefully when useTexture == 0 (procedural gradient).
layout(set = 0, binding = 1) uniform sampler2DArray tileTextures;

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
  // Phase 2 – map dimensions for SSBO cell index computation
  uint mapWidth;
  uint mapHeight;
  float mapXYFactor;
  uint useBlendData;
} material;

// ---------------------------------------------------------------------------
// Phase 2 – per-cell blend data SSBO.
// One CellBlendInfo entry per terrain cell (row-major, Z increasing forward).
// Each entry is 16 bytes = 4 x uint32 in std430 layout:
//   word0: baseTileIndex(u16) | baseQuadrant(u16)
//   word1: blendTileIndex(u16) | blendQuadrant(u16)
//   word2: extraTileIndex(u16) | extraQuadrant(u16)
//   word3: blendDir(u8) | extraDir(u8) | flags(u8) | padding(u8)
// ---------------------------------------------------------------------------
layout(std430, set = 0, binding = 2) readonly buffer CellBlendBuffer {
  uint cellData[];
};

// Blend direction encoding must match BlendDirectionEncoding in terrain_blend_data.hpp
const uint BLEND_NONE              = 0u;
const uint BLEND_HORIZ             = 1u;
const uint BLEND_HORIZ_INV         = 2u;
const uint BLEND_VERT              = 3u;
const uint BLEND_VERT_INV          = 4u;
const uint BLEND_DIAG_R            = 5u;
const uint BLEND_DIAG_R_INV        = 6u;
const uint BLEND_DIAG_L            = 7u;
const uint BLEND_DIAG_L_INV        = 8u;
const uint BLEND_LONG_DIAG         = 9u;
const uint BLEND_LONG_DIAG_INV     = 10u;
const uint BLEND_LONG_DIAG_ALT     = 11u;
const uint BLEND_LONG_DIAG_ALT_INV = 12u;
// Phase 5.5: custom blend edge texture -- blendQuadrant holds the GPU layer index of the
// edge tile whose alpha channel is sampled to drive the blend mask.
const uint BLEND_CUSTOM_EDGE       = 13u;

const uint CELL_FLAG_IS_CLIFF = 0x01u;

// Each CellBlendInfo occupies 4 uint32s in std430 layout:
// word0: baseTileIndex (u16) | baseQuadrant (u16)
// word1: blendTileIndex (u16) | blendQuadrant (u16)
// word2: extraTileIndex (u16) | extraQuadrant (u16)
// word3: blendDir (u8) | extraDir (u8) | flags (u8) | padding (u8)
const uint WORDS_PER_CELL = 4u;

uint getCellWord(uint cellIndex, uint wordOffset) {
  return cellData[cellIndex * WORDS_PER_CELL + wordOffset];
}

uint getCellBaseTile(uint cellIndex) {
  return getCellWord(cellIndex, 0u) & 0xFFFFu;
}

uint getCellBaseQuadrant(uint cellIndex) {
  return (getCellWord(cellIndex, 0u) >> 16u) & 0xFFFFu;
}

uint getCellBlendTile(uint cellIndex) {
  return getCellWord(cellIndex, 1u) & 0xFFFFu;
}

uint getCellBlendQuadrant(uint cellIndex) {
  return (getCellWord(cellIndex, 1u) >> 16u) & 0xFFFFu;
}

uint getCellExtraTile(uint cellIndex) {
  return getCellWord(cellIndex, 2u) & 0xFFFFu;
}

uint getCellExtraQuadrant(uint cellIndex) {
  return (getCellWord(cellIndex, 2u) >> 16u) & 0xFFFFu;
}

uint getCellBlendDir(uint cellIndex) {
  return getCellWord(cellIndex, 3u) & 0xFFu;
}

uint getCellExtraDir(uint cellIndex) {
  return (getCellWord(cellIndex, 3u) >> 8u) & 0xFFu;
}

uint getCellFlags(uint cellIndex) {
  return (getCellWord(cellIndex, 3u) >> 16u) & 0xFFu;
}

// ---------------------------------------------------------------------------
// Compute UV within a 64x64 tile given the in-cell fraction [0,1] x [0,1]
// and the 32x32 quadrant index encoded as (bit1 << 1 | bit0).
//
// Original engine encoding (WorldHeightMap::getUVForNdx):
//   bit0 (& 1): 0 = left half,   1 = right half
//   bit1 (& 2): 0 = bottom half, 1 = top half  (Y flipped due to TGA convention)
//
// Since the TGA decoder already flips the image vertically (V=0 is top),
// bit1=1 maps to vOffset=0 (top) and bit1=0 maps to vOffset=0.5 (bottom).
// ---------------------------------------------------------------------------
vec2 quadrantUV(vec2 cellFrac, uint quadrant) {
  float uOffset = float(quadrant & 1u) * 0.5;
  float vOffset = float(1u - ((quadrant >> 1u) & 1u)) * 0.5;
  return vec2(uOffset + cellFrac.x * 0.5, vOffset + cellFrac.y * 0.5);
}

// ---------------------------------------------------------------------------
// Compute the blend alpha [0,1] for a given direction and in-cell UV.
// ---------------------------------------------------------------------------
float blendAlpha(uint direction, vec2 uv) {
  float u = uv.x;
  float v = uv.y;
  switch (direction) {
    case BLEND_HORIZ:         return u;
    case BLEND_HORIZ_INV:     return 1.0 - u;
    case BLEND_VERT:          return v;
    case BLEND_VERT_INV:      return 1.0 - v;
    case BLEND_DIAG_R:        return clamp((u + v) * 0.5, 0.0, 1.0);
    case BLEND_DIAG_R_INV:    return clamp(1.0 - (u + v) * 0.5, 0.0, 1.0);
    case BLEND_DIAG_L:        return clamp(((1.0 - u) + v) * 0.5, 0.0, 1.0);
    case BLEND_DIAG_L_INV:    return clamp(1.0 - ((1.0 - u) + v) * 0.5, 0.0, 1.0);
    case BLEND_LONG_DIAG:     return clamp((u + v) * 0.75, 0.0, 1.0);
    case BLEND_LONG_DIAG_INV: return clamp(1.0 - (u + v) * 0.75, 0.0, 1.0);
    case BLEND_LONG_DIAG_ALT: return clamp(((1.0 - u) + v) * 0.75, 0.0, 1.0);
    case BLEND_LONG_DIAG_ALT_INV: return clamp(1.0 - ((1.0 - u) + v) * 0.75, 0.0, 1.0);
    default: return 0.0;
  }
}

// ---------------------------------------------------------------------------
// Simple hash-based 2D noise for procedural cloud shadows.
// ---------------------------------------------------------------------------
float hash21(vec2 p) {
  p = fract(p * vec2(127.1, 311.7));
  p += dot(p, p + 19.19);
  return fract(p.x * p.y);
}

float smoothNoise(vec2 uv) {
  vec2 i = floor(uv);
  vec2 f = fract(uv);
  vec2 u = f * f * (3.0 - 2.0 * f);

  float a = hash21(i);
  float b = hash21(i + vec2(1.0, 0.0));
  float c = hash21(i + vec2(0.0, 1.0));
  float d = hash21(i + vec2(1.0, 1.0));

  return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float cloudPattern(vec2 uv) {
  return smoothNoise(uv) * 0.6 + smoothNoise(uv * 2.1 + 4.7) * 0.4;
}

void main() {
  vec3 normal = normalize(fragNormal);

  vec3 baseColor;

  if (material.useTexture == 1u && material.useBlendData == 1u) {
    // ---------------------------------------------------------------------------
    // Phase 2 – splatmap blending via SSBO + texture array.
    // ---------------------------------------------------------------------------

    // Compute which terrain cell this fragment falls in.
    float cellX = fragWorldPos.x / material.mapXYFactor;
    float cellZ = fragWorldPos.z / material.mapXYFactor;

    uint cX = uint(clamp(cellX, 0.0, float(material.mapWidth  - 2u)));
    uint cZ = uint(clamp(cellZ, 0.0, float(material.mapHeight - 2u)));
    uint cellIndex = cZ * material.mapWidth + cX;

    vec2 cellFrac = vec2(fract(cellX), fract(cellZ));

    uint baseTile  = getCellBaseTile(cellIndex);
    uint cellFlags = getCellFlags(cellIndex);
    vec2 baseUV;
    if ((cellFlags & CELL_FLAG_IS_CLIFF) != 0u) {
      baseUV = fragAtlasCoord;
    } else {
      uint baseQuad = getCellBaseQuadrant(cellIndex);
      baseUV = quadrantUV(cellFrac, baseQuad);
    }
    baseColor = texture(tileTextures, vec3(baseUV, float(baseTile))).rgb;

    uint blendTile = getCellBlendTile(cellIndex);
    uint blendDir  = getCellBlendDir(cellIndex);
    if (blendTile > 0u && blendDir != BLEND_NONE) {
      uint blendQuad  = getCellBlendQuadrant(cellIndex);
      float alpha;
      if (blendDir == BLEND_CUSTOM_EDGE) {
        // Phase 5.5: blendQuad holds the GPU layer index of the edge tile.
        // Sample its alpha channel to drive the blend mask.
        vec2 edgeUV = quadrantUV(cellFrac, 0u);
        alpha = texture(tileTextures, vec3(edgeUV, float(blendQuad))).a;
      } else {
        alpha = blendAlpha(blendDir, cellFrac);
      }
      vec2 blendUV    = quadrantUV(cellFrac, blendDir == BLEND_CUSTOM_EDGE ? 0u : blendQuad);
      vec3 blendColor = texture(tileTextures, vec3(blendUV, float(blendTile))).rgb;
      baseColor       = mix(baseColor, blendColor, alpha);
    }

    uint extraTile = getCellExtraTile(cellIndex);
    uint extraDir  = getCellExtraDir(cellIndex);
    if (extraTile > 0u && extraDir != BLEND_NONE) {
      uint extraQuad  = getCellExtraQuadrant(cellIndex);
      float alpha;
      if (extraDir == BLEND_CUSTOM_EDGE) {
        // Phase 5.5: extraQuad holds the GPU layer index of the edge tile.
        vec2 edgeUV = quadrantUV(cellFrac, 0u);
        alpha = texture(tileTextures, vec3(edgeUV, float(extraQuad))).a;
      } else {
        alpha = blendAlpha(extraDir, cellFrac);
      }
      vec2 extraUV    = quadrantUV(cellFrac, extraDir == BLEND_CUSTOM_EDGE ? 0u : extraQuad);
      vec3 extraColor = texture(tileTextures, vec3(extraUV, float(extraTile))).rgb;
      baseColor       = mix(baseColor, extraColor, alpha);
    }

  } else if (material.useTexture == 1u) {
    // Phase 1.4 – tile array without blend data: use baked atlas UV.
    // fragAtlasCoord carries the base tile quadrant UV from mesh generation.
    // Since we now have a texture array, we sample layer 0 as fallback.
    baseColor = texture(tileTextures, vec3(fragAtlasCoord, 0.0)).rgb;

  } else {
    // Fallback: procedural height-based gradient (green low, brown high).
    float height = fragWorldPos.y;
    float t = clamp(height / 100.0, 0.0, 1.0);
    vec3 lowColor  = vec3(0.35, 0.55, 0.25);
    vec3 highColor = vec3(0.65, 0.55, 0.40);
    baseColor = mix(lowColor, highColor, t);
  }

  vec3 lightDir = normalize(-material.lightDirection);

  vec3 ambient = material.ambientColor.rgb * baseColor;

  float diff = max(dot(normal, lightDir), 0.0);
  vec3 diffuse = material.diffuseColor.rgb * diff * baseColor;

  vec3 result = ambient + diffuse;

  // Phase 6.2 – shadow colour tint.
  if (material.shadowColor.a > 0.0) {
    float shadowFactor = (1.0 - diff) * material.shadowColor.a;
    result = mix(result, result * material.shadowColor.rgb, shadowFactor);
  }

  // Phase 6.3 – cloud shadow overlay.
  if (material.cloudStrength > 0.0) {
    float cloud = cloudPattern(fragCloudCoord);
    float shadow = smoothstep(0.45, 0.65, cloud) * material.cloudStrength;
    result *= (1.0 - shadow * 0.6);
  }

  outColor = vec4(result, 1.0);
}
