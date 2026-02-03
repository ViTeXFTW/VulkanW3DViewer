# User Interface

This guide explains the VulkanW3DViewer interface and its components.

## Overview

The interface uses [Dear ImGui](https://github.com/ocornut/imgui) for immediate-mode GUI rendering. Windows and panels can be moved, resized, and docked.

## Main Components

```
┌─────────────────────────────────────────────────────────────┐
│  Menu Bar                                                   │
├────────────────┬────────────────────────────┬───────────────┤
│                │                            │               │
│  File Browser  │        Viewport            │   Panels      │
│                │                            │   - Model     │
│                │      (3D View)             │   - Animation │
│                │                            │   - LOD       │
│                │                            │   - Camera    │
│                │                            │   - Display   │
├────────────────┴────────────────────────────┴───────────────┤
│  Console Window                                             │
└─────────────────────────────────────────────────────────────┘
```

## Windows

### Viewport Window

The main 3D rendering area.

- **Purpose**: Display the loaded model
- **Interaction**: Camera controls (see [Navigation](navigation-controls.md))
- **Features**:
    - Mesh hover detection
    - Model rendering
    - Skeleton overlay (when enabled)

### File Browser

Navigate and load W3D files.

- **Open**: Automatically shown on startup
- **Features**:
    - Directory navigation
    - File filtering (`.w3d` only)
    - Quick load on click

### Console Window

Debug output and logging.

- **Shows**:
    - Loading progress
    - Warnings and errors
    - Debug information (with `--debug`)
- **Clear**: Right-click for context menu

## Panels

### Model Info Panel

Displays information about the currently loaded model.

| Field | Description |
|-------|-------------|
| **Name** | Model/mesh name |
| **Meshes** | Number of meshes |
| **Vertices** | Total vertex count |
| **Triangles** | Total triangle count |
| **Bones** | Skeleton bone count |
| **Textures** | Loaded texture names |

### Animation Panel

Controls for animation playback.

| Control | Function |
|---------|----------|
| **Animation** | Dropdown to select animation |
| **Play/Pause** | Toggle playback |
| **Speed** | Playback speed multiplier |
| **Frame** | Current frame number |
| **Timeline** | Scrubber for manual frame control |
| **Loop** | Toggle loop mode |

!!! tip "Slow Motion"
    Set speed to 0.1x to analyze animation details frame by frame.

### LOD Panel

Level of Detail controls.

| Control | Function |
|---------|----------|
| **Current LOD** | Active LOD level |
| **Auto** | Enable automatic LOD switching |
| **Force LOD** | Manually select LOD level |
| **Distances** | LOD switching thresholds |

### Camera Panel

Camera settings and controls.

| Setting | Description |
|---------|-------------|
| **FOV** | Field of view (degrees) |
| **Near/Far** | Clipping plane distances |
| **Position** | Current camera position (read-only) |
| **Target** | Current focal point (read-only) |

### Display Panel

Visual options.

| Option | Description |
|--------|-------------|
| **Wireframe** | Render as wireframe |
| **Skeleton** | Show bone visualization |
| **Bounding Box** | Show model bounds |
| **Normals** | Visualize vertex normals |
| **Grid** | Toggle ground grid |

## Window Management

### Moving Windows

- Click and drag the title bar
- Windows can be placed anywhere

### Resizing Windows

- Drag window edges or corners
- Minimum size is enforced

### Collapsing Windows

- Double-click title bar to collapse
- Click arrow to expand

### Closing Windows

- Click the X button (if available)
- Windows can be reopened from View menu

## Docking

Windows can be docked to create layouts:

1. **Drag** a window by its title bar
2. **Hover** over docking targets (shown as overlay)
3. **Release** to dock

### Docking Positions

- **Center**: Tab with existing window
- **Left/Right**: Split horizontally
- **Top/Bottom**: Split vertically

### Undocking

- **Drag** the tab away from the dock
- Window becomes floating again

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| ++ctrl+o++ | Open file browser |
| ++escape++ | Close popup/exit |
| ++f11++ | Toggle fullscreen |

## Themes

The interface uses a dark theme by default, optimized for extended use and matching the Vulkan aesthetic.

## Tips

### Efficient Layout

1. Dock commonly used panels together
2. Collapse panels you don't need
3. Use tabs to group related panels

### Performance

- Minimize console output in normal use
- Close unused windows
- Collapse complex panels when not needed

### Debugging

1. Open Console window
2. Enable `--debug` mode
3. Check output for warnings/errors
