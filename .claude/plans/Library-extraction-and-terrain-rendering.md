# VulkanW3DViewer: Library Extraction and Terrain Rendering Plan

## Executive Summary

Transform VulkanW3DViewer from a monolithic W3D mesh viewer into a reusable rendering library with support for W3D models, terrain, and complete map files from Command & Conquer: Generals Zero Hour.

**Approach**: Full library extraction first, then add terrain support.

---

## Project Structure Transformation

### Current Structure
```
src/
├── main.cpp
├── core/          # Vulkan framework
├── render/        # W3D-specific rendering
├── w3d/           # W3D format parsing
└── ui/            # UI components
```

### Target Structure
```
src/
├── main.cpp                          # Viewer application entry point
├── lib/                              # NEW: Reusable rendering library
│   ├── gfx/                          # Graphics abstraction layer
│   │   ├── vulkan_context.hpp/cpp
│   │   ├── renderer.hpp/cpp          # Generic renderer
│   │   ├── pipeline.hpp/cpp
│   │   ├── buffer.hpp/cpp
│   │   ├── texture.hpp/cpp
│   │   ├── camera.hpp/cpp
│   │   └── renderable.hpp            # NEW: IRenderable interface
│   ├── scene/                        # NEW: Scene management
│   │   ├── scene.hpp/cpp             # Scene graph
│   │   └── render_list.hpp/cpp       # Renderable collection
│   └── formats/                      # File format parsers
│       ├── w3d/                      # W3D format
│       └── map/                      # NEW: Map/terrain format
├── viewer/                           # NEW: Viewer-specific code
│   ├── viewer_application.hpp/cpp    # Main viewer app
│   ├── mesh_view.hpp/cpp             # W3D mesh view component
│   └── terrain_view.hpp/cpp          # NEW: Terrain view component
└── ui/                               # UI components
```

---

## Implementation Phases

### Phase 1: Renderable Interface & Library Foundation

**Goal**: Create abstract renderable interface and begin library extraction.

#### 1.1 Create IRenderable Interface
**New file**: `src/lib/gfx/renderable.hpp`
```cpp
namespace gfx {

class IRenderable {
public:
  virtual ~IRenderable() = default;

  // Draw the renderable
  virtual void draw(vk::CommandBuffer cmd) = 0;

  // Get bounding box for culling/raycasting
  virtual const BoundingBox& bounds() const = 0;

  // Get renderable type name (for debugging/layering)
  virtual const char* typeName() const = 0;

  // Check if renderable is valid/loaded
  virtual bool isValid() const = 0;
};

} // namespace gfx
```

#### 1.2 Create Scene Graph
**New files**: `src/lib/scene/scene.hpp/cpp`
- Manages collection of `IRenderable` objects
- Provides `addRenderable()`, `removeRenderable()`
- Implements frustum culling (optional)
- Returns renderable list for Renderer

#### 1.3 Refactor Renderer
**Modify**: `src/core/renderer.hpp/cpp` → `src/lib/gfx/renderer.hpp/cpp`

Current signature:
```cpp
void recordCommandBuffer(vk::CommandBuffer cmd, uint32_t imageIndex, const FrameContext &ctx);
```

New signature:
```cpp
void recordCommandBuffer(vk::CommandBuffer cmd, uint32_t imageIndex, const Scene &scene);
```

Changes:
- Remove W3D-specific `FrameContext`
- Remove hardcoded mesh/skeleton rendering
- Accept generic `Scene` with renderables
- Each renderable handles its own drawing

#### 1.4 Move Core Components to lib/
Move these files from `src/core/` to `src/lib/gfx/`:
- `vulkan_context.hpp/cpp`
- `pipeline.hpp/cpp`
- `buffer.hpp/cpp`
- `texture.hpp/cpp`
- `camera.hpp/cpp`
- `renderer.hpp/cpp` (refactored)

#### 1.5 Refactor HLodModel to IRenderable
**Modify**: `src/render/hlod_model.hpp/cpp` → `src/lib/formats/w3d/hlod_model.hpp/cpp`
- Inherit from `gfx::IRenderable`
- Implement `draw()`, `bounds()`, `typeName()`, `isValid()`
- Keep existing template-based drawing methods internally

**Files to modify**:
- [src/render/hlod_model.hpp](src/render/hlod_model.hpp)
- [src/render/hlod_model.cpp](src/render/hlod_model.cpp)

---

### Phase 2: Terrain Format Parsing

**Goal**: Parse Generals terrain data from .map files.

#### 2.1 Create Terrain Data Structures
**New file**: `src/lib/formats/map/terrain_types.hpp`
```cpp
namespace map {

struct HeightmapData {
  uint16_t width = 0;
  uint16_t height = 0;
  uint16_t borderSize = 0;
  std::vector<uint8_t> heights;  // 0-255 range

  // World space conversion constants
  static constexpr float MAP_XY_FACTOR = 10.0f;
  static constexpr float MAP_HEIGHT_SCALE = MAP_XY_FACTOR / 16.0f;
};

struct TileIndex {
  uint16_t baseTile;     // Primary texture tile
  uint16_t blendTile;    // Optional blend tile (0xFFFF = none)
  uint16_t cliffInfo;    // Cliff information

  bool hasBlend() const { return blendTile != 0xFFFF; }
};

struct TerrainData {
  HeightmapData heightmap;
  std::vector<TileIndex> tiles;
  std::vector<std::string> textureFiles;  // Base tile textures
};

} // namespace map
```

#### 2.2 Create Map Chunk Reader
**New file**: `src/lib/formats/map/map_chunk_reader.hpp/cpp`
- Reuse existing chunk reading patterns from `src/w3d/chunk_reader.hpp`
- Support map-specific chunks:
  - `HeightMapData` (chunk ID to be determined from legacy code)
  - `BlendTileData`
  - `WorldDict`
  - `Objects` (deferred to later phase)

#### 2.3 Create Terrain Loader
**New file**: `src/lib/formats/map/terrain_loader.hpp/cpp`
```cpp
namespace map {

class TerrainLoader {
public:
  // Load terrain from .map file
  std::optional<TerrainData> loadTerrain(const std::filesystem::path &mapPath);

  // Load heightmap only (minimal implementation)
  std::optional<HeightmapData> loadHeightmap(const std::filesystem::path &mapPath);
};

} // namespace map
```

**Reference files**:
- [legacy/GeneralsMD/Code/GameEngineDevice/Include/W3DDevice/GameClient/WorldHeightMap.h](legacy/GeneralsMD/Code/GameEngineDevice/Include/W3DDevice/GameClient/WorldHeightMap.h)

---

### Phase 3: Terrain Rendering (Minimal)

**Goal**: Render heightmap as vertex mesh with basic textures.

#### 3.1 Create Terrain Renderable
**New file**: `src/lib/formats/map/terrain_renderable.hpp/cpp`
```cpp
namespace map {

class TerrainRenderable : public gfx::IRenderable {
public:
  // Load terrain from parsed data
  bool load(gfx::VulkanContext &context, const TerrainData &data);

  // IRenderable implementation
  void draw(vk::CommandBuffer cmd) override;
  const BoundingBox& bounds() const override;
  const char* typeName() const override { return "Terrain"; }
  bool isValid() const override { return vertexBuffer_.valid(); }

private:
  VertexBuffer<Vertex> vertexBuffer_;
  IndexBuffer indexBuffer_;
  BoundingBox bounds_;

  // Texture handling (basic - one texture for now)
  std::string textureName_;
};

} // namespace map
```

#### 3.2 Heightmap to Mesh Conversion
- Convert heightmap grid to vertex positions
- Calculate UV coordinates from tile indices
- Generate triangle indices (grid-based)
- Compute bounding box

**Algorithm**:
```
For each cell (x, y):
  vertex.x = x * MAP_XY_FACTOR
  vertex.y = heightmap[y * width + x] * MAP_HEIGHT_SCALE
  vertex.z = y * MAP_XY_FACTOR
  vertex.uv = tileIndexToUV(x, y)
```

#### 3.3 Terrain Shader
**New files**: `shaders/terrain.vert.spv`, `shaders/terrain.frag.spv`
- Vertex shader: standard vertex transform
- Fragment shader: texture sampling (reuse existing material push constants)

---

### Phase 4: Map File Support with Objects

**Goal**: Load complete .map files with terrain and objects.

#### 4.1 Map Object Parsing
**New file**: `src/lib/formats/map/map_object.hpp`
```cpp
namespace map {

struct MapObject {
  std::string name;
  std::string thingTemplate;  // Object type
  glm::vec3 position;
  float angle = 0.0f;
  std::unordered_map<std::string, std::string> properties;
};

struct MapData {
  TerrainData terrain;
  std::vector<MapObject> objects;
};

} // namespace map
```

#### 4.2 Map Loader
**New file**: `src/lib/formats/map/map_loader.hpp/cpp`
- Parse all map chunks
- Return complete `MapData` structure
- Handle object list parsing

**Reference files**:
- [legacy/GeneralsMD/Code/GameEngine/Include/Common/MapObject.h](legacy/GeneralsMD/Code/GameEngine/Include/Common/MapObject.h)

#### 4.3 Object Instantiation
- Create `HLodModel` instances for each map object
- Load referenced W3D files
- Position objects using transform data
- Add all to scene graph

---

### Phase 5: Viewer Application Refactoring

**Goal**: Restructure viewer application to use the new library.

#### 5.1 Create ViewerApplication
**New file**: `src/viewer/viewer_application.hpp/cpp`
- Inherits from or uses library components
- Manages window, UI, and user input
- Uses `Scene` to manage renderables
- Handles file loading for both W3D and map files

#### 5.2 Update main.cpp
**Modify**: [src/main.cpp](src/main.cpp)
- Simplified entry point
- Creates `ViewerApplication`
- Delegates all logic

#### 5.3 UI Updates
- Add terrain controls to UI panels
- Add map file browser support
- Display terrain info (heightmap dimensions, tile count)

---

## Critical Files to Modify

### Library Extraction (Phase 1)
| File | Action | New Location |
|------|--------|--------------|
| [src/core/vulkan_context.hpp](src/core/vulkan_context.hpp) | Move | `src/lib/gfx/vulkan_context.hpp` |
| [src/core/renderer.hpp](src/core/renderer.hpp) | Refactor + Move | `src/lib/gfx/renderer.hpp` |
| [src/render/hlod_model.hpp](src/render/hlod_model.hpp) | Implement IRenderable + Move | `src/lib/formats/w3d/hlod_model.hpp` |
| [src/core/application.hpp](src/core/application.hpp) | Refactor | `src/viewer/viewer_application.hpp` |

### Terrain Parsing (Phase 2)
| Reference File | Purpose |
|----------------|---------|
| [legacy/GeneralsMD/Code/GameEngineDevice/Include/W3DDevice/GameClient/WorldHeightMap.h](legacy/GeneralsMD/Code/GameEngineDevice/Include/W3DDevice/GameClient/WorldHeightMap.h) | Heightmap data structure |
| [legacy/GeneralsMD/Code/GameEngineDevice/Source/W3DDevice/GameClient/WorldHeightMap.cpp](legacy/GeneralsMD/Code/GameEngineDevice/Source/W3DDevice/GameClient/WorldHeightMap.cpp) | Chunk parsing implementation |
| [legacy/GeneralsMD/Code/GameEngine/Include/Common/MapObject.h](legacy/GeneralsMD/Code/GameEngine/Include/Common/MapObject.h) | Map object structure |

---

## Verification

### Phase 1 Verification
1. Existing W3D viewer still works after refactoring
2. `HLodModel` successfully implements `IRenderable`
3. Renderer can render any `IRenderable` without type-specific code
4. Build passes with no warnings

### Phase 2 Verification
1. Can load heightmap from .map file
2. Heightmap dimensions and data are correct
3. Unit tests for chunk parsing

### Phase 3 Verification
1. Heightmap renders as 3D mesh
2. Textures display correctly
3. Bounding box is accurate for culling

### Phase 4 Verification
1. Can load complete .map file
2. Objects appear at correct positions
3. Terrain and objects render together

### Phase 5 Verification
1. Single executable can view W3D files and .map files
2. UI shows appropriate controls for each mode
3. No regressions in existing functionality

---

## Testing Strategy

### Unit Tests
- Chunk parsing tests (`map_chunk_reader_test.cpp`)
- Heightmap data validation tests
- Tile index mapping tests

### Integration Tests
- Load sample .map file
- Verify terrain renders
- Verify objects load and position correctly

### Manual Testing
- Use existing map files from `lib/GeneralsGameCode/`
- Compare rendering with original game screenshots

---

## Dependencies

### Existing (Reusable)
- `src/w3d/chunk_reader.hpp` - Chunk reading patterns
- `src/core/buffer.hpp` - GPU buffer management
- `src/core/texture.hpp` - Texture loading

### New Dependencies
- None (terrain system is independent)

---

## Risk Mitigation

| Risk | Mitigation |
|------|------------|
| Breaking existing W3D viewer | Run existing tests after each phase |
| Terrain format complexity | Start with minimal implementation (heightmap only) |
| Large refactoring scope | Phase 1 creates usable library immediately |
| Map format undocumented | Reference implementation exists in legacy/ |

---

## Next Steps After Implementation

1. **Advanced terrain features**: Blend tiles, water, cliffs
2. **Performance optimization**: LOD system, frustum culling
3. **Export as library**: CMake target for external projects
4. **Documentation**: API documentation for library users
