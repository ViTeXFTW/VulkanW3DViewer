# User Guide

Welcome to the VulkanW3DViewer user guide. This section covers everything you need to know to effectively use the viewer.

## Overview

VulkanW3DViewer is a modern 3D model viewer for the W3D format used in Command & Conquer: Generals. It provides:

- Real-time 3D visualization
- Animation playback
- LOD (Level of Detail) inspection
- Material and texture viewing
- Debug skeleton visualization

## Sections

<div class="grid cards" markdown>

-   :material-folder-open:{ .lg .middle } **Loading Models**

    ---

    How to open and load W3D files

    [:octicons-arrow-right-24: Loading Models](loading-models.md)

-   :material-rotate-3d:{ .lg .middle } **Navigation Controls**

    ---

    Camera controls and viewport interaction

    [:octicons-arrow-right-24: Navigation](navigation-controls.md)

-   :material-monitor:{ .lg .middle } **User Interface**

    ---

    Understanding the UI panels and windows

    [:octicons-arrow-right-24: Interface Guide](user-interface.md)

-   :material-console:{ .lg .middle } **Command Line**

    ---

    Command line options and batch processing

    [:octicons-arrow-right-24: CLI Reference](command-line.md)

</div>

## Quick Reference

### Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| ++escape++ | Exit application |
| ++f11++ | Toggle fullscreen |

### Mouse Controls

| Action | Control |
|--------|---------|
| Rotate | Left drag |
| Zoom | Scroll wheel |
| Pan | Middle drag |

### Supported File Types

| Extension | Description |
|-----------|-------------|
| `.w3d` | W3D model files |
| `.tga` | Texture files (TGA format) |
| `.dds` | Texture files (DDS format) |

## Features at a Glance

### Model Viewing

- Load any W3D model from Command & Conquer: Generals
- View static meshes with materials and textures
- Inspect skinned meshes with bone weights

### Animation

- Play skeletal animations
- Adjust playback speed
- Scrub through animation timeline
- Loop or single-play modes

### LOD System

- View all LOD levels
- Manual LOD selection
- Inspect LOD switching thresholds
- Compare detail levels

### Debugging

- Skeleton visualization
- Bounding box display
- Console output for diagnostics
- Mesh hover detection

## Getting Help

If you encounter issues:

1. Enable debug mode with `--debug` flag
2. Check the console window for error messages
3. Review the [troubleshooting section](../getting-started/building.md#troubleshooting)
