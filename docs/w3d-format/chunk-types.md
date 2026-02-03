# Chunk Types

Complete reference of W3D chunk type identifiers.

## Overview

Chunk types are 32-bit unsigned integers. They are organized into ranges by category.

## Mesh Chunks (0x00000000 - 0x000000FF)

### Container Chunks

| ID | Name | Description |
|----|------|-------------|
| 0x00000000 | MESH | Mesh container |
| 0x00000023 | PRELIT_UNLIT | Prelit unlit data |
| 0x00000024 | PRELIT_VERTEX | Prelit vertex data |
| 0x00000025 | PRELIT_LIGHTMAP_MULTI_PASS | Lightmap multi-pass |
| 0x00000026 | PRELIT_LIGHTMAP_MULTI_TEXTURE | Lightmap multi-texture |

### Data Chunks

| ID | Name | Description |
|----|------|-------------|
| 0x00000002 | VERTICES | Vertex positions (Vector3[]) |
| 0x00000003 | VERTEX_NORMALS | Normal vectors (Vector3[]) |
| 0x0000000C | MESH_USER_TEXT | User text string |
| 0x0000000D | TEXCOORDS | UV coordinates (Vector2[]) |
| 0x0000000E | VERTEX_INFLUENCES | Bone weights |
| 0x0000001F | MESH_HEADER3 | Mesh header (version 3+) |
| 0x00000020 | TRIANGLES | Triangle data |
| 0x00000022 | VERTEX_SHADE_INDICES | Shade indices |
| 0x00000115 | VERTEX_COLORS | Per-vertex colors (RGBA[]) |

### Material Chunks

| ID | Name | Description |
|----|------|-------------|
| 0x00000028 | MATERIAL_INFO | Material count info |
| 0x00000029 | SHADERS | Shader definitions |
| 0x0000002A | VERTEX_MATERIALS | Material container |
| 0x0000002B | VERTEX_MATERIAL | Single material |
| 0x0000002C | VERTEX_MATERIAL_NAME | Material name |
| 0x0000002D | VERTEX_MATERIAL_INFO | Material properties |
| 0x0000002E | VERTEX_MAPPER_ARGS0 | Mapper arguments 0 |
| 0x0000002F | VERTEX_MAPPER_ARGS1 | Mapper arguments 1 |
| 0x00000030 | TEXTURES | Texture container |
| 0x00000031 | TEXTURE | Single texture |
| 0x00000032 | TEXTURE_NAME | Texture filename |
| 0x00000033 | TEXTURE_INFO | Texture properties |
| 0x00000038 | MATERIAL_PASS | Material pass container |
| 0x00000039 | VERTEX_MATERIAL_IDS | Per-vertex material IDs |
| 0x0000003A | SHADER_IDS | Per-vertex shader IDs |
| 0x0000003B | DCG | Diffuse color per-vertex |
| 0x0000003C | DIG | Diffuse illumination per-vertex |
| 0x0000003E | SCG | Specular color per-vertex |
| 0x00000048 | TEXTURE_STAGE | Texture stage container |
| 0x00000049 | TEXTURE_IDS | Texture IDs per-vertex |
| 0x0000004A | STAGE_TEXCOORDS | Per-stage UV coordinates |
| 0x0000004B | PER_FACE_TEXCOORD_IDS | Per-face UV indices |

### Deform Chunks

| ID | Name | Description |
|----|------|-------------|
| 0x00000058 | DEFORM | Deform container |
| 0x00000059 | DEFORM_SET | Deform set |
| 0x0000005A | DEFORM_KEYFRAME | Deform keyframe |
| 0x0000005B | DEFORM_DATA | Deform data |

### Collision Chunks

| ID | Name | Description |
|----|------|-------------|
| 0x00000090 | AABTREE | AABB tree container |
| 0x00000091 | AABTREE_HEADER | Tree header |
| 0x00000092 | AABTREE_POLYINDICES | Polygon indices |
| 0x00000093 | AABTREE_NODES | Tree nodes |

## Hierarchy Chunks (0x00000100 - 0x000001FF)

| ID | Name | Description |
|----|------|-------------|
| 0x00000100 | HIERARCHY | Hierarchy container |
| 0x00000101 | HIERARCHY_HEADER | Header (version, name, counts) |
| 0x00000102 | PIVOTS | Bone/pivot data |
| 0x00000103 | PIVOT_FIXUPS | Fixup vectors |

## Animation Chunks (0x00000200 - 0x000002FF)

### Uncompressed Animation

| ID | Name | Description |
|----|------|-------------|
| 0x00000200 | ANIMATION | Animation container |
| 0x00000201 | ANIMATION_HEADER | Header (name, frames, rate) |
| 0x00000202 | ANIMATION_CHANNEL | Animation channel |
| 0x00000203 | BIT_CHANNEL | Visibility channel |

### Compressed Animation

| ID | Name | Description |
|----|------|-------------|
| 0x00000280 | COMPRESSED_ANIMATION | Container |
| 0x00000281 | COMPRESSED_ANIMATION_HEADER | Header |
| 0x00000282 | COMPRESSED_ANIMATION_CHANNEL | Compressed channel |
| 0x00000283 | COMPRESSED_BIT_CHANNEL | Compressed visibility |

### Morph Animation

| ID | Name | Description |
|----|------|-------------|
| 0x000002C0 | MORPH_ANIMATION | Container |
| 0x000002C1 | MORPHANIM_HEADER | Header |
| 0x000002C2 | MORPHANIM_CHANNEL | Morph channel |
| 0x000002C3 | MORPHANIM_POSENAME | Pose name |
| 0x000002C4 | MORPHANIM_KEYDATA | Key data |
| 0x000002C5 | MORPHANIM_PIVOTCHANNELDATA | Pivot channel data |

## HTree Chunks (0x00000300 - 0x000003FF)

Legacy hierarchy format:

| ID | Name | Description |
|----|------|-------------|
| 0x00000300 | HTREE | Container |
| 0x00000301 | HTREE_HEADER | Header |
| 0x00000302 | HTREE_PIVOTS | Pivots |

## Collection Chunks (0x00000400 - 0x000004FF)

| ID | Name | Description |
|----|------|-------------|
| 0x00000400 | TEXTURE_REPLACER_INFO | Texture replacer |
| 0x00000420 | COLLECTION | Collection container |
| 0x00000421 | COLLECTION_HEADER | Header |
| 0x00000422 | COLLECTION_OBJ_NAME | Object name |
| 0x00000423 | PLACEHOLDER | Placeholder |
| 0x00000424 | TRANSFORM_NODE | Transform node |

## HModel Chunks (0x00000500 - 0x000005FF)

Legacy model format:

| ID | Name | Description |
|----|------|-------------|
| 0x00000500 | HMODEL | Container |
| 0x00000501 | HMODEL_HEADER | Header |
| 0x00000502 | HMODEL_AUX_DATA | Auxiliary data |
| 0x00000503 | NODE | Node |

## Aggregate Chunks (0x00000600 - 0x000006FF)

| ID | Name | Description |
|----|------|-------------|
| 0x00000600 | AGGREGATE | Container |
| 0x00000601 | AGGREGATE_HEADER | Header |
| 0x00000602 | AGGREGATE_INFO | Info |
| 0x00000603 | TEXTURE_REPLACER_INFO_V2 | Replacer v2 |
| 0x00000604 | AGGREGATE_CLASS_INFO | Class info |

## HLod Chunks (0x00000700 - 0x000007FF)

| ID | Name | Description |
|----|------|-------------|
| 0x00000700 | HLOD | Container |
| 0x00000701 | HLOD_HEADER | Header (name, counts) |
| 0x00000702 | HLOD_LOD_ARRAY | LOD level container |
| 0x00000703 | HLOD_SUB_OBJECT_ARRAY_HEADER | Sub-object header |
| 0x00000704 | HLOD_SUB_OBJECT | Sub-object definition |
| 0x00000705 | HLOD_AGGREGATE_ARRAY | Aggregate array |
| 0x00000706 | HLOD_PROXY_ARRAY | Proxy array |

## Primitive Chunks (0x00000740 - 0x000007FF)

| ID | Name | Description |
|----|------|-------------|
| 0x00000740 | BOX | Box collision |
| 0x00000750 | SPHERE | Sphere collision |
| 0x00000760 | RING | Ring primitive |

## Emitter Chunks (0x00000800 - 0x000008FF)

| ID | Name | Description |
|----|------|-------------|
| 0x00000800 | EMITTER | Container |
| 0x00000801 | EMITTER_HEADER | Header |
| 0x00000802 | EMITTER_USER_DATA | User data |
| 0x00000803 | EMITTER_INFO | Info |
| 0x00000804 | EMITTER_INFOV2 | Info v2 |
| 0x00000805 | EMITTER_PROPS | Properties |
| 0x00000806 | EMITTER_COLOR_KEYFRAME | Color keys |
| 0x00000807 | EMITTER_OPACITY_KEYFRAME | Opacity keys |
| 0x00000808 | EMITTER_SIZE_KEYFRAME | Size keys |
| 0x00000809 | EMITTER_LINE_PROPERTIES | Line properties |
| 0x0000080A | EMITTER_ROTATION_KEYFRAMES | Rotation keys |
| 0x0000080B | EMITTER_FRAME_KEYFRAMES | Frame keys |
| 0x0000080C | EMITTER_BLUR_TIME_KEYFRAMES | Blur time keys |
| 0x0000080D | SECONDARY_EMITTER | Secondary emitter |

## Light Chunks (0x00000900 - 0x000009FF)

| ID | Name | Description |
|----|------|-------------|
| 0x00000900 | LIGHT | Container |
| 0x00000901 | LIGHT_INFO | Light info |
| 0x00000902 | SPOT_LIGHT_INFO | Spotlight info |
| 0x00000903 | NEAR_ATTENUATION | Near attenuation |
| 0x00000904 | FAR_ATTENUATION | Far attenuation |

## Dazzle Chunks (0x00000A00 - 0x00000AFF)

| ID | Name | Description |
|----|------|-------------|
| 0x00000A00 | DAZZLE | Container |
| 0x00000A01 | DAZZLE_NAME | Name |
| 0x00000A02 | DAZZLE_TYPENAME | Type name |

## Sound Chunks (0x00000B00 - 0x00000BFF)

| ID | Name | Description |
|----|------|-------------|
| 0x00000B00 | SOUNDROBJ | Container |
| 0x00000B01 | SOUNDROBJ_HEADER | Header |
| 0x00000B02 | SOUNDROBJ_DEFINITION | Definition |
