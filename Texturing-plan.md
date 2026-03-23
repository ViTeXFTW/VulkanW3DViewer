# Terrain rendering to WorldBuilder replacement

**Session ID:** ses_359e50ecdffefl3gB4bgxYaAXo
**Created:** 2/28/2026, 10:15:09 PM
**Updated:** 2/28/2026, 10:37:33 PM

---

## User

Now that all the phases in the terrain rendering plan I want to expand the functionality more towards a WorldBuilder replacement. I think the next step would be to be able to draw the textures onto the rendered terrain. Help me create a detailed plan for the next steps in this process. If you have any clarifying questions ask them before proposing the plan

---

## Assistant (Plan · claude-opus-4-6 · 208.5s)



I'd like to understand the current state of the project and ask some clarifying questions before proposing a plan. Let me first explore what's already implemented.

**Tool: task**

**Input:**
```json
{
  "description": "Explore terrain rendering state",
  "subagent_type": "explore",
  "prompt": "Very thorough exploration of the VulkanW3DViewer project to understand the current state of terrain rendering and texture handling. I need to know:\n\n1. What terrain rendering code exists? Look in src/render/terrain/, src/lib/formats/map/, and related directories.\n2. What shaders exist for terrain? Check shaders/ directory.\n3. How are textures currently loaded and managed? Look for texture atlas, terrain textures, blend tiles.\n4. What map parsing is implemented? Check data_chunk_reader, map_loader, heightmap_parser, blend_tile_parser, etc.\n5. What is the current state of the UI? Check src/ui/ and src/core/ for any editor-like functionality.\n6. Is there any existing texture painting or brush system?\n7. How does the current rendering pipeline work? Check VulkanContext, renderer, renderables.\n8. What INI parsing exists for terrain types?\n9. Check CMakeLists.txt for library structure and dependencies.\n10. Look at any TODO comments or planned features related to texture painting.\n\nReturn a comprehensive summary of what exists, what's working, and what's missing for terrain texture painting."
}
```

**Output:**
```
task_id: ses_359e4e7d1ffeOMrjMBKQqM4UYb (for resuming to continue this task if needed)

<task_result>
Now I have a comprehensive picture. Let me compile the final report.

---

# Comprehensive Terrain Rendering & Texture Handling Analysis

## 1. TERRAIN RENDERING CODE (src/render/terrain/)

**Status: Fully implemented for geometry and basic texturing. Five complete modules exist.**

### terrain_mesh.hpp/cpp
- **Complete.** Generates terrain geometry from heightmap data.
- Supports two code paths:
  - `generateTerrainMesh()` -- simple heightmap-only mesh (uses heightmap UV coords)
  - `generateTerrainMeshFromBlendData()` -- blend-data-aware mesh (uses per-cell tile UVs from atlas, cliff UV overrides)
- Terrain is subdivided into 32x32-cell chunks (`CHUNK_SIZE = 32`) for frustum culling.
- `TerrainVertex` layout: `position (vec3)`, `normal (vec3)`, `texCoord (vec2)`, `atlasCoord (vec2)` = 40 bytes.
- Implements diagonal flipping for better slope appearance (`shouldFlipDiagonal`).
- Normal computation uses central-difference method from heightmap neighbors.

### terrain_atlas.hpp/cpp
- **Partially complete.** UV computation is done, but real texture loading is missing.
- `computeTileUVTable()` -- computes UV coordinates for tiles in a 2048-wide atlas, given texture class metadata from BlendTileData.
- `decodeTileIndex()` / `decodeQuadrant()` / `decodeTileNdxUV()` -- decodes the packed `int16_t tileNdx` format (top 14 bits = tile index, bottom 2 bits = 32x32 quadrant within a 64x64 tile).
- `buildProceduralAtlas()` -- builds a **procedural placeholder atlas** with hash-derived solid colors (no real terrain textures). This is the only atlas currently in use.
- **MISSING**: Loading actual TGA terrain textures from `TerrainZH.big` archives (from `Art/Terrain/` directory). The tile UV math is ready, but no code extracts and arranges real tile bitmaps into an atlas image.

### terrain_blend.hpp/cpp
- **Complete for CPU-side logic.** All 12 blend directions are implemented.
- Generates alpha gradient patterns (64x64 each) for Horizontal, Vertical, DiagonalRight, DiagonalLeft, LongDiagonal directions + their inverses.
- `blendDirectionFromInfo()` decodes a `BlendTileInfo` struct to determine the blend direction.
- `cellHasBlend()` checks if a cell needs blending.
- **MISSING**: The blend patterns are generated but **never used in actual rendering**. There is no multi-pass or alpha-blended terrain rendering. The current pipeline is single-pass only.

### terrain_renderable.hpp/cpp
- **Complete for rendering path.** Manages GPU-side terrain.
- Two load paths:
  - `load()` -- height-gradient-only rendering (no textures)
  - `loadWithBlendData()` -- uses atlas UV coordinates
- Implements per-chunk frustum culling with `updateFrustum()`.
- Has `initPipeline()` (no atlas) and `initPipelineWithAtlas()` (uploads procedural atlas texture).
- Push constants include: ambient/diffuse/light direction, shadow color, cloud shadow animation params, `useTexture` flag.
- **CRITICAL NOTE**: In `application.cpp`, the current `loadMapFile()` calls `terrainRenderable_.load()` (the height-gradient path), **not** `loadWithBlendData()`. The atlas path is defined but **not wired up** in the actual map loading flow.

### terrain_minimap.hpp/cpp
- **Complete.** Generates a CPU-side RGBA8 minimap image from heightmap data.
- Height-based gradient coloring (dark green to light tan).
- Supports full-resolution and bilinearly downscaled output.

---

## 2. TERRAIN SHADERS (shaders/)

**Status: Working, but single-pass only.**

### terrain.vert
- Inputs: position (vec3), normal (vec3), texCoord (vec2), atlasCoord (vec2).
- Transforms position through MVP matrices from UBO.
- Generates cloud shadow UVs from world-space XZ, animated by push constant `cloudScrollU/V * cloudTime`.
- `kCloudUVScale = 0.002` for cloud UV tiling.

### terrain.frag
- Samples `texSampler` at `fragAtlasCoord` when `useTexture == 1`, otherwise uses a procedural height-based gradient (green-to-tan).
- Directional lighting: `ambient + diffuse * dot(normal, lightDir)`.
- Phase 6.2: Shadow color tint (lerps based on shadow alpha when facing away from light).
- Phase 6.3: Procedural cloud shadows using two-octave FBM noise, scrolled over world-space UVs.
- **MISSING**:
  - No alpha blend pass for terrain transitions.
  - No multi-texture splatmap support.
  - No blend pattern application in the shader.
  - No extra/3-way blend pass.
  - No cliff texture special handling in the shader (handled in mesh generation).

### water.vert / water.frag
- Complete dual-layer scrolling UV water shader with configurable color/opacity via push constants.

---

## 3. TEXTURE LOADING & MANAGEMENT

**Status: Infrastructure is solid; terrain-specific texture loading is missing.**

### TextureManager (src/lib/gfx/texture.hpp/cpp)
- Supports TGA and DDS texture loading from disk or BIG archive extraction.
- VMA-based GPU image creation with optional mipmaps.
- `createTexture()` -- creates from raw RGBA8 pixel data (used by the procedural atlas).
- `createTextureArray()` -- creates a multi-layer texture array (available but not used for terrain).
- `createTextureWithFormat()` -- format-aware creation.
- Has `setAssetRegistry()` and `setBigArchiveManager()` for BIG archive texture extraction.
- **Can** load terrain TGAs from BIG archives; the path resolution infrastructure exists. But no caller invokes it for terrain tile TGAs.

### Current Terrain Atlas Flow
1. `BlendTileParser` parses texture class names (e.g., "TEDesert1") and tile counts.
2. `computeTileUVTable()` calculates UV positions for each tile in a 2048-wide atlas.
3. `buildProceduralAtlas()` creates a solid-color placeholder for each tile.
4. `terrainRenderable_.initPipelineWithAtlas()` uploads the atlas and binds it to the pipeline.
5. **But step 4 is never called from `loadMapFile()`** -- only `initPipeline()` (no atlas) is called.

### What's Missing for Real Terrain Textures
1. **TGA extraction**: Extract terrain TGA files (e.g., `Art/Terrain/TEDesert1.tga`) from `TerrainZH.big`.
2. **INI lookup**: Use `TerrainTypeCollection` to resolve texture class names to TGA filenames.
3. **Tile splitting**: Each TGA is split into 64x64-pixel tiles (a 128x128 TGA yields 4 tiles).
4. **Atlas assembly**: Pack all tiles into a 2048-wide RGBA atlas image.
5. **Wire up the atlas path**: Call `loadWithBlendData()` and `initPipelineWithAtlas()` in `loadMapFile()`.

---

## 4. MAP PARSING

**Status: Fully implemented. All major chunks are parsed.**

### DataChunkReader (data_chunk_reader.hpp/cpp)
- Complete binary reader for the DataChunk format.
- Magic: `CkMp` (0x704D6B43).
- Reads TOC, opens/closes chunks, supports nested chunks.
- Primitives: `readByte()`, `readInt()`, `readReal()`, `readAsciiString()`, `readUnicodeString()`, `readDict()`, `readBytes()`.

### MapLoader (map_loader.hpp/cpp)
- Loads from file path or memory.
- Supports RefPack decompression for compressed maps.
- Dispatches to all chunk parsers in order:
  - HeightMapParser (version 1-4)
  - BlendTileParser (version 1-8, ZH)
  - ObjectsParser
  - TriggersParser (PolygonTriggers)
  - LightingParser (GlobalLighting)
  - WorldInfoParser
  - SidesListParser

### BlendTileParser (blend_tile_parser.hpp/cpp)
- Complete parser supporting all 8 BlendTileData versions.
- Parses: tile index arrays, blend tile arrays, extra blend arrays, cliff info arrays, cliff state bitfield, texture classes, edge texture classes, blend tile infos with FLAG_VAL validation, cliff infos with full UV quad.

### types.hpp
- Comprehensive data structures: `HeightMap`, `BlendTileData`, `TextureClass`, `BlendTileInfo`, `CliffInfo`, `MapObject`, `PolygonTrigger`, `GlobalLighting`, `TimeOfDayLighting`, `WorldInfo`, `SidesList`, `MapFile`.
- Constants: `MAP_XY_FACTOR=10.0`, `MAP_HEIGHT_SCALE=0.625`, `FLAG_VAL=0x7ADA0000`, `TILE_PIXEL_EXTENT=64`, `INVERTED_MASK=0x1`, `FLIPPED_MASK=0x2`.

### Tests
- `tests/map/test_map_loader.cpp`
- `tests/map/test_blend_tile_parser.cpp`
- `tests/terrain/test_terrain_mesh.cpp`
- `tests/terrain/test_terrain_atlas.cpp`
- `tests/terrain/test_terrain_blend.cpp`
- `tests/terrain/test_terrain_minimap.cpp`
- `tests/ini/test_terrain_types.cpp`

---

## 5. UI STATE

**Status: Map viewing UI is implemented; no editing UI exists.**

### Map Viewer UI Components
- **MapViewportWindow** (src/ui/map_viewport_window.hpp/cpp) -- Main map viewer window with panel sidebar.
- **MapInfoPanel** -- Shows heightmap dimensions, world size, texture class list, blend/cliff counts, object/trigger counts, terrain chunk stats, atlas status.
- **LayerTogglesPanel** -- Checkboxes for terrain/water/objects/triggers visibility.
- **TimeOfDayPanel** -- Dropdown to switch time-of-day (Morning/Afternoon/Evening/Night), shows lighting values.
- **ObjectListPanel** -- Lists map objects.
- **MapBrowser** -- Searchable list of maps from BIG archives.
- **UIContext** -- Has `loadedMap`, `terrainRenderable`, `waterRenderable`, `rtsCamera`, `lightingState` pointers.

### What UI Does NOT Have
- No terrain texture palette/swatch selector.
- No brush tool or brush options panel.
- No terrain painting mode toggle.
- No undo/redo system.
- No texture preview thumbnails.
- No height editing tools.
- No tile/blend editing tools.

---

## 6. TEXTURE PAINTING / BRUSH SYSTEM

**Status: Does not exist at all in the viewer.**

There is zero code for texture painting or brush systems in `src/`. The only brush/paint references found are in the original SAGE engine's WorldBuilder source code under `lib/GeneralsGameCode/` (reference only, not compiled):
- `MoundTool` (height brush with width/feather)
- `BrushOptions` dialog (width/feather/height sliders)
- `Tool::calcRoundBlendFactor()` / `calcSquareBlendFactor()` (brush falloff math)
- `TerrainSwatches` / `TerrainMaterial` panels
- Texture painting tracked in `wbview3d.cpp`

The only TODO comments in the project's own source are:
- `src/ui/ui_manager.cpp` line 137: `// TODO: Could trigger an about dialog window`
- `src/lib/formats/big/asset_registry.cpp` line 18: `// TODO: Add handling for user specified archives.`

---

## 7. RENDERING PIPELINE

**Status: Well-structured, fully operational for both model and map viewing modes.**

### Pipeline Architecture
- **VulkanContext** manages instance, device, swapchain (with depth), render pass, framebuffers, VMA allocator.
- **Pipeline** class creates graphics pipelines from `PipelineCreateInfo` presets:
  - `PipelineCreateInfo::standard()` -- basic model rendering
  - `PipelineCreateInfo::skinned()` -- GPU skinned rendering with bone SSBO
  - `PipelineCreateInfo::terrain()` -- terrain with 40-byte vertex, `TerrainPushConstant`
  - `PipelineCreateInfo::water()` -- alpha-blended water with `WaterPushConstant`, depth writes off, two-sided
- **Renderer** owns standard + skinned pipelines, records command buffers per frame.
- **Double-buffered** with MAX_FRAMES_IN_FLIGHT = 2.
- **DescriptorManager** manages per-frame UBO + per-texture descriptor sets.

### Map Rendering Flow (per frame)
1. `lightingState_.update(deltaTime)` -- advance cloud animation.
2. `terrainRenderable_.applyLightingState(lightingState_)` -- update push constant.
3. `terrainRenderable_.updateFrustum(viewProjection)` -- set frustum for culling.
4. In `recordCommandBuffer()`:
   - If MapViewer mode and `showTerrain`: bind terrain pipeline + descriptors, push `TerrainPushConstant`, draw visible chunks.
   - If MapViewer mode and `showWater`: bind water pipeline + descriptors, push `WaterPushConstant`, draw water polygons.
5. Water has separate `update(deltaSeconds)` for UV animation time.

---

## 8. INI PARSING FOR TERRAIN TYPES

**Status: Fully implemented.**

### IniParser (src/lib/formats/ini/ini_parser.hpp/cpp)
- Generic SAGE INI dialect parser.
- Block-based: `registerBlock("Terrain", handler)`.
- Supports `parseAsciiString()`, `parseBool()`, `parseInt()`, `parseReal()`, `parseRGBColor()`, `parseRGBAColorInt()`, `parseIndexList()`.

### TerrainTypeCollection (src/lib/formats/ini/terrain_types.hpp/cpp)
- Parses `TerrainType` definitions from INI text.
- Each `TerrainType` has: `name`, `texture` (TGA filename), `blendEdgeTexture`, `terrainClass` (37 classes defined), `restrictConstruction`.
- `findByName()` lookup.
- `loadFromINI()` processes INI text.
- **Test coverage** in `tests/ini/test_terrain_types.cpp`.

### WaterSettings (src/lib/formats/ini/water_settings.hpp/cpp)
- Parses Water.ini for per-time-of-day water settings.
- Settings: sky/water textures, scroll rates, diffuse colors, transparency depth, standing water texture.

### What's Missing
- No code calls `TerrainTypeCollection::loadFromINI()` during map loading.
- The INI text itself needs to be extracted from `INIZH.big` (the archive is opened but no code reads Terrain.ini from it).

---

## 9. CMAKE / LIBRARY STRUCTURE

**Status: Clean two-library structure.**

- **`w3d_lib`** (static library): All `src/lib/**/*.cpp` and `src/render/**/*.cpp` files. Includes terrain, water, map parsing, INI parsing, gfx abstractions. Links: Vulkan, GLFW, GLM, BigXtractor, VMA.
- **`VulkanW3DViewer`** (executable): `src/core/**/*.cpp`, `src/ui/**/*.cpp`, `src/main.cpp`, plus ImGui sources. Links to `w3d_lib`.
- Shaders compiled from GLSL to SPIR-V and embedded into `embedded_shaders.hpp` at build time.
- Test targets are isolated with Vulkan-free stubs.

---

## 10. SUMMARY: WHAT EXISTS, WHAT WORKS, WHAT'S MISSING

### What Exists and Works
| Component | Status |
|---|---|
| Map file parsing (all chunks) | **Complete** |
| HeightMap, BlendTileData, GlobalLighting parsing | **Complete** |
| DataChunk reader with RefPack decompression | **Complete** |
| Terrain mesh generation (chunked, with diagonal flipping) | **Complete** |
| Terrain atlas UV computation (tile index decoding) | **Complete** |
| Terrain blend pattern generation (12 directions) | **Complete** |
| Terrain rendering with height-gradient fallback | **Complete** |
| Terrain push constants (lighting + cloud shadows) | **Complete** |
| Terrain frustum culling per chunk | **Complete** |
| Water mesh + rendering (scrolling dual-layer UV) | **Complete** |
| INI parser + TerrainType definitions | **Complete** |
| Terrain minimap generation | **Complete** |
| Map viewer UI (info, layer toggles, time-of-day) | **Complete** |
| RTS camera (WASD pan, scroll zoom) | **Complete** |
| Procedural atlas (solid colors per tile) | **Complete** |
| TextureManager with TGA/DDS/BIG archive support | **Complete** |

### What's Partially Done
| Component | Status |
|---|---|
| Terrain atlas with real textures | UV math done, but no TGA loading/splitting/assembly |
| Blend-data-aware mesh loading | Code exists (`loadWithBlendData`) but not wired into `loadMapFile()` |
| Terrain type INI resolution | Parser exists but never called during map load |

### What's Completely Missing for Terrain Texture Painting
| Component | Gap |
|---|---|
| **Real terrain texture atlas** | Need: extract TGAs from TerrainZH.big, split into 64x64 tiles, assemble into GPU atlas |
| **Multi-pass terrain rendering** | Need: blend pass (alpha overlay), extra blend pass (3-way), custom edge pass |
| **Blend shader support** | Need: fragment shader that samples two textures + blend alpha pattern |
| **Terrain editing data model** | Need: mutable HeightMap/BlendTileData with change tracking |
| **Brush system** | Need: brush shape (round/square), width, feather, falloff calculation |
| **Texture palette UI** | Need: terrain type swatch selector, foreground/background texture, preview thumbnails |
| **Brush tool UI** | Need: brush options panel (width, feather, strength), brush cursor overlay in viewport |
| **Painting logic** | Need: mouse-drag handler that modifies `tileNdxes[]` and `blendTileNdxes[]` based on brush |
| **Dynamic atlas/mesh updates** | Need: re-upload modified tile data and mesh chunks after painting |
| **Undo/redo system** | Need: command pattern for reversible edits |
| **Map serialization** | Need: write modified map data back to .map file (DataChunk writer) |
| **Viewport brush cursor** | Need: render a circle/square overlay showing brush extent on terrain |
| **Blend recalculation** | Need: after painting, recompute blend tile infos for affected cells and neighbors |
| **GPU texture update** | Need: `TextureManager` method to update a sub-region of an existing atlas texture (partial upload) |
| **Real-time atlas patching** | Need: when a texture is painted, update only the affected atlas region on GPU |
</task_result>
```

**Tool: task**

**Input:**
```json
{
  "description": "Explore reference SAGE engine code",
  "subagent_type": "explore",
  "prompt": "Explore the reference SAGE engine source code in lib/GeneralsGameCode/ to understand how the original WorldBuilder handled terrain texture painting. Specifically look for:\n\n1. How WorldBuilder applies textures to terrain tiles - look for brush/paint related code in GeneralsMD/Code/Tools/WorldBuilder/\n2. How BlendTileData works - what data structures store which texture is applied to which tile\n3. How terrain blending works between adjacent tiles with different textures - the blend patterns and alpha masks\n4. How the tileNdxes array maps tiles to textures\n5. How cliff detection and special cliff UVs work\n6. Any brush tool implementations for painting terrain textures\n7. How TerrainType definitions connect texture names to actual texture files\n\nSearch in:\n- lib/GeneralsGameCode/GeneralsMD/Code/Tools/WorldBuilder/\n- lib/GeneralsGameCode/Generals/Code/Tools/WorldBuilder/\n- lib/GeneralsGameCode/Generals/Code/GameEngine/Source/GameClient/Terrain/\n- lib/GeneralsGameCode/GeneralsMD/Code/GameEngine/Source/GameClient/Terrain/\n- Any files related to WorldHeightMap, TileData, TerrainTex, BlendTileData\n\nReturn a comprehensive summary of how the original engine handles terrain texture painting, blending, and the data flow from brush tool to rendered terrain."
}
```

**Output:**
```
task_id: ses_359e4ccffffewcTjeQGboPTZb4 (for resuming to continue this task if needed)

<task_result>
Now I have everything. Here is the comprehensive summary:

---

## Comprehensive Summary: SAGE Engine Terrain Texture Painting System

### 1. Architecture Overview

The terrain texture system involves three layers of classes:

- **`WorldHeightMap`** (`Core/GameEngineDevice/`) -- The runtime base class. Stores all tile/blend/cliff data arrays and handles reading from map files, UV coordinate generation, and texture atlas management.
- **`WorldHeightMapEdit`** (`Tools/WorldBuilder/`) -- The editing subclass used by WorldBuilder. Contains all painting, blending, flood-fill, cliff adjustment, and tile optimization logic. Maintains a static `m_globalTextureClasses[]` array of all available textures loaded from INI.
- **Tool classes** (`TileTool`, `BigTileTool`, `FloodFillTool`, `BlendEdgeTool`, `AutoEdgeOutTool`, `EyedropperTool`, `FeatherTool`) -- MFC-based UI tools that call into `WorldHeightMapEdit`.

### 2. TerrainType Definitions: INI to Texture File Pipeline

**File:** `TerrainTypes.h` / `TerrainTypes.cpp`

Each terrain type is defined in INI with this parse table:
```
TerrainType <name>
  Texture = <filename.tga>     ; e.g., "TEDesert1.tga"
  BlendEdges = Yes/No          ; whether this is a blend edge tile
  Class = DESERT_1             ; enum classification
  RestrictConstruction = No    ; pathfinding flag
End
```

The `TerrainTypeCollection` holds a linked list of `TerrainType` objects. Each has:
- `m_name` -- e.g., `"TEDesert1"`
- `m_texture` -- e.g., `"TEDesert1.tga"`
- `m_blendEdgeTexture` -- whether it provides custom blend edges
- `m_class` -- a `TerrainClass` enum value

**File resolution:** `TERRAIN_TGA_DIR_PATH` is defined as `"Art/Terrain/"`. The full path becomes `"Art/Terrain/" + terrain->getTexture()`, e.g., `"Art/Terrain/TEDesert1.tga"`. These TGA files live inside `TerrainZH.big`.

### 3. Texture Loading: TGA to TileData

**File:** `WHeightMapEdit.cpp` -- `loadImagesFromTerrainType()` and `loadBaseImages()`

On WorldBuilder startup, `init()` calls `loadBaseImages()`, which iterates over all `TerrainType` definitions from `TheTerrainTypes` and calls `loadImagesFromTerrainType()` for each.

Each TGA file is read and decomposed into 64x64-pixel tiles:
1. `WorldHeightMap::countTiles()` reads the TGA header to determine how many 64x64 tiles fit (e.g., a 128x128 TGA yields 4 tiles in a 2x2 grid; a 256x256 yields 16 in 4x4).
2. `WorldHeightMap::readTiles()` breaks the TGA into individual `TileData` objects. Each `TileData` holds a 64x64 BGRA bitmap (`m_tileData[DATA_LEN_BYTES]` = 64*64*4 = 16384 bytes) plus pre-generated mip levels down to 1x1.
3. These are stored in the static `m_globalTextureClasses[NUM_TEXTURE_CLASSES]` array (max 256 classes). Each `TGlobalTextureClass` entry has:
   - `numTiles` -- total tiles (e.g., 4 for a 2x2 source image)
   - `width` -- tiles per row (e.g., 2 for a 128x128 source)
   - `tiles[MAX_TILES_PER_CLASS]` -- pointers to `TileData` objects (max 100)
   - `isBlendEdgeTile` -- flag for blend edge textures (names starting with `"TE"`)
   - `terrainType` -- back-pointer to `TerrainType`
   - `name`, `filePath`, `uiName` -- identifiers

### 4. The tileNdxes Array: Mapping Cells to Textures

**Key data structure:** `Short *m_tileNdxes` -- a flat array of `m_width * m_height` entries, one per heightmap cell.

Each `tileNdx` value encodes two things:
- **Top 14 bits** (`tileNdx >> 2`): Index into `m_sourceTiles[]`, identifying which 64x64 tile bitmap
- **Bottom 2 bits** (`tileNdx & 3`): Quadrant within that 64x64 tile (since each terrain cell maps to 32x32 pixels, i.e., one quarter of a tile)

**Quadrant encoding:** Bit 0 = X sub-index, Bit 1 = Y sub-index. So `tileNdx & 1` selects left/right half, `tileNdx & 2` selects top/bottom half.

**How a texture class maps to tile indices** (`getTileNdxForClass()`):
```cpp
Int tileNdx = firstTile + ((xIndex/2) % width) + width * ((yIndex/2) % width);
tileNdx = tileNdx << 2;          // shift to make room for quadrant
tileNdx += 2 * (yIndex & 0x01);  // y sub-index
tileNdx += (xIndex & 0x01);      // x sub-index
```
This means every 2x2 block of terrain cells uses the same 64x64 tile, and the tile selection wraps/tiles across the texture class's grid of source tiles.

**Per-map texture classes:** The map maintains `m_textureClasses[NUM_TEXTURE_CLASSES]` (local to the map), each with `firstTile`, `numTiles`, `width`, `globalTextureClass` (index into the global array), and `name`. The `m_sourceTiles[NUM_SOURCE_TILES]` array (max 1024) stores the actual `TileData` pointers used by this map.

### 5. Terrain Blending Between Adjacent Tiles

**Key data structures:**
- `Short *m_blendTileNdxes` -- per-cell index into `m_blendedTiles[]`. Value 0 = no blend.
- `Short *m_extraBlendTileNdxes` -- per-cell secondary blend (for 3-way blending). Value 0 = no extra blend.
- `TBlendTileInfo m_blendedTiles[NUM_BLEND_TILES]` (max 16192) -- blend descriptors.

Each `TBlendTileInfo` contains:
```cpp
struct TBlendTileInfo {
    Int blendNdx;              // tileNdx of the OVERLAY texture to blend in
    UnsignedByte horiz;        // horizontal blend (left-right gradient)
    UnsignedByte vert;         // vertical blend (top-bottom gradient)
    UnsignedByte rightDiagonal; // right diagonal blend
    UnsignedByte leftDiagonal;  // left diagonal blend
    UnsignedByte inverted;      // bit 0: invert direction, bit 1: force flip for 3-way
    UnsignedByte longDiagonal;  // wider diagonal (covers 3 corners instead of 1)
    Int customBlendEdgeClass;   // -1 = use alpha gradient, else edge texture class
};
```

**The 12 alpha blend patterns** are generated procedurally in `setupAlphaTiles()`:
```
Index 0: K_HORIZ           -- horizontal gradient
Index 1: K_VERT            -- vertical gradient
Index 2: K_LDIAG           -- left diagonal gradient
Index 3: K_RDIAG           -- right diagonal gradient
Index 4: K_LLDIAG           -- long left diagonal
Index 5: K_LRDIAG           -- long right diagonal
Index 6-11: same as 0-5 but INVERTED (gradient direction reversed)
```

Each is a 64x64 TileData where only the alpha channel matters. The alpha is computed as a linear gradient:
- **Horizontal:** `alpha = h / 63` where `h` is the x position (or `63-h` if inverted)
- **Vertical:** `alpha = v / 63` where `v` is the y position (or `63-v` if inverted)
- **Diagonal:** `alpha = (v + h) / 63` for left diagonal, `alpha = (v + (63-h)) / 63` for right diagonal
- **Long diagonal:** Same but subtracts `TILE_PIXEL_EXTENT` from the sum, creating a wider coverage

These gradients create smooth transitions. The alpha value determines per-pixel blending: the overlay texture shows through where alpha is high, and the base texture shows where alpha is low.

**Rendering pass order:**
1. **Pass 1 (Base):** Render base tile from `m_tileNdxes[]` with full opacity
2. **Pass 2 (Alpha blend):** If `m_blendTileNdxes[cell] != 0`, render the blend tile's texture (`blendedTiles[ndx].blendNdx`) using per-vertex alpha values derived from the blend direction. The alpha values are set per-corner:
   - Horizontal: two corners get 255, two get 0
   - Vertical: two corners get 255, two get 0
   - Diagonal: one corner gets 255 (or three for long diagonal)
3. **Pass 3 (Extra blend):** If `m_extraBlendTileNdxes[cell] != 0`, render a third texture layer for 3-way blends, same mechanism.

**Triangle flip state:** The `m_cellFlipState` bitfield determines which diagonal splits each quad into two triangles. This is crucial for blends -- diagonal blends require specific triangle orientations to look correct. The `FLIPPED_MASK` (bit 1 of `inverted`) forces a specific flip state when 3-way blending requires the base and extra layers to share the same triangle orientation.

### 6. Brush Tool Implementations

**`TileTool` / `BigTileTool`** (texture paint brush):
1. On `mouseDown()`, gets the foreground (`TRACK_L`) or background (`TRACK_R`) texture class from `TerrainMaterial` panel
2. Creates a duplicate of the height map (`pDoc->GetHeightMap()->duplicate()`) for undo
3. On `mouseMoved()`, interpolates mouse positions for smooth strokes, converts view coordinates to cell indices
4. For each cell in the brush width: calls `m_htMapEditCopy->setTileNdx(i, j, textureClass, singleTile)`
5. `setTileNdx()` computes the appropriate tile index via `getTileNdxForClass()`, sets `m_tileNdxes[ndx]`, and clears `m_blendTileNdxes[ndx]`, `m_extraBlendTileNdxes[ndx]`, and `m_cliffInfoNdxes[ndx]` to 0 (removing any existing blend/cliff at that cell)
6. If `singleTile` is true, calls `updateFlatCellForAdjacentCliffs()` to adjust UV coordinates on neighboring cliff cells
7. On `mouseUp()`, creates an undoable command and pushes it to the document

**`FloodFillTool`** (bucket fill):
- On click, calls `htMapEditCopy->floodFill(ndx.x, ndx.y, textureClass, shiftKey)`
- `floodFill()` does BFS from the click point, replacing all connected cells of the same texture class
- If `doReplace` (shift-click), replaces the texture class globally across the entire map (even in blend references)
- Can also do cliff texture adjustments with `doCliffAdjustment()`

**`BlendEdgeTool`** (manual edge blending):
- User drags from a source cell to a destination cell
- Left-click: `htMapEditCopy->blendTile(to, from, -1, -1)` -- auto-detects source texture class
- Right-click: `htMapEditCopy->blendTile(to, from, fgTexClass, -1)` -- uses foreground texture from panel
- `blendTile()` computes the direction (horizontal/vertical/diagonal) from source to destination and creates a `TBlendTileInfo` entry

**`AutoEdgeOutTool`** (automatic edge smoothing):
- Single-click on a tile to auto-blend all edges of that texture region
- Calls `htMapEditCopy->autoBlendOut(ndx.x, ndx.y, blendTexClass)`
- `autoBlendOut()` does a flood-fill to find all cells of the same texture class, identifies border cells, then for each neighbor of a different class, calls `blendToThisClass()` which selects the correct blend type:
  - **0 side neighbors:** corner/diagonal blend
  - **1 side neighbor:** edge/side blend
  - **2 side neighbors:** concave corner blend using `longDiagonal`
  - **3+ side neighbors:** cell gets converted to the majority texture

**`EyedropperTool`** (texture picker):
- Click to pick up the texture at a cell: `pMap->getTextureClass(ndx.x, ndx.y, true)` then `TerrainMaterial::setFgTexClass(texClass)`

### 7. Cliff Detection and Special Cliff UVs

**Key data structures:**
- `Short *m_cliffInfoNdxes` -- per-cell index into `m_cliffInfo[]`. Value 0 = no cliff override.
- `TCliffInfo m_cliffInfo[NUM_CLIFF_INFO]` (max 32384) -- custom UV coordinates per cliff cell
- `UnsignedByte *m_cellCliffState` -- bitfield marking cells as cliffs for pathfinding

**Cliff detection** (`setCellCliffFlagFromHeights()`):
```cpp
Real maxZ - minZ > PATHFIND_CLIFF_SLOPE_LIMIT_F  // 9.8 world units
```
Where heights of the 4 corners of a cell are compared after scaling by `MAP_HEIGHT_SCALE` (0.625).

**Cliff UV system** (`doCliffAdjustment()` in WHeightMapEdit):
For steep cells, the standard flat UV mapping causes extreme texture stretching. The cliff system computes custom per-vertex UV coordinates based on the height gradient:

1. For each cliff cell, compute the height delta vector across the cell
2. Derive a UV direction vector perpendicular to the cliff face
3. Set V coordinates proportional to height: `v = startV - (height * HEIGHT_SCALE * TEX_PER_CELL)`
4. Set U coordinates based on the direction vector, normalized and scaled by `TEX_PER_CELL` (= 32/2048)
5. Store as `TCliffInfo` with custom UVs for all 4 corners: `(u0,v0), (u1,v1), (u2,v2), (u3,v3)`
6. The `flip` field determines triangle orientation for the cliff
7. The `mutant` flag marks cells where UV continuity with neighbors could not be maintained

**Adjacent cliff coordination** (`updateForAdjacentCliffs()`):
When computing cliff UVs, the system checks already-processed neighboring cells and snaps shared edges to match, preventing seams. It "locks" corner UVs from adjacent cells and derives the remaining corners from the cliff's own height gradient. This is a complex multi-pass algorithm that handles cases of 1, 2, 3, or 4 locked corners.

**UV tiling adjustment** (`adjustForTiling()`):
Ensures cliff UVs stay within the texture class's region, handling wrap-around at texture boundaries.

### 8. The Texture Atlas

**Constants:**
- `TEXTURE_WIDTH = 2048` pixels
- `TILE_PIXEL_EXTENT = 64` pixels per tile
- `TILE_OFFSET = 8` pixels of padding between tiles in the atlas
- `TILE_BYTES_PER_PIXEL = 4` (BGRA)
- `tilesPerRow = TEXTURE_WIDTH / (TILE_PIXEL_EXTENT + TILE_OFFSET)` = 2048/72 = 28

**Atlas layout** (`updateTileTexturePositions()`):
Tile classes are placed into the atlas using a largest-first bin-packing algorithm. For each texture class, a contiguous `width x width` block of available grid cells is found. The 28x28 grid of possible tile positions is tracked with a boolean `availableGrid[][]`. Two separate atlases are maintained:
- Base terrain atlas (`m_terrainTex`) -- `TerrainTextureClass`
- Blend edge atlas (`m_alphaEdgeTex`) -- `AlphaEdgeTextureClass`

### 9. Complete Data Flow: Brush to Rendered Terrain

```
[INI] TerrainType definitions
  |
  v
[loadImagesFromTerrainType] Read TGAs into m_globalTextureClasses[]
  |
  v
[User selects texture in TerrainMaterial panel] -> m_currentFgTexture
  |
  v
[TileTool.mouseMoved()] -> getCellIndex -> setTileNdx(x, y, textureClass)
  |
  v
[setTileNdx]:
  1. getTileNdxForClass(x, y, class) -> allocateTiles() if needed
     - Copies TileData from global to m_sourceTiles[]
     - Creates m_textureClasses[] entry with firstTile, width
     - Computes: tileNdx = (firstTile + tilePosition) << 2 | quadrant
  2. m_tileNdxes[ndx] = tileNdx
  3. m_blendTileNdxes[ndx] = 0 (clear blend)
  4. m_cliffInfoNdxes[ndx] = 0 (clear cliff)
  |
  v
[updateHeightMap()] -> triggers 3D view update
  |
  v
[HeightMapRenderObjClass uses]:
  - getUVData(x,y) -> reads m_tileNdxes[ndx], maps to UV in atlas
  - getAlphaUVData(x,y) -> reads m_blendTileNdxes[ndx], generates
    per-vertex alpha + UV for blend overlay
  - getExtraAlphaUVData(x,y) -> same for 3-way blend layer
  - Cliff override: if m_cliffInfoNdxes[ndx] != 0, uses TCliffInfo
    custom UVs instead of standard tile UVs
  |
  v
[Multi-pass rendering]:
  Pass 1: Base texture (full atlas UV from m_tileNdxes)
  Pass 2: Alpha-blended overlay (atlas UV + per-vertex alpha from m_blendedTiles)
  Pass 3: Extra alpha blend (3-way terrain transitions)
  Pass 4+: Cloud shadows, macro texture, shoreline, etc.
```

### Key Files Summary

| File | Role |
|---|---|
| `WHeightMapEdit.h/cpp` | All editing logic: paint, blend, flood fill, cliff adjust, optimize, save |
| `WorldHeightMap.h/cpp` | Runtime data storage, file parsing, UV generation, alpha tile setup |
| `TileData.h/cpp` | 64x64 BGRA bitmap tile with mip chain |
| `TerrainTypes.h/cpp` | INI-defined terrain definitions (name -> TGA path) |
| `TerrainTex.h/cpp` | D3D texture atlas construction from TileData |
| `TileTool.cpp` | Brush paint tool |
| `FloodFillTool.cpp` | Bucket fill tool |
| `BlendEdgeTool.cpp` | Manual edge blend tool |
| `AutoEdgeOutTool.cpp` | Automatic edge smoothing tool |
| `EyedropperTool.cpp` | Texture picker tool |
| `TerrainMaterial.h/cpp` | UI panel for terrain texture selection (fg/bg, brush width) |
| `BlendMaterial.h/cpp` | UI panel for blend edge texture selection |
| `MapObject.h` | Defines `MAP_XY_FACTOR` (10.0) and `MAP_HEIGHT_SCALE` (0.625) |
| `FileSystem.h` | Defines `TERRAIN_TGA_DIR_PATH` = `"Art/Terrain/"` |
</task_result>
```

---

## Assistant (Plan · claude-opus-4-6 · 823.4s)

Excellent. I now have a thorough understanding of both the current state of the project and how the original SAGE engine handled terrain texture painting. Before I propose a plan, I have several clarifying questions:

**Tool: question**

**Input:**
```json
{
  "questions": [
    {
      "header": "Texture Source",
      "question": "Where will the actual terrain TGA textures come from? The project has BIG archive support and TerrainType INI parsing, but the TGAs aren't currently being extracted or loaded. Do you have TerrainZH.big and INIZH.big available, and should the plan include wiring up the extraction pipeline?",
      "options": [
        {
          "label": "Wire up BIG extraction",
          "description": "Automatically extract terrain TGAs from TerrainZH.big and parse Terrain.ini from INIZH.big during map load"
        },
        {
          "label": "Load from loose files",
          "description": "Support loading terrain TGAs from a user-specified Art/Terrain/ directory on disk"
        },
        {
          "label": "Both approaches",
          "description": "Support BIG archive extraction as primary, with loose file fallback"
        }
      ]
    },
    {
      "header": "Real textures first?",
      "question": "The current terrain rendering uses a procedural solid-color atlas placeholder, and the blend-data-aware mesh path exists but isn't wired up. Should the plan start by getting real textures rendered correctly (including multi-pass blending) BEFORE adding painting tools, or do you want painting tools that work with the procedural placeholder first?",
      "options": [
        {
          "label": "Real textures first (Recommended)",
          "description": "Get actual terrain textures rendering with proper blending before adding any painting tools"
        },
        {
          "label": "Painting tools first",
          "description": "Build the painting infrastructure using the procedural atlas, then swap in real textures later"
        }
      ]
    },
    {
      "header": "Blending approach",
      "question": "The original engine uses a multi-pass approach (base + alpha blend overlay + extra blend). For the Vulkan implementation, do you have a preference on approach?",
      "options": [
        {
          "label": "Multi-pass (Recommended)",
          "description": "Match the original engine: separate draw calls for base, blend, and extra blend layers with alpha blending. Simpler shader, closer to reference."
        },
        {
          "label": "Splatmap in shader",
          "description": "Use a texture array + blend weight texture, do all blending in a single shader pass. More modern but diverges from reference code."
        },
        {
          "label": "Your recommendation",
          "description": "You decide the best approach given the existing codebase"
        }
      ]
    },
    {
      "header": "Scope of painting",
      "question": "How much of the WorldBuilder's terrain editing do you want in this first plan? The original had: tile paint, flood fill, auto-edge blending, manual blend edges, eyedropper, cliff adjustment, height editing. Should we scope this to just texture painting or go broader?",
      "options": [
        {
          "label": "Texture painting only",
          "description": "Just the brush tool for painting textures onto terrain, plus auto-blend. No height editing."
        },
        {
          "label": "Full terrain editing",
          "description": "Texture painting + height editing (raise/lower/flatten/smooth) in one plan"
        },
        {
          "label": "Painting + basic tools",
          "description": "Texture painting brush, flood fill, eyedropper, and auto-edge blending. No height editing yet."
        }
      ]
    },
    {
      "header": "Undo/redo priority",
      "question": "Should the plan include an undo/redo system from the start, or defer it?",
      "options": [
        {
          "label": "Include from start (Recommended)",
          "description": "Build undo/redo infrastructure as part of the editing foundation - prevents costly refactoring later"
        },
        {
          "label": "Defer to later",
          "description": "Get basic painting working first, add undo/redo in a follow-up plan"
        }
      ]
    },
    {
      "header": "Map saving",
      "question": "Should the plan include writing modified maps back to .map files (DataChunk writer), or is viewing/painting without saving sufficient for now?",
      "options": [
        {
          "label": "Include map saving",
          "description": "Implement a DataChunk writer to serialize modified terrain back to .map format"
        },
        {
          "label": "Defer saving",
          "description": "Focus on rendering and painting; saving comes in a future plan"
        }
      ]
    }
  ]
}
```

**Output:**
```
User has answered your questions: "Where will the actual terrain TGA textures come from? The project has BIG archive support and TerrainType INI parsing, but the TGAs aren't currently being extracted or loaded. Do you have TerrainZH.big and INIZH.big available, and should the plan include wiring up the extraction pipeline?"="Wire up BIG extraction", "The current terrain rendering uses a procedural solid-color atlas placeholder, and the blend-data-aware mesh path exists but isn't wired up. Should the plan start by getting real textures rendered correctly (including multi-pass blending) BEFORE adding painting tools, or do you want painting tools that work with the procedural placeholder first?"="Real textures first (Recommended)", "The original engine uses a multi-pass approach (base + alpha blend overlay + extra blend). For the Vulkan implementation, do you have a preference on approach?"="Splatmap in shader", "How much of the WorldBuilder's terrain editing do you want in this first plan? The original had: tile paint, flood fill, auto-edge blending, manual blend edges, eyedropper, cliff adjustment, height editing. Should we scope this to just texture painting or go broader?"="Nothing this is just what the final result should go towards - this is still just a rendering application for now", "Should the plan include an undo/redo system from the start, or defer it?"="Still only a rendering tool for now with a Worldbuilder focus for the future", "Should the plan include writing modified maps back to .map files (DataChunk writer), or is viewing/painting without saving sufficient for now?"="Focus only on rendering". You can now continue with the user's answers in mind.
```

---

## Assistant (Plan · claude-opus-4-6 · 132.5s)

Thank you for the clarifications. So the scope is focused: **get real terrain textures rendering correctly with proper blending** -- no editing tools yet, but architected with the future WorldBuilder direction in mind.

Let me ask one more set of questions to nail down the details:

**Tool: question**

**Input:**
```json
{
  "questions": [
    {
      "header": "Splatmap shader details",
      "question": "For the splatmap-in-shader approach: the original engine has up to ~30+ terrain texture classes per map. Vulkan texture arrays have device-dependent layer limits (usually 256+). The options are:",
      "options": [
        {
          "label": "Texture array + blend weights (Recommended)",
          "description": "One VkImage texture array with all terrain tiles as layers. Per-cell blend weight data in a separate texture/SSBO. Single draw call per chunk."
        },
        {
          "label": "Atlas + splatmap texture",
          "description": "Keep the 2048-wide atlas approach but add a per-cell splatmap texture that stores blend weights and texture indices. Closer to existing UV code."
        }
      ]
    },
    {
      "header": "Blend weight storage",
      "question": "Each cell can have: a base texture, a blend overlay, and an extra blend (3-way). How should blend weights be stored for the shader?",
      "options": [
        {
          "label": "Per-vertex attributes",
          "description": "Encode blend texture IDs and weights as additional vertex attributes. Requires mesh rebuild on data change but simple."
        },
        {
          "label": "Data texture/SSBO",
          "description": "Store blend info in a GPU texture or SSBO indexed by cell coordinates. Shader looks up blend data. Decouples mesh from blend state."
        },
        {
          "label": "Your recommendation",
          "description": "Pick whatever fits best with the existing terrain_mesh/terrain_renderable code"
        }
      ]
    },
    {
      "header": "Performance target",
      "question": "What map sizes should this handle smoothly? This affects chunking strategy and GPU upload approach.",
      "options": [
        {
          "label": "Standard ZH maps",
          "description": "Up to 500x500 cells (typical large skirmish maps). ~1M triangles."
        },
        {
          "label": "Maximum size maps",
          "description": "Up to 1024x1024 cells (WorldBuilder maximum). ~4M triangles."
        },
        {
          "label": "Both with LOD",
          "description": "Support maximum maps with distance-based LOD for performance"
        }
      ]
    }
  ]
}
```

**Output:**
```
User has answered your questions: "For the splatmap-in-shader approach: the original engine has up to ~30+ terrain texture classes per map. Vulkan texture arrays have device-dependent layer limits (usually 256+). The options are:"="Texture array + blend weights (Recommended)", "Each cell can have: a base texture, a blend overlay, and an extra blend (3-way). How should blend weights be stored for the shader?"="Your recommendation", "What map sizes should this handle smoothly? This affects chunking strategy and GPU upload approach."="Maximum size maps". You can now continue with the user's answers in mind.
```

---

## Assistant (Plan · claude-opus-4-6 · 91.1s)

Good. Now I have all the context I need. Let me verify one more thing about the existing code to make my recommendation on blend weight storage precise:

**Tool: task**

**Input:**
```json
{
  "description": "Check terrain vertex and shader",
  "subagent_type": "explore",
  "prompt": "Quick exploration to answer specific questions about the existing terrain rendering code:\n\n1. Read src/render/terrain/terrain_mesh.hpp -- what is the current TerrainVertex struct layout? What vertex attributes exist?\n2. Read src/render/terrain/terrain_renderable.hpp and .cpp -- how is the vertex input binding configured? How many vertex attributes are bound?\n3. Read src/render/terrain/terrain_atlas.hpp -- what does computeTileUVTable return? What's the TileUVEntry struct?\n4. Read shaders/terrain.vert and terrain.frag -- full shader source\n5. Read src/lib/gfx/pipeline.hpp and pipeline.cpp -- how is PipelineCreateInfo::terrain() configured? What vertex input state does it specify?\n6. Read src/lib/formats/map/types.hpp -- what are the BlendTileInfo and BlendTileData structures?\n7. Check if the existing TerrainVertex has room for additional attributes or if there are any SSBOs already bound.\n\nReturn the exact struct definitions, shader source, and pipeline configuration details."
}
```

**Output:**
```
task_id: ses_359d326e5ffe6p2UWW4Gp2iyyi (for resuming to continue this task if needed)

<task_result>
Here is the complete summary of findings:

---

## 1. TerrainVertex Struct Layout

**File:** `C:\CodeProjects\VulkanW3DViewer\src\render\terrain\terrain_mesh.hpp` (lines 14-19)

```cpp
struct TerrainVertex {
  glm::vec3 position;    // 12 bytes (offset 0)
  glm::vec3 normal;      // 12 bytes (offset 12)
  glm::vec2 texCoord;    // 8 bytes  (offset 24)
  glm::vec2 atlasCoord;  // 8 bytes  (offset 32)
};
// Total: 40 bytes
```

**4 vertex attributes:** position (vec3), normal (vec3), texCoord (vec2), atlasCoord (vec2).

---

## 2. Vertex Input Binding Configuration

**File:** `C:\CodeProjects\VulkanW3DViewer\src\lib\gfx\pipeline.hpp` (lines 177-203) -- `PipelineCreateInfo::terrain()`

```cpp
static PipelineCreateInfo terrain() {
    PipelineCreateInfo info;
    info.vertShaderPath = "shaders/terrain.vert.spv";
    info.fragShaderPath = "shaders/terrain.frag.spv";

    info.vertexInput.binding =
        vk::VertexInputBindingDescription{0, 40, vk::VertexInputRate::eVertex};
    info.vertexInput.attributes = {
        vk::VertexInputAttributeDescription{0, 0, vk::Format::eR32G32B32Sfloat, 0 },  // position
        vk::VertexInputAttributeDescription{1, 0, vk::Format::eR32G32B32Sfloat, 12},  // normal
        vk::VertexInputAttributeDescription{2, 0, vk::Format::eR32G32Sfloat,    24},  // texCoord
        vk::VertexInputAttributeDescription{3, 0, vk::Format::eR32G32Sfloat,    32}   // atlasCoord
    };
    // ...
}
```

- **1 binding** (binding 0), stride 40 bytes, per-vertex rate.
- **4 attributes** bound, all in binding 0, locations 0-3.
- **Descriptor bindings:** binding 0 = UBO (vertex stage), binding 1 = combined image sampler (fragment stage).
- **Push constants:** `TerrainPushConstant` (fragment stage only), 80 bytes total.
- **No blending**, depth write enabled, back-face culling.

---

## 3. TileUV Struct and computeTileUVTable

**File:** `C:\CodeProjects\VulkanW3DViewer\src\render\terrain\terrain_atlas.hpp` (lines 10-38)

```cpp
struct TileUV {
  float u = 0.0f;      // left U coordinate in atlas
  float v = 0.0f;      // top V coordinate in atlas
  float uSize = 0.0f;  // width in UV space
  float vSize = 0.0f;  // height in UV space
};
```

`computeTileUVTable` signature:
```cpp
[[nodiscard]] std::vector<TileUV>
computeTileUVTable(const std::vector<map::TextureClass> &textureClasses,
                   int32_t atlasWidth = 2048, int32_t tilePixelSize = 64);
```
It returns a `std::vector<TileUV>` -- one UV entry per tile, mapping each tile index to its position/size in the atlas.

Helper functions: `decodeTileIndex(int16_t)` extracts the top 14 bits (source tile index), `decodeQuadrant(int16_t)` extracts the bottom 2 bits (32x32 quadrant), and `decodeTileNdxUV` combines both to produce a final quadrant-level UV.

---

## 4. Full Shader Source

### terrain.vert (55 lines)

```glsl
#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
  mat4 model;
  mat4 view;
  mat4 proj;
  vec4 lightDirection;
  vec4 ambientColor;
  vec4 diffuseColor;
} ubo;

layout(push_constant) uniform TerrainMaterial {
  vec4 ambientColor;
  vec4 diffuseColor;
  vec3 lightDirection;
  uint useTexture;
  vec4 shadowColor;
  float cloudScrollU;
  float cloudScrollV;
  float cloudTime;
  float cloudStrength;
} material;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec2 inAtlasCoord;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragWorldPos;
layout(location = 3) out vec2 fragAtlasCoord;
layout(location = 4) out vec2 fragCloudCoord;

const float kCloudUVScale = 0.002;

void main() {
  vec4 worldPos = ubo.model * vec4(inPosition, 1.0);
  gl_Position = ubo.proj * ubo.view * worldPos;
  fragNormal = mat3(transpose(inverse(ubo.model))) * inNormal;
  fragTexCoord = inTexCoord;
  fragWorldPos = worldPos.xyz;
  fragAtlasCoord = inAtlasCoord;
  vec2 cloudBase = worldPos.xz * kCloudUVScale;
  fragCloudCoord = cloudBase + vec2(material.cloudScrollU * material.cloudTime,
                                    material.cloudScrollV * material.cloudTime);
}
```

### terrain.frag (100 lines)

```glsl
#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec2 fragAtlasCoord;
layout(location = 4) in vec2 fragCloudCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D texSampler;

layout(push_constant) uniform TerrainMaterial {
  vec4 ambientColor;
  vec4 diffuseColor;
  vec3 lightDirection;
  uint useTexture;
  vec4 shadowColor;
  float cloudScrollU;
  float cloudScrollV;
  float cloudTime;
  float cloudStrength;
} material;

// ... hash-based noise functions (hash21, smoothNoise, cloudPattern) ...

void main() {
  vec3 normal = normalize(fragNormal);
  vec3 baseColor;
  if (material.useTexture == 1u) {
    baseColor = texture(texSampler, fragAtlasCoord).rgb;    // samples atlas texture
  } else {
    // fallback: procedural height-based gradient (green low, brown high)
    float t = clamp(fragWorldPos.y / 100.0, 0.0, 1.0);
    baseColor = mix(vec3(0.35, 0.55, 0.25), vec3(0.65, 0.55, 0.40), t);
  }
  vec3 lightDir = normalize(-material.lightDirection);
  vec3 ambient = material.ambientColor.rgb * baseColor;
  float diff = max(dot(normal, lightDir), 0.0);
  vec3 diffuse = material.diffuseColor.rgb * diff * baseColor;
  vec3 result = ambient + diffuse;
  // Shadow colour tint (Phase 6.2)
  if (material.shadowColor.a > 0.0) { /* lerp by shadow factor */ }
  // Cloud shadow overlay (Phase 6.3)
  if (material.cloudStrength > 0.0) { /* procedural cloud pattern attenuation */ }
  outColor = vec4(result, 1.0);
}
```

---

## 5. PipelineCreateInfo::terrain() -- Full Configuration

From `C:\CodeProjects\VulkanW3DViewer\src\lib\gfx\pipeline.hpp` lines 177-203:

| Property | Value |
|---|---|
| **Vertex shader** | `shaders/terrain.vert.spv` |
| **Fragment shader** | `shaders/terrain.frag.spv` |
| **Vertex stride** | 40 bytes |
| **Vertex attributes** | 4 (locations 0-3) |
| **Topology** | TriangleList (default) |
| **Cull mode** | Back (twoSided=false) |
| **Depth test** | Enabled, write enabled |
| **Blending** | Disabled |
| **Descriptor set** | binding 0: UBO (vertex), binding 1: sampler2D (fragment) |
| **Push constants** | `TerrainPushConstant` (80 bytes), fragment stage only |

The `TerrainPushConstant` struct (lines 80-92):
```cpp
struct TerrainPushConstant {
  alignas(16) glm::vec4 ambientColor;     // 16 bytes
  alignas(16) glm::vec4 diffuseColor;     // 16 bytes
  alignas(16) glm::vec3 lightDirection;   // 12 bytes
  alignas(4)  uint32_t useTexture;        //  4 bytes
  alignas(16) glm::vec4 shadowColor;      // 16 bytes
  alignas(4)  float cloudScrollU;         //  4 bytes
  alignas(4)  float cloudScrollV;         //  4 bytes
  alignas(4)  float cloudTime;            //  4 bytes
  alignas(4)  float cloudStrength;        //  4 bytes
};                                        // = 80 bytes
```

Note: push constants are declared for fragment stage only (`vk::ShaderStageFlagBits::eFragment`), but the vertex shader also declares the same push_constant block (GLSL allows this -- Vulkan drivers typically handle it, though the stage flag technically should include `eVertex` since `terrain.vert` reads `material.cloudScrollU/V/Time`).

---

## 6. BlendTileInfo and BlendTileData Structures

**File:** `C:\CodeProjects\VulkanW3DViewer\src\lib\formats\map\types.hpp`

```cpp
struct BlendTileInfo {           // lines 61-70
  int32_t blendNdx = 0;         // index into blend tile texture
  int8_t horiz = 0;             // horizontal blend direction
  int8_t vert = 0;              // vertical blend direction
  int8_t rightDiagonal = 0;     // right diagonal blend
  int8_t leftDiagonal = 0;      // left diagonal blend
  int8_t inverted = 0;          // bit 0 = inverted (INVERTED_MASK), bit 1 = flipped (FLIPPED_MASK)
  int8_t longDiagonal = 0;      // long diagonal blend
  int32_t customBlendEdgeClass = -1;  // custom edge class override (-1 = none)
};

struct BlendTileData {           // lines 82-104
  int32_t dataSize = 0;                            // number of cells (width-1)*(height-1)
  std::vector<int16_t> tileNdxes;                  // per-cell base tile index (top 14 = tile, bottom 2 = quadrant)
  std::vector<int16_t> blendTileNdxes;             // per-cell blend overlay tile index
  std::vector<int16_t> extraBlendTileNdxes;        // per-cell extra (3-way) blend tile index
  std::vector<int16_t> cliffInfoNdxes;             // per-cell cliff info index (-1 = not cliff)
  std::vector<uint8_t> cellCliffState;             // per-cell cliff state byte

  int32_t numBitmapTiles = 0;                      // total tile bitmaps in atlas
  int32_t numBlendedTiles = 0;                     // number of blended tile entries
  int32_t numCliffInfo = 0;                        // number of cliff info entries

  std::vector<TextureClass> textureClasses;        // terrain texture classes (name -> tile range)
  int32_t numEdgeTiles = 0;                        // edge tile count
  std::vector<TextureClass> edgeTextureClasses;    // edge texture classes
  std::vector<BlendTileInfo> blendTileInfos;       // blend info for each blended tile
  std::vector<CliffInfo> cliffInfos;               // cliff UV override data

  bool isValid() const { ... }
};
```

Supporting struct:
```cpp
struct TextureClass {         // lines 54-59
  int32_t firstTile = 0;     // first tile index in the atlas
  int32_t numTiles = 0;      // number of tiles in this class
  int32_t width = 0;         // source texture width in tiles
  std::string name;          // e.g. "TEDesert1"
};

struct CliffInfo {            // lines 72-80
  int32_t tileIndex = 0;
  float u0, v0, u1, v1, u2, v2, u3, v3;  // custom UV coordinates per corner
  int8_t flip = 0;
  int8_t mutant = 0;
};
```

---

## 7. Room for Additional Attributes / Existing SSBOs

**No SSBOs are bound** in the terrain pipeline. The terrain descriptor set has only 2 bindings:
- Binding 0: `UniformBuffer` (vertex stage)
- Binding 1: `CombinedImageSampler` (fragment stage)

This contrasts with the `skinned()` pipeline which does use an SSBO at binding 2 for bone transforms.

**Room for additional vertex attributes:** The current `TerrainVertex` is a compact 40-byte struct with 4 attributes at locations 0-3. To add more attributes you would need to:

1. Add fields to `TerrainVertex` (increasing the stride beyond 40 bytes).
2. Add corresponding `vk::VertexInputAttributeDescription` entries at location 4+ in `PipelineCreateInfo::terrain()`.
3. Add matching `layout(location = N) in ...` declarations in `terrain.vert`.
4. Update `terrain_mesh.cpp` to populate the new fields during mesh generation.

Alternatively, additional per-cell data (like blend tile indices, blend alpha, extra texture coordinates) could be passed via an **SSBO** (adding a `vk::DescriptorType::eStorageBuffer` binding at binding 2, following the skinned pipeline pattern) rather than inflating the vertex stride -- this is especially relevant for blend data that is per-cell rather than per-vertex.

There is currently **no blend-related data flowing into the shader** -- the `atlasCoord` carries the base tile UV from the atlas, but there is no blend tile index, blend alpha, or second-texture coordinate in the vertex or any buffer. The shader samples a single texture (`texSampler` at binding 1) and has no multi-texture blending logic.
</task_result>
```

---

## Assistant (Plan · claude-opus-4-6 · 88.9s)

Now I have all the details needed. Here is my comprehensive plan:

---

# Plan: Real Terrain Texture Rendering with Splatmap Blending

## Goal
Replace the procedural solid-color terrain atlas with real terrain textures extracted from BIG archives, and implement single-pass splatmap-style blending in the shader to correctly render base textures, blend overlays, and 3-way terrain transitions -- supporting maps up to 1024x1024 cells.

---

## Phase 1: Wire Up Terrain INI + TGA Extraction Pipeline

**Objective:** Load TerrainType definitions from INIZH.big and extract terrain TGA files from TerrainZH.big at map load time.

### 1.1 Load Terrain.ini from INIZH.big
- In the map loading flow (`application.cpp` or a new terrain resource manager), call into `BigArchiveManager` to extract `Data/INI/Terrain.ini` from `INIZH.big`
- Feed the extracted text to `TerrainTypeCollection::loadFromINI()`
- Store the resulting collection so it's available for texture resolution
- The INI parser and `TerrainTypeCollection` already exist and are tested

### 1.2 Extract Terrain TGAs from TerrainZH.big
- For each `TextureClass` in the loaded map's `BlendTileData`, look up the corresponding `TerrainType` by name to get the TGA filename
- Extract `Art/Terrain/<name>.tga` from `TerrainZH.big` via the existing `TextureManager`/`AssetRegistry` infrastructure
- Handle edge texture classes the same way (blend edge TGAs)

### 1.3 Split TGAs into 64x64 Tiles
- Each extracted TGA may be 64x64, 128x128, 256x256, etc.
- Split into individual 64x64 BGRA tiles (matching the original engine's `readTiles()` logic)
- Store as a flat list of tile bitmaps indexed to match the map's `tileNdxes` encoding
- The `TextureClass.firstTile`, `numTiles`, and `width` fields from `BlendTileData` define the mapping

### 1.4 Build a Real Texture Array
- Instead of a 2D atlas, create a `VkImage` with `imageType = 2D` and `arrayLayers = N` where N = total number of unique 64x64 tiles across all texture classes
- Each layer is one 64x64 tile, RGBA8, with mipmaps generated down to 1x1
- Use the existing `TextureManager::createTextureArray()` method
- This replaces the procedural atlas entirely
- **Rationale:** A texture array avoids UV seam/bleeding issues inherent in atlases and maps naturally to the splatmap shader approach

---

## Phase 2: Per-Cell Blend Data via SSBO

**Objective:** Encode per-cell texture and blend information into a GPU-side storage buffer that the terrain shader can index.

### 2.1 Define Cell Blend Data Structure
```cpp
struct CellBlendInfo {       // 16 bytes per cell, tightly packed
  uint16_t baseTileIndex;    // decoded tile index for base texture
  uint16_t baseQuadrant;     // quadrant within the 64x64 tile (0-3)
  uint16_t blendTileIndex;   // overlay tile index (0 = no blend)
  uint16_t blendQuadrant;    // overlay quadrant
  uint16_t extraTileIndex;   // extra blend tile index (0 = no extra)
  uint16_t extraQuadrant;    // extra blend quadrant
  uint8_t  blendDirection;   // encoded blend direction (0-11, from BlendTileInfo)
  uint8_t  extraDirection;   // encoded extra blend direction
  uint8_t  flags;            // cliff flag, flip state, etc.
  uint8_t  padding;
};
```

### 2.2 Build the SSBO from Parsed BlendTileData
- Iterate over all cells in the map
- For each cell, decode `tileNdxes[i]` into baseTileIndex + quadrant
- If `blendTileNdxes[i] != 0`, look up `blendTileInfos[blendTileNdxes[i]]` to get the overlay tile index and blend direction
- Same for `extraBlendTileNdxes[i]`
- For cliff cells (`cliffInfoNdxes[i] != 0`), set the cliff flag (cliff UVs are already handled in the mesh vertex data)
- Upload as a `VkBuffer` with `StorageBuffer` usage

### 2.3 Add SSBO Binding to Terrain Pipeline
- Add a third descriptor binding (binding 2) to the terrain pipeline: `StorageBuffer`, fragment stage
- Follow the existing pattern from the `skinned()` pipeline which already uses an SSBO at binding 2
- Update `DescriptorManager` to allocate and bind this descriptor

### 2.4 Pass Cell Coordinates to Shader
- Add a new vertex attribute to `TerrainVertex`: `ivec2 cellCoord` (the grid x,y of this vertex's cell)
- Or more efficiently: compute cell coordinates in the vertex shader from world position (`cellX = int(position.x / MAP_XY_FACTOR)`, `cellY = int(position.z / MAP_XY_FACTOR)`) -- this avoids inflating the vertex stride
- **Recommendation:** Compute from world position in the vertex shader to avoid changing the vertex layout. Pass as flat integer varying to fragment shader.

---

## Phase 3: Splatmap Terrain Shader

**Objective:** Rewrite the terrain fragment shader to sample from the texture array using per-cell blend data from the SSBO.

### 3.1 New Shader Inputs
```glsl
// Binding 1: texture array (replaces single sampler2D)
layout(set = 0, binding = 1) uniform sampler2DArray tileTextures;

// Binding 2: per-cell blend info SSBO
layout(std430, set = 0, binding = 2) readonly buffer CellBlendBuffer {
  CellBlendInfo cells[];
};
```

### 3.2 Base Texture Sampling
- In the fragment shader, compute the cell index from the interpolated world position: `cellIndex = cellY * mapWidth + cellX`
- Read `cells[cellIndex]` to get `baseTileIndex` and `baseQuadrant`
- Compute UV within the 64x64 tile: use `fract(worldPos.xz / MAP_XY_FACTOR)` to get the [0,1] position within the cell, then offset by quadrant (each quadrant covers half the tile in each axis)
- Sample `tileTextures` at layer `baseTileIndex` with the computed UV

### 3.3 Blend Pattern Generation in Shader
- The 12 blend directions (horiz, vert, left diagonal, right diagonal, long diagonal, + inverses) can be computed procedurally in the shader rather than storing alpha textures
- Given the blend direction enum from the SSBO, compute a blend alpha:
  - Horizontal: `alpha = uv.x` or `1.0 - uv.x`
  - Vertical: `alpha = uv.y` or `1.0 - uv.y`  
  - Diagonal: `alpha = (uv.x + uv.y) / 2.0` etc.
  - Long diagonal: wider coverage variant
- This avoids needing a separate alpha texture array

### 3.4 Multi-Layer Blending
```glsl
vec4 base = texture(tileTextures, vec3(baseUV, baseTileIndex));
vec4 result = base;

if (blendTileIndex > 0) {
    vec4 blend = texture(tileTextures, vec3(blendUV, blendTileIndex));
    float alpha = computeBlendAlpha(blendDirection, cellUV);
    result = mix(result, blend, alpha);
}

if (extraTileIndex > 0) {
    vec4 extra = texture(tileTextures, vec3(extraUV, extraTileIndex));
    float alpha = computeBlendAlpha(extraDirection, cellUV);
    result = mix(result, extra, alpha);
}
```

### 3.5 Cliff UV Override
- For cells flagged as cliffs, the custom UVs are already in the vertex data (`atlasCoord`)
- The shader should detect the cliff flag from the SSBO and use the vertex `atlasCoord` mapped to the cliff's texture layer instead of the computed cell-based UV
- This preserves the existing cliff UV system from `terrain_mesh.cpp`

### 3.6 Preserve Existing Features
- Keep the directional lighting, shadow color tint, and cloud shadow overlay from the current shader
- Keep the `useTexture` toggle so the height-gradient fallback still works when no textures are loaded
- Keep the existing UBO structure unchanged

---

## Phase 4: Wire Up the Full Pipeline

**Objective:** Connect everything in the map loading flow so real textures render on load.

### 4.1 Terrain Resource Manager
- Create a new class (e.g., `TerrainTextureLoader`) that orchestrates:
  1. Loading Terrain.ini from INIZH.big
  2. Resolving texture class names to TGA filenames
  3. Extracting and splitting TGAs into tiles
  4. Building the texture array on the GPU
  5. Building the cell blend SSBO
- This keeps `terrain_renderable.cpp` focused on rendering

### 4.2 Update Map Loading Flow
- In `Application::loadMapFile()`, after parsing the map:
  1. Call `TerrainTextureLoader` to build the texture array and blend SSBO
  2. Call `terrainRenderable_.loadWithBlendData()` (already exists but unused) instead of `terrainRenderable_.load()`
  3. Pass the texture array and SSBO to `terrainRenderable_.initPipelineWithAtlas()` (rename/extend to handle the new descriptor layout)

### 4.3 Update TerrainRenderable
- Modify `initPipelineWithAtlas()` to:
  - Accept a texture array instead of a 2D atlas texture
  - Accept and bind the cell blend SSBO
  - Use the updated pipeline with 3 descriptor bindings
- Store `mapWidth` in push constants or UBO so the shader can compute cell indices

### 4.4 Descriptor Set Updates
- The terrain pipeline descriptor set needs to go from 2 bindings to 3
- Update the descriptor pool sizes and layout creation accordingly
- This is a localized change in `terrain_renderable.cpp` (it creates its own pipeline and descriptors)

---

## Phase 5: Edge Cases and Polish

### 5.1 Missing Texture Fallback
- If a TGA can't be found in TerrainZH.big, use a visible "missing texture" pattern (e.g., magenta/black checkerboard) for that layer
- Log a warning with the missing texture name
- The application should never crash due to missing art assets

### 5.2 Map Width in Shader
- The shader needs `mapWidth` to compute SSBO cell indices from world position
- Add `mapWidth` as a new field in `TerrainPushConstant` (there's room -- currently 80 bytes, well under the 128-byte minimum guaranteed by Vulkan)

### 5.3 Mipmap Generation
- Generate mipmaps for each layer of the texture array to avoid aliasing at oblique angles
- Use `vkCmdBlitImage` or compute shader for mip generation (the `TextureManager` may already support this)

### 5.4 Texture Filtering
- Use `VK_FILTER_LINEAR` with `VK_SAMPLER_MIPMAP_MODE_LINEAR` for the terrain texture array
- Consider anisotropic filtering (typically 4x-8x) for terrain viewed at grazing angles

### 5.5 Edge Texture Classes
- Blend edge textures (the `edgeTextureClasses` in `BlendTileData`) are a separate set of tiles used for custom blend edges instead of the procedural alpha gradient
- Support the `customBlendEdgeClass` field: when it's not -1, sample the edge texture's alpha channel instead of computing the blend alpha procedurally
- This can be deferred if no test maps use custom blend edges

### 5.6 Performance Considerations for 1024x1024 Maps
- SSBO size: 1024*1024 * 16 bytes = 16 MB -- well within Vulkan limits
- Texture array: ~30 texture classes × ~4-16 tiles each = ~100-500 layers × 64×64×4 bytes × mip chain ≈ ~10-50 MB -- reasonable
- The 32x32 chunk frustum culling already handles large maps
- Ensure the SSBO index calculation in the shader handles the full range (use `uint` indexing)

---

## Phase 6: Testing

### 6.1 Unit Tests
- Test TGA splitting logic: verify a 128x128 TGA produces 4 correct 64x64 tiles
- Test cell blend data building: verify SSBO construction from known BlendTileData
- Test blend direction decoding: verify all 12 directions produce correct enum values
- Test tile index decoding: verify `decodeTileIndex` and `decodeQuadrant` match expected values for known maps

### 6.2 Visual Verification
- Load a known ZH map and visually compare against the original game/WorldBuilder
- Verify base textures tile correctly (no seams at cell boundaries)
- Verify blend transitions look smooth (compare gradient patterns)
- Verify cliff cells display without stretching
- Verify 3-way blends render all three layers

### 6.3 Performance Testing
- Load a maximum-size (1024x1024) map and verify smooth frame rate
- Profile GPU memory usage for the texture array + SSBO

---

## Implementation Order (Suggested)

| Step | Phase | Description | Dependencies |
|------|-------|-------------|--------------|
| 1 | 1.1 | Load Terrain.ini from INIZH.big | Existing INI parser + BigArchiveManager |
| 2 | 1.2-1.3 | Extract + split terrain TGAs | Step 1 (need TerrainType lookup) |
| 3 | 1.4 | Build texture array on GPU | Step 2 (need tile data) |
| 4 | 2.1-2.2 | Build cell blend SSBO from BlendTileData | Existing map parser |
| 5 | 2.3-2.4 | Add SSBO binding to pipeline | Step 3 (pipeline changes) |
| 6 | 3.1-3.6 | Write new terrain shaders | Steps 3, 5 (need texture array + SSBO) |
| 7 | 4.1-4.4 | Wire up in map load flow | Steps 3, 4, 6 |
| 8 | 5.1-5.6 | Edge cases and polish | Step 7 |
| 9 | 6.1-6.3 | Testing | Step 7 |

Steps 1-2 and step 4 can be done in parallel since they have no mutual dependencies.

---

## Files to Create/Modify

**New files:**
- `src/render/terrain/terrain_texture_loader.hpp/cpp` -- orchestrates INI loading, TGA extraction, tile splitting, texture array and SSBO construction
- `shaders/terrain.vert` -- updated vertex shader (add cell coord output)
- `shaders/terrain.frag` -- rewritten fragment shader (splatmap blending)

**Modified files:**
- `src/render/terrain/terrain_renderable.hpp/cpp` -- accept texture array + SSBO, updated descriptor set layout
- `src/render/terrain/terrain_atlas.hpp/cpp` -- may need extension for tile splitting logic, or this moves to the new loader
- `src/lib/gfx/pipeline.hpp` -- update `PipelineCreateInfo::terrain()` for 3 bindings + push constant size
- `src/core/application.cpp` -- wire up the new loading flow in `loadMapFile()`
- `src/render/terrain/terrain_mesh.hpp/cpp` -- possibly no changes if cell coords are computed in shader from world pos
- `src/lib/formats/map/types.hpp` -- possibly add helper methods on `BlendTileInfo` for direction decoding

---

Does this plan look right to you? Any phases you'd want to adjust, reorder, or cut?

---

