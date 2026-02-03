# Loading Models

This guide explains how to load W3D models into the viewer.

## File Browser

The built-in file browser provides the easiest way to load models.

### Opening the File Browser

The file browser opens automatically when launching without arguments:

```bash
./VulkanW3DViewer
```

### Navigating

- **Double-click** folders to enter them
- **Single-click** a `.w3d` file to load it
- Use the **path bar** to navigate directly
- **Parent directory** button goes up one level

### Filtering

The file browser automatically filters to show:

- Directories
- `.w3d` files

Other file types are hidden for clarity.

## Command Line Loading

### Basic Loading

Load a model directly:

```bash
./VulkanW3DViewer path/to/model.w3d
```

### With Custom Textures

Specify a custom texture search path:

```bash
./VulkanW3DViewer model.w3d -t /path/to/textures
```

Or using the long form:

```bash
./VulkanW3DViewer model.w3d --textures /path/to/textures
```

!!! tip "Texture Search"
    By default, textures are searched in:

    1. Same directory as the W3D file
    2. Custom path specified with `-t`

## Supported Model Types

### Static Meshes

Simple models without animation or bones.

- **Example**: Buildings, props, terrain pieces
- **What loads**: Geometry, materials, textures

### Skinned Meshes

Models with bone weights for skeletal animation.

- **Example**: Infantry, vehicles with moving parts
- **What loads**: Geometry, skeleton, bone weights

### Hierarchical Models (HLod)

Complex models with multiple LOD levels and sub-objects.

- **Example**: Units with multiple components
- **What loads**: All LOD levels, bone attachments, aggregates

## What Gets Loaded

When you load a W3D file, the viewer parses:

| Component | Description |
|-----------|-------------|
| **Meshes** | Geometry with vertices, normals, UVs |
| **Materials** | Shader settings, colors, properties |
| **Textures** | Referenced texture files |
| **Hierarchy** | Bone/skeleton structure |
| **Animations** | Keyframe animation data |
| **HLod** | LOD configuration and sub-objects |
| **Boxes** | Collision geometry |

## Texture Loading

### Supported Formats

| Format | Extension | Notes |
|--------|-----------|-------|
| TGA | `.tga` | Most common in C&C Generals |
| DDS | `.dds` | Compressed textures |

### Texture Search Order

1. Same directory as the W3D file
2. Custom texture path (if specified)
3. Common game texture directories

### Missing Textures

If a texture cannot be found:

- A placeholder magenta texture is used
- Warning appears in the console
- Model still loads and displays

!!! tip "Debug Texture Issues"
    Use `--debug` to see texture search paths:
    ```bash
    ./VulkanW3DViewer model.w3d --debug
    ```

## Multi-File Models

Some models span multiple files:

- **Main file**: Contains HLod and references
- **Mesh files**: Individual mesh data
- **Animation files**: Separate `.w3d` files with animations

The viewer automatically resolves references within the same directory.

## Loading Errors

### Common Issues

??? failure "File not found"
    - Verify the file path is correct
    - Check file permissions
    - Ensure the file exists

??? failure "Invalid W3D file"
    - File may be corrupted
    - File may be a different format
    - Try a different W3D file to verify viewer works

??? failure "Textures not loading"
    - Check texture path with `-t`
    - Verify texture files exist
    - Check console for missing texture names

## Large Models

For very large or complex models:

- Loading may show progress in console
- Initial frame may take longer
- LOD switching helps performance

## Next Steps

- Learn [navigation controls](navigation-controls.md)
- Explore the [user interface](user-interface.md)
