# Phase 4.3: Object Instantiation - Scoping Analysis

## Executive Summary

This document scopes the work required to implement object instantiation for map files, transforming parsed `MapObject` data into rendered `HLodModel` instances positioned on the terrain.

**Resolution**: INI file parser required. The game's `Object/*.ini` files will be available in the resource directory alongside the application, providing the `thingTemplate` → W3D model mapping.

### Resource Directory Structure (Confirmed)

```
<app_directory>/
├── VulkanW3DViewer.exe
├── resources/
│   ├── Art/
│   │   ├── W3D/           # W3D model files (*.w3d)
│   │   └── Textures/      # Texture files (*.tga, *.dds)
│   └── Data/
│       └── INI/
│           └── Object/    # Object definition files (*.ini)
│               ├── AmericaVehicle.ini
│               ├── ChinaVehicle.ini
│               ├── CivilianUnit.ini
│               └── ...
└── maps/                  # Map files (*.map)
```

---

## Current State Analysis

### What Exists

| Component | Status | Location |
|-----------|--------|----------|
| MapObject parsing | ✅ Complete | `terrain_loader.cpp:461-542` |
| MapObject structure | ✅ Complete | `terrain_types.hpp:124-142` |
| HLodModel rendering | ✅ Complete | `hlod_model.hpp/cpp` |
| Scene container | ✅ Complete | `scene.hpp/cpp` |
| W3D file loading | ✅ Complete | `loader.hpp/cpp`, `model_loader.hpp/cpp` |
| Terrain rendering | ✅ Complete | `terrain_renderable.hpp/cpp` |

### What's Missing

| Component | Status | Complexity |
|-----------|--------|------------|
| Template → W3D mapping | ❌ Not started | **High** - requires design decision |
| INI file parser | ❌ Not started | Medium |
| Object instance management | ❌ Not started | Medium |
| Transform application | ❌ Not started | Low |
| Scene integration for maps | ❌ Not started | Medium |
| Map camera controls | ❌ Not started | Low |

---

## Architecture Analysis

### HLodModel Construction Requirements

From `hlod_model.hpp:82`:
```cpp
void load(gfx::VulkanContext &context, const W3DFile &file, const SkeletonPose *pose);
```

Each instantiated object needs:
1. **VulkanContext** - shared across all objects
2. **W3DFile** - loaded from filesystem path
3. **SkeletonPose** (optional) - for animated objects

### Scene Architecture

From `scene.hpp`:
- Simple vector of `IRenderable*` pointers
- Scene does **not** own renderables (caller manages lifetime)
- No spatial partitioning or culling built-in

### MapObject Data Available

From `terrain_types.hpp:124-142`:
```cpp
struct MapObject {
  std::string name;
  std::string thingTemplate;  // e.g., "AmericaVehicleDozer"
  struct { float x, y, z; } position;
  float angle;  // degrees
  int32_t flags;
  std::vector<std::pair<std::string, std::string>> properties;  // currently not fully parsed
};
```

---

## INI File Format Specification

The game's INI files define object templates with their associated W3D models. Understanding this format is critical for template resolution.

### Object Block Structure

```ini
Object <TemplateName>
  ; Object properties (Scale, Geometry, etc.)

  Draw = W3DModelDraw ModuleTag_XX
    DefaultConditionState
      Model = <ModelName>           ; W3D file without extension
      Animation = <AnimationName>
      AnimationMode = LOOP
    End
    ConditionState = REALLYDAMAGED
      Model = <ModelName_D>         ; Damaged model variant
    End
  End

  ; Other modules (Body, Behavior, etc.)
End
```

### Real Examples (from CivilianUnit.ini)

**Vehicle Example:**
```ini
Object MilitiaTank
  Draw = W3DTankDraw ModuleTag_01
    DefaultConditionState
      Model               = CVTank      ; → Art/W3D/CVTank.w3d
      Turret              = Turret01
    End
    ConditionState       = REALLYDAMAGED
      Model              = CVTank_D     ; → Art/W3D/CVTank_D.w3d
    End
  End
  Geometry               = BOX
  GeometryMajorRadius    = 15.0
  GeometryMinorRadius    = 10.0
  GeometryHeight         = 10.0
End
```

**Infantry Example:**
```ini
Object MogadishuMaleCivilian01
  Draw = W3DModelDraw ModuleTag_01
    DefaultConditionState
      Model = CIUMC01_SKN               ; → Art/W3D/CIUMC01_SKN.w3d
      IdleAnimation = CIUMC01_SKL.CIUMC01_STN 0 30
    End
  End
  Geometry = CYLINDER
  GeometryMajorRadius = 3.0
  GeometryHeight = 12.0
End
```

**Car Example:**
```ini
Object CarLuxury01
  Draw = W3DModelDraw ModuleTag_01
    ConditionState = NONE
      Model = CVBMW                     ; → Art/W3D/CVBMW.w3d
    End
  End
End
```

### Key Parsing Requirements

1. **Object block**: `Object <name>` ... `End` - extract template name
2. **Draw module**: `Draw = W3D*` ... `End` - multiple draw types exist:
   - `W3DModelDraw` - standard models
   - `W3DTankDraw` - tanks with turrets
   - `W3DTruckDraw` - trucks
   - `W3DOverlordTankDraw` - special tanks
3. **Model reference**: `Model = <name>` - the W3D filename without `.w3d` extension
4. **Condition states**: Extract model from `DefaultConditionState` or `ConditionState = NONE`
5. **Comments**: Lines starting with `;` are comments

### Model Path Resolution

```
Model = CVTank  →  <resources>/Art/W3D/CVTank.w3d
```

Some objects have **no Draw module** (waypoints, triggers) - these should be skipped.

---

## Key Design Decisions

### 1. Template Resolution Strategy

**Decision**: INI file parsing (user confirmed INI files will be available)

Implementation approach:
```cpp
class IniParser {
public:
  // Parse all INI files in directory
  void loadDirectory(const std::filesystem::path& iniDir);

  // Get model name for template (returns empty if not found)
  std::optional<std::string> getModelName(const std::string& templateName) const;

private:
  std::unordered_map<std::string, ObjectTemplate> templates_;
};

struct ObjectTemplate {
  std::string name;
  std::string modelName;       // From Model = line
  std::string drawType;        // W3DModelDraw, W3DTankDraw, etc.
  // Future: damaged models, animations, geometry
};
```

### 2. Object Lifetime Management

**Decision**: Model Pool with Instance References (required for scalability)

Since the goal is to support "as many objects as possible", we need GPU resource sharing:

```cpp
// Prototype: Shared W3D model data (loaded once per unique model)
struct ModelPrototype {
  std::string modelName;
  std::unique_ptr<HLodModel> model;        // GPU resources
  gfx::BoundingBox bounds;
};

// Instance: Per-object transform (lightweight)
struct ObjectInstance {
  ModelPrototype* prototype;               // Shared model
  glm::mat4 transform;                     // Position + rotation
  const MapObject* source;                 // Reference to parsed data
};

class MapScene {
  TerrainRenderable terrain_;
  std::unordered_map<std::string, ModelPrototype> prototypes_;  // Keyed by model name
  std::vector<ObjectInstance> instances_;
};
```

**Benefits**:
- 100 tanks sharing same W3D = 1 GPU upload, 100 transforms
- Memory scales with unique model count, not instance count
- Transform updates don't require GPU re-upload

### 3. GPU Resource Strategy

**Decision**: Prototype caching + frustum culling

For "as many as possible" performance:

1. **Prototype Caching**: Load each unique W3D model once
2. **Frustum Culling**: Skip drawing off-screen instances
3. **Instance Batching** (future): Draw multiple instances in single draw call

```cpp
void MapScene::render(const Camera& camera) {
  terrain_.draw();

  for (const auto& instance : instances_) {
    // Simple frustum check against transformed bounds
    if (!camera.isVisible(instance.getBounds())) continue;

    // Set per-instance transform, draw shared model
    pushConstants.modelMatrix = instance.transform;
    instance.prototype->model->draw();
  }
}
```

**Memory Estimate** (typical map):
- 50 unique model types × ~2MB avg = ~100MB GPU memory
- 500 instances × 64 bytes transform = ~32KB CPU memory
- Acceptable for modern hardware

---

## Transform Application

### Coordinate System Analysis

From `terrain_types.hpp:10-11`:
```cpp
constexpr float MAP_XY_FACTOR = 10.0f;
constexpr float MAP_HEIGHT_SCALE = MAP_XY_FACTOR / 16.0f;
```

Map positions are in map units. Conversion to world space:
```cpp
worldX = mapX * MAP_XY_FACTOR;  // Not needed - already world space
worldY = mapY * MAP_XY_FACTOR;  // Height
worldZ = mapZ * MAP_XY_FACTOR;  // Not needed - already world space
```

**Note**: Looking at `terrain_loader.cpp:475-477`, positions are read directly as floats, suggesting they're already in world space (the terrain mesh conversion applies `MAP_XY_FACTOR`).

### Rotation Application

From `MapObject`:
- `angle` is in degrees
- Rotation is around Y-axis (vertical, Generals uses Y-up)

Transform matrix:
```cpp
glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);
transform = glm::rotate(transform, glm::radians(angle), glm::vec3(0, 1, 0));
```

**Current HLodModel limitation**: No per-instance transform support. Each `HLodModel` draws at origin. Need to add model matrix push constant or modify rendering.

---

## Implementation Plan

### Scope

1. **INI Parser** - Parse Object/*.ini files to build template registry
2. **MapScene** class - Container for terrain + object instances with prototype caching
3. **Transform support** - Per-object model matrix in renderer
4. **Frustum culling** - Skip off-screen objects
5. **Map camera** - Orbit around terrain center

### Files to Create

| File | Purpose |
|------|---------|
| `src/lib/formats/ini/ini_parser.hpp/cpp` | Generic INI file tokenizer |
| `src/lib/formats/ini/object_ini_parser.hpp/cpp` | Object.ini specific parsing |
| `src/lib/formats/ini/types.hpp` | ObjectTemplate struct definition |
| `src/lib/formats/map/map_scene.hpp/cpp` | Terrain + objects container |
| `tests/ini/test_ini_parser.cpp` | INI parser unit tests |
| `tests/ini/test_object_ini_parser.cpp` | Object template parsing tests |

### Files to Modify

| File | Change |
|------|--------|
| `src/core/renderer.hpp/cpp` | Add per-instance transform push constant |
| `src/core/application.hpp/cpp` | Add map loading mode, resource path config |
| `src/lib/gfx/camera.hpp/cpp` | Add frustum culling support |
| `src/render/hlod_model.hpp/cpp` | Add external transform support |

### Estimated Complexity

| Component | Effort | Risk |
|-----------|--------|------|
| INI Parser (tokenizer) | 1.5 days | Low - well-defined format |
| Object INI Parser | 1.5 days | Medium - nested blocks |
| MapScene + Prototypes | 2 days | Medium - GPU resource management |
| Renderer transform support | 1 day | Low - push constant exists |
| Frustum culling | 1 day | Low - AABB test |
| Map camera controls | 0.5 days | Low |
| Integration + Testing | 1.5 days | Medium |
| **Total** | **9 days** | **Medium** |

---

## Blocking Issues & Dependencies

### Resolved

1. ~~**No template mapping data**~~ → INI files will be provided in resources directory
2. ~~**No W3D asset files**~~ → W3D files will be in resources/Art/W3D/

### Remaining Dependencies

1. **Properties not fully parsed**
   - `terrain_loader.cpp:499-536` reads but skips property values
   - May need parsing for scale, team color later
   - **Status**: Defer until specific property needed

2. **Scene not used in current rendering**
   - Application renders `HLodModel` directly
   - **Status**: MapScene will handle its own rendering

3. **HLodModel lacks external transform**
   - Currently renders at origin
   - **Status**: Add transform push constant in Phase 4.3.3

---

## Implementation Phases

### Phase 4.3.1: INI Parser Foundation
**Goal**: Parse Object/*.ini files into template registry

1. Create generic INI tokenizer
   - Handle `Object ... End` blocks
   - Handle `Draw = ... End` nested blocks
   - Handle `Key = Value` pairs
   - Handle `;` comments
   - Handle whitespace-insensitive matching

2. Create ObjectIniParser
   - Extract `Object <name>` template names
   - Find `Draw = W3D*` blocks
   - Extract `Model = <name>` from condition states
   - Build template → model name map

3. Tests
   - Unit tests with INI snippets
   - Integration test loading real INI file

**Deliverable**: `IniParser::getModelName("AmericaVehicleDozer")` → `"AVDozer"`

### Phase 4.3.2: MapScene with Prototype Caching
**Goal**: Load and manage object prototypes

1. Create MapScene class
   - Hold TerrainRenderable
   - Hold prototype cache (model name → HLodModel)
   - Hold instance list (transform + prototype ref)

2. Implement prototype loading
   - Given model name, find W3D file
   - Load W3D into HLodModel
   - Cache by model name

3. Implement instance creation
   - For each MapObject, lookup template → model
   - Load prototype if not cached
   - Create instance with transform

**Deliverable**: `MapScene::loadFromMap(mapData, iniParser, resourcePath)`

### Phase 4.3.3: Renderer Transform Support
**Goal**: Render objects at their map positions

1. Verify push constant layout
   - Check if model matrix already in push constants
   - Add if missing

2. Modify HLodModel rendering
   - Accept external transform matrix
   - Apply before drawing

3. Test single object at position
   - Render one object at known coordinates
   - Verify position/rotation correct

**Deliverable**: Objects render at correct map positions

### Phase 4.3.4: Performance - Frustum Culling
**Goal**: Efficiently handle 500+ objects

1. Add frustum extraction to Camera
   - Extract 6 frustum planes from VP matrix
   - AABB-frustum intersection test

2. Apply culling in MapScene
   - Transform instance bounds by model matrix
   - Skip drawing if outside frustum

3. Benchmark
   - Measure FPS with/without culling
   - Log objects drawn vs total

**Deliverable**: Smooth rendering with 500+ objects

### Phase 4.3.5: Integration & Polish
**Goal**: Complete map viewing experience

1. Map loading UI
   - Add "Load Map" option to file browser
   - Show loading progress (INI parsing, model loading)

2. Map camera mode
   - Orbit around terrain center
   - Zoom to fit terrain bounds
   - Click object to focus

3. Debug UI
   - Show object count
   - Show prototype count (unique models)
   - Show memory usage estimate
   - Toggle object visibility

**Deliverable**: Complete map viewer with objects

---

## Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| INI format variations | Medium | Medium | Test with multiple INI files, handle edge cases |
| Missing W3D files | Medium | Low | Log warning, skip instance, continue loading |
| Coordinate system mismatch | Medium | Medium | Test with single object first, verify against game |
| Performance with 500+ objects | Medium | Medium | Frustum culling in Phase 4.3.4 |
| Complex INI nesting | Low | Medium | State machine parser handles arbitrary depth |
| Properties affect rendering | Low | Low | Defer; most properties are gameplay, not visual |

---

## Resolved Questions

| Question | Answer |
|----------|--------|
| Asset Location | Resources directory alongside application |
| Template Mapping | INI files from original game provided |
| Target Map | Start simple, gradually increase complexity |
| Performance Requirements | As many objects as possible |

---

## Appendix A: INI Parser State Machine

The INI format uses nested blocks requiring a state machine:

```
State: ROOT
  "Object <name>" → push OBJECT state
  ";" → skip comment

State: OBJECT
  "Draw = W3D*" → push DRAW state
  "End" → pop to ROOT, save template
  other "Key = Value" → store property

State: DRAW
  "DefaultConditionState" or "ConditionState = *" → push CONDITION state
  "End" → pop to OBJECT
  other → ignore

State: CONDITION
  "Model = <name>" → store model name (if first)
  "End" → pop to DRAW
  other → ignore
```

### Parsing Priority

When multiple condition states exist, prefer in order:
1. `DefaultConditionState`
2. `ConditionState = NONE`
3. First `ConditionState` found

---

## Appendix B: Template Name Examples

From typical Generals maps, common templates include:
- `AmericaVehicleDozer` - Construction dozer
- `AmericaTankCrusader` - Crusader tank
- `ChinaVehicleDozer` - China construction dozer
- `TechOilDerrick` - Neutral oil derrick
- `CivilianPropBuilding*` - Civilian buildings
- `CarLuxury01` - Civilian car
- `MilitiaTank` - Militia tank

### Templates Without Models (Skip)

These templates have no `Draw` module and should be filtered:
- `Waypoint` - Navigation waypoints
- `SkirmishCrate*` - Invisible crate spawners
- `Team*` - Team definitions
- `*Trigger*` - Script triggers

---

## Appendix C: References

- [EA C&C Generals Source Code](https://github.com/electronicarts/CnC_Generals_Zero_Hour) - Official W3D implementation
- [Command_And_Conquer_INI](https://github.com/FreemanZY/Command_And_Conquer_INI) - INI file examples
- [OpenSAGE](https://github.com/OpenSAGE/OpenSAGE) - Open source SAGE engine reimplementation
