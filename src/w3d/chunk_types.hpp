#pragma once

#include <cstdint>

namespace w3d {

// W3D chunk type identifiers
// Based on legacy/GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2/w3d_file.h

enum class ChunkType : uint32_t {
  // Mesh chunks
  MESH = 0x00000000,
  VERTICES = 0x00000002,
  VERTEX_NORMALS = 0x00000003,
  MESH_USER_TEXT = 0x0000000C,
  VERTEX_INFLUENCES = 0x0000000E,
  MESH_HEADER3 = 0x0000001F,
  TRIANGLES = 0x00000020,
  VERTEX_SHADE_INDICES = 0x00000022,
  PRELIT_UNLIT = 0x00000023,
  PRELIT_VERTEX = 0x00000024,
  PRELIT_LIGHTMAP_MULTI_PASS = 0x00000025,
  PRELIT_LIGHTMAP_MULTI_TEXTURE = 0x00000026,
  MATERIAL_INFO = 0x00000028,
  SHADERS = 0x00000029,
  VERTEX_MATERIALS = 0x0000002A,
  VERTEX_MATERIAL = 0x0000002B,
  VERTEX_MATERIAL_NAME = 0x0000002C,
  VERTEX_MATERIAL_INFO = 0x0000002D,
  VERTEX_MAPPER_ARGS0 = 0x0000002E,
  VERTEX_MAPPER_ARGS1 = 0x0000002F,
  TEXTURES = 0x00000030,
  TEXTURE = 0x00000031,
  TEXTURE_NAME = 0x00000032,
  TEXTURE_INFO = 0x00000033,
  MATERIAL_PASS = 0x00000038,
  VERTEX_MATERIAL_IDS = 0x00000039,
  SHADER_IDS = 0x0000003A,
  DCG = 0x0000003B,
  DIG = 0x0000003C,
  SCG = 0x0000003E,
  TEXTURE_STAGE = 0x00000048,
  TEXTURE_IDS = 0x00000049,
  STAGE_TEXCOORDS = 0x0000004A,
  PER_FACE_TEXCOORD_IDS = 0x0000004B,
  DEFORM = 0x00000058,
  DEFORM_SET = 0x00000059,
  DEFORM_KEYFRAME = 0x0000005A,
  DEFORM_DATA = 0x0000005B,
  PS2_SHADERS = 0x00000080,
  AABTREE = 0x00000090,
  AABTREE_HEADER = 0x00000091,
  AABTREE_POLYINDICES = 0x00000092,
  AABTREE_NODES = 0x00000093,
  TEXCOORDS = 0x0000000D,
  VERTEX_COLORS = 0x00000115,

  // Hierarchy chunks
  HIERARCHY = 0x00000100,
  HIERARCHY_HEADER = 0x00000101,
  PIVOTS = 0x00000102,
  PIVOT_FIXUPS = 0x00000103,

  // Animation chunks
  ANIMATION = 0x00000200,
  ANIMATION_HEADER = 0x00000201,
  ANIMATION_CHANNEL = 0x00000202,
  BIT_CHANNEL = 0x00000203,
  COMPRESSED_ANIMATION = 0x00000280,
  COMPRESSED_ANIMATION_HEADER = 0x00000281,
  COMPRESSED_ANIMATION_CHANNEL = 0x00000282,
  COMPRESSED_BIT_CHANNEL = 0x00000283,
  MORPH_ANIMATION = 0x000002C0,
  MORPHANIM_HEADER = 0x000002C1,
  MORPHANIM_CHANNEL = 0x000002C2,
  MORPHANIM_POSENAME = 0x000002C3,
  MORPHANIM_KEYDATA = 0x000002C4,
  MORPHANIM_PIVOTCHANNELDATA = 0x000002C5,

  // HTree chunks (old)
  HTREE = 0x00000300,
  HTREE_HEADER = 0x00000301,
  HTREE_PIVOTS = 0x00000302,

  // Texture replacer chunks
  TEXTURE_REPLACER_INFO = 0x00000400,

  // Aggregate chunks
  AGGREGATE = 0x00000600,
  AGGREGATE_HEADER = 0x00000601,
  AGGREGATE_INFO = 0x00000602,
  TEXTURE_REPLACER_INFO_V2 = 0x00000603,
  AGGREGATE_CLASS_INFO = 0x00000604,

  // HLod chunks
  HLOD = 0x00000700,
  HLOD_HEADER = 0x00000701,
  HLOD_LOD_ARRAY = 0x00000702,
  HLOD_SUB_OBJECT_ARRAY_HEADER = 0x00000703,
  HLOD_SUB_OBJECT = 0x00000704,
  HLOD_AGGREGATE_ARRAY = 0x00000705,
  HLOD_PROXY_ARRAY = 0x00000706,

  // Box chunks
  BOX = 0x00000740,

  // Sphere chunks (placeholder)
  SPHERE = 0x00000750,

  // Ring chunks (placeholder)
  RING = 0x00000760,

  // HModel chunks (legacy)
  HMODEL = 0x00000500,
  HMODEL_HEADER = 0x00000501,
  HMODEL_AUX_DATA = 0x00000502,
  NODE = 0x00000503,

  // Collection chunks
  COLLECTION = 0x00000420,
  COLLECTION_HEADER = 0x00000421,
  COLLECTION_OBJ_NAME = 0x00000422,
  PLACEHOLDER = 0x00000423,
  TRANSFORM_NODE = 0x00000424,

  // Null object
  NULL_OBJECT = 0x00000750,

  // Emitter chunks
  EMITTER = 0x00000800,
  EMITTER_HEADER = 0x00000801,
  EMITTER_USER_DATA = 0x00000802,
  EMITTER_INFO = 0x00000803,
  EMITTER_INFOV2 = 0x00000804,
  EMITTER_PROPS = 0x00000805,
  EMITTER_COLOR_KEYFRAME = 0x00000806,
  EMITTER_OPACITY_KEYFRAME = 0x00000807,
  EMITTER_SIZE_KEYFRAME = 0x00000808,
  EMITTER_LINE_PROPERTIES = 0x00000809,
  EMITTER_ROTATION_KEYFRAMES = 0x0000080A,
  EMITTER_FRAME_KEYFRAMES = 0x0000080B,
  EMITTER_BLUR_TIME_KEYFRAMES = 0x0000080C,
  SECONDARY_EMITTER = 0x0000080D,

  // Light chunks
  LIGHT = 0x00000900,
  LIGHT_INFO = 0x00000901,
  SPOT_LIGHT_INFO = 0x00000902,
  NEAR_ATTENUATION = 0x00000903,
  FAR_ATTENUATION = 0x00000904,

  // Dazzle chunks
  DAZZLE = 0x00000A00,
  DAZZLE_NAME = 0x00000A01,
  DAZZLE_TYPENAME = 0x00000A02,

  // Sound object chunks
  SOUNDROBJ = 0x00000B00,
  SOUNDROBJ_HEADER = 0x00000B01,
  SOUNDROBJ_DEFINITION = 0x00000B02,
};

// Mesh attribute flags
namespace MeshFlags {
constexpr uint32_t COLLISION_TYPE_MASK = 0x00000FF0;
constexpr uint32_t COLLISION_TYPE_SHIFT = 4;
constexpr uint32_t COLLISION_TYPE_PHYSICAL = 0x00000010;
constexpr uint32_t COLLISION_TYPE_PROJECTILE = 0x00000020;
constexpr uint32_t COLLISION_TYPE_VIS = 0x00000040;
constexpr uint32_t COLLISION_TYPE_CAMERA = 0x00000080;
constexpr uint32_t COLLISION_TYPE_VEHICLE = 0x00000100;

constexpr uint32_t HIDDEN = 0x00001000;
constexpr uint32_t TWO_SIDED = 0x00002000;
constexpr uint32_t OBSOLETE_LIGHT_MAPPED = 0x00004000;
constexpr uint32_t CAST_SHADOW = 0x00008000;

constexpr uint32_t GEOMETRY_TYPE_MASK = 0x00FF0000;
constexpr uint32_t GEOMETRY_TYPE_NORMAL = 0x00000000;
constexpr uint32_t GEOMETRY_TYPE_CAMERA_ALIGNED = 0x00010000;
constexpr uint32_t GEOMETRY_TYPE_SKIN = 0x00020000;
constexpr uint32_t GEOMETRY_TYPE_CAMERA_ORIENTED = 0x00060000;

constexpr uint32_t PRELIT_MASK = 0x0F000000;
constexpr uint32_t PRELIT_UNLIT = 0x01000000;
constexpr uint32_t PRELIT_VERTEX = 0x02000000;
constexpr uint32_t PRELIT_LIGHTMAP_MULTI_PASS = 0x04000000;
constexpr uint32_t PRELIT_LIGHTMAP_MULTI_TEXTURE = 0x08000000;

constexpr uint32_t SORT_LEVEL_MASK = 0xF0000000;
constexpr uint32_t SORT_LEVEL_NONE = 0x00000000;
constexpr uint32_t SORT_LEVEL_BIN1 = 0x10000000;
constexpr uint32_t SORT_LEVEL_BIN2 = 0x20000000;
constexpr uint32_t SORT_LEVEL_BIN3 = 0x30000000;
} // namespace MeshFlags

// Vertex channel flags
namespace VertexChannels {
constexpr uint32_t LOCATION = 0x01;
constexpr uint32_t NORMAL = 0x02;
constexpr uint32_t TEXCOORD = 0x04;
constexpr uint32_t COLOR = 0x08;
constexpr uint32_t BONEID = 0x10;
} // namespace VertexChannels

// Face channel flags
namespace FaceChannels {
constexpr uint32_t FACE = 0x01;
} // namespace FaceChannels

// Shader constants
namespace Shader {
// Depth compare functions
constexpr uint8_t DEPTHCOMPARE_PASS_NEVER = 0;
constexpr uint8_t DEPTHCOMPARE_PASS_LESS = 1;
constexpr uint8_t DEPTHCOMPARE_PASS_EQUAL = 2;
constexpr uint8_t DEPTHCOMPARE_PASS_LEQUAL = 3;
constexpr uint8_t DEPTHCOMPARE_PASS_GREATER = 4;
constexpr uint8_t DEPTHCOMPARE_PASS_NOTEQUAL = 5;
constexpr uint8_t DEPTHCOMPARE_PASS_GEQUAL = 6;
constexpr uint8_t DEPTHCOMPARE_PASS_ALWAYS = 7;

// Depth mask modes
constexpr uint8_t DEPTHMASK_WRITE_DISABLE = 0;
constexpr uint8_t DEPTHMASK_WRITE_ENABLE = 1;

// Destination blend functions
constexpr uint8_t DESTBLENDFUNC_ZERO = 0;
constexpr uint8_t DESTBLENDFUNC_ONE = 1;
constexpr uint8_t DESTBLENDFUNC_SRC_COLOR = 2;
constexpr uint8_t DESTBLENDFUNC_ONE_MINUS_SRC_COLOR = 3;
constexpr uint8_t DESTBLENDFUNC_SRC_ALPHA = 4;
constexpr uint8_t DESTBLENDFUNC_ONE_MINUS_SRC_ALPHA = 5;

// Source blend functions
constexpr uint8_t SRCBLENDFUNC_ZERO = 0;
constexpr uint8_t SRCBLENDFUNC_ONE = 1;
constexpr uint8_t SRCBLENDFUNC_SRC_ALPHA = 2;
constexpr uint8_t SRCBLENDFUNC_ONE_MINUS_SRC_ALPHA = 3;

// Gradient modes
constexpr uint8_t PRIGRADIENT_DISABLE = 0;
constexpr uint8_t PRIGRADIENT_MODULATE = 1;
constexpr uint8_t PRIGRADIENT_ADD = 2;
constexpr uint8_t PRIGRADIENT_BUMPENVMAP = 3;

constexpr uint8_t SECGRADIENT_DISABLE = 0;
constexpr uint8_t SECGRADIENT_ENABLE = 1;

// Texturing modes
constexpr uint8_t TEXTURING_DISABLE = 0;
constexpr uint8_t TEXTURING_ENABLE = 1;

// Alpha test
constexpr uint8_t ALPHATEST_DISABLE = 0;
constexpr uint8_t ALPHATEST_ENABLE = 1;

// Detail color function
constexpr uint8_t DETAILCOLORFUNC_DISABLE = 0;
constexpr uint8_t DETAILCOLORFUNC_DETAIL = 1;
constexpr uint8_t DETAILCOLORFUNC_SCALE = 2;
constexpr uint8_t DETAILCOLORFUNC_INVSCALE = 3;
constexpr uint8_t DETAILCOLORFUNC_ADD = 4;
constexpr uint8_t DETAILCOLORFUNC_SUB = 5;
constexpr uint8_t DETAILCOLORFUNC_SUBR = 6;
constexpr uint8_t DETAILCOLORFUNC_BLEND = 7;
constexpr uint8_t DETAILCOLORFUNC_DETAILBLEND = 8;

// Detail alpha function
constexpr uint8_t DETAILALPHAFUNC_DISABLE = 0;
constexpr uint8_t DETAILALPHAFUNC_DETAIL = 1;
constexpr uint8_t DETAILALPHAFUNC_SCALE = 2;
constexpr uint8_t DETAILALPHAFUNC_INVSCALE = 3;
} // namespace Shader

// Animation channel types
namespace AnimChannelType {
constexpr uint16_t X = 0;
constexpr uint16_t Y = 1;
constexpr uint16_t Z = 2;
constexpr uint16_t XR = 3;
constexpr uint16_t YR = 4;
constexpr uint16_t ZR = 5;
constexpr uint16_t Q = 6;
constexpr uint16_t TIMECODED_X = 0;
constexpr uint16_t TIMECODED_Y = 1;
constexpr uint16_t TIMECODED_Z = 2;
constexpr uint16_t TIMECODED_Q = 3;
constexpr uint16_t ADAPTIVEDELTA_X = 4;
constexpr uint16_t ADAPTIVEDELTA_Y = 5;
constexpr uint16_t ADAPTIVEDELTA_Z = 6;
constexpr uint16_t ADAPTIVEDELTA_Q = 7;
} // namespace AnimChannelType

// Constants
constexpr uint32_t W3D_NAME_LEN = 16;
constexpr uint32_t W3D_CURRENT_MESH_VERSION = 0x00040002; // 4.2

// Helper to create version number
constexpr uint32_t MakeVersion(uint16_t major, uint16_t minor) {
  return (static_cast<uint32_t>(major) << 16) | minor;
}

constexpr uint16_t VersionMajor(uint32_t version) {
  return static_cast<uint16_t>(version >> 16);
}

constexpr uint16_t VersionMinor(uint32_t version) {
  return static_cast<uint16_t>(version & 0xFFFF);
}

// Helper to get chunk type name for debugging
inline const char *ChunkTypeName(ChunkType type) {
  switch (type) {
  case ChunkType::MESH:
    return "MESH";
  case ChunkType::VERTICES:
    return "VERTICES";
  case ChunkType::VERTEX_NORMALS:
    return "VERTEX_NORMALS";
  case ChunkType::MESH_USER_TEXT:
    return "MESH_USER_TEXT";
  case ChunkType::VERTEX_INFLUENCES:
    return "VERTEX_INFLUENCES";
  case ChunkType::MESH_HEADER3:
    return "MESH_HEADER3";
  case ChunkType::TRIANGLES:
    return "TRIANGLES";
  case ChunkType::VERTEX_SHADE_INDICES:
    return "VERTEX_SHADE_INDICES";
  case ChunkType::PRELIT_UNLIT:
    return "PRELIT_UNLIT";
  case ChunkType::PRELIT_VERTEX:
    return "PRELIT_VERTEX";
  case ChunkType::PRELIT_LIGHTMAP_MULTI_PASS:
    return "PRELIT_LIGHTMAP_MULTI_PASS";
  case ChunkType::PRELIT_LIGHTMAP_MULTI_TEXTURE:
    return "PRELIT_LIGHTMAP_MULTI_TEXTURE";
  case ChunkType::MATERIAL_INFO:
    return "MATERIAL_INFO";
  case ChunkType::SHADERS:
    return "SHADERS";
  case ChunkType::VERTEX_MATERIALS:
    return "VERTEX_MATERIALS";
  case ChunkType::VERTEX_MATERIAL:
    return "VERTEX_MATERIAL";
  case ChunkType::VERTEX_MATERIAL_NAME:
    return "VERTEX_MATERIAL_NAME";
  case ChunkType::VERTEX_MATERIAL_INFO:
    return "VERTEX_MATERIAL_INFO";
  case ChunkType::VERTEX_MAPPER_ARGS0:
    return "VERTEX_MAPPER_ARGS0";
  case ChunkType::VERTEX_MAPPER_ARGS1:
    return "VERTEX_MAPPER_ARGS1";
  case ChunkType::TEXTURES:
    return "TEXTURES";
  case ChunkType::TEXTURE:
    return "TEXTURE";
  case ChunkType::TEXTURE_NAME:
    return "TEXTURE_NAME";
  case ChunkType::TEXTURE_INFO:
    return "TEXTURE_INFO";
  case ChunkType::MATERIAL_PASS:
    return "MATERIAL_PASS";
  case ChunkType::VERTEX_MATERIAL_IDS:
    return "VERTEX_MATERIAL_IDS";
  case ChunkType::SHADER_IDS:
    return "SHADER_IDS";
  case ChunkType::DCG:
    return "DCG";
  case ChunkType::DIG:
    return "DIG";
  case ChunkType::SCG:
    return "SCG";
  case ChunkType::TEXTURE_STAGE:
    return "TEXTURE_STAGE";
  case ChunkType::TEXTURE_IDS:
    return "TEXTURE_IDS";
  case ChunkType::STAGE_TEXCOORDS:
    return "STAGE_TEXCOORDS";
  case ChunkType::PER_FACE_TEXCOORD_IDS:
    return "PER_FACE_TEXCOORD_IDS";
  case ChunkType::AABTREE:
    return "AABTREE";
  case ChunkType::AABTREE_HEADER:
    return "AABTREE_HEADER";
  case ChunkType::AABTREE_POLYINDICES:
    return "AABTREE_POLYINDICES";
  case ChunkType::AABTREE_NODES:
    return "AABTREE_NODES";
  case ChunkType::TEXCOORDS:
    return "TEXCOORDS";
  case ChunkType::VERTEX_COLORS:
    return "VERTEX_COLORS";
  case ChunkType::HIERARCHY:
    return "HIERARCHY";
  case ChunkType::HIERARCHY_HEADER:
    return "HIERARCHY_HEADER";
  case ChunkType::PIVOTS:
    return "PIVOTS";
  case ChunkType::PIVOT_FIXUPS:
    return "PIVOT_FIXUPS";
  case ChunkType::ANIMATION:
    return "ANIMATION";
  case ChunkType::ANIMATION_HEADER:
    return "ANIMATION_HEADER";
  case ChunkType::ANIMATION_CHANNEL:
    return "ANIMATION_CHANNEL";
  case ChunkType::BIT_CHANNEL:
    return "BIT_CHANNEL";
  case ChunkType::COMPRESSED_ANIMATION:
    return "COMPRESSED_ANIMATION";
  case ChunkType::COMPRESSED_ANIMATION_HEADER:
    return "COMPRESSED_ANIMATION_HEADER";
  case ChunkType::COMPRESSED_ANIMATION_CHANNEL:
    return "COMPRESSED_ANIMATION_CHANNEL";
  case ChunkType::COMPRESSED_BIT_CHANNEL:
    return "COMPRESSED_BIT_CHANNEL";
  case ChunkType::HLOD:
    return "HLOD";
  case ChunkType::HLOD_HEADER:
    return "HLOD_HEADER";
  case ChunkType::HLOD_LOD_ARRAY:
    return "HLOD_LOD_ARRAY";
  case ChunkType::HLOD_SUB_OBJECT_ARRAY_HEADER:
    return "HLOD_SUB_OBJECT_ARRAY_HEADER";
  case ChunkType::HLOD_SUB_OBJECT:
    return "HLOD_SUB_OBJECT";
  case ChunkType::HLOD_AGGREGATE_ARRAY:
    return "HLOD_AGGREGATE_ARRAY";
  case ChunkType::HLOD_PROXY_ARRAY:
    return "HLOD_PROXY_ARRAY";
  case ChunkType::BOX:
    return "BOX";
  default:
    return "UNKNOWN";
  }
}

} // namespace w3d
