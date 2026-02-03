# Quick Start

This guide will get you viewing W3D models in minutes.

## Running the Viewer

After [building](building.md) the project, launch the viewer:

=== "Windows"

    ```powershell
    .\build\release\VulkanW3DViewer.exe
    ```

=== "Linux/macOS"

    ```bash
    ./build/release/VulkanW3DViewer
    ```

## Loading a Model

### Using the GUI

1. Launch the viewer without arguments
2. The **File Browser** panel opens automatically
3. Navigate to your W3D files
4. Click a `.w3d` file to load it

### Using Command Line

Load a model directly:

```bash
./VulkanW3DViewer path/to/model.w3d
```

With custom texture path:

```bash
./VulkanW3DViewer model.w3d -t /path/to/textures
```

## Basic Navigation

Once a model is loaded:

| Action | Control |
|--------|---------|
| Rotate camera | Left mouse drag |
| Zoom in/out | Mouse scroll wheel |
| Pan camera | Middle mouse drag |

## Understanding the Interface

The viewer has several UI panels:

### Viewport Window

The main 3D view showing your model. This is where you interact with the camera.

### Model Info Panel

Displays information about the loaded model:

- Mesh count and names
- Vertex/triangle counts
- Texture information
- Bone hierarchy

### Animation Panel

If the model has animations:

- Play/pause animation
- Adjust playback speed
- Select different animations
- Scrub through timeline

### LOD Panel

For models with multiple LOD levels:

- View available LOD levels
- Force specific LOD level
- See LOD switching thresholds

### Console Window

Shows debug output and loading messages. Useful for troubleshooting.

## Example Workflow

1. **Launch the viewer**
   ```bash
   ./VulkanW3DViewer
   ```

2. **Open a model** using the file browser

3. **Explore the model**
   - Rotate with left mouse drag
   - Zoom with scroll wheel
   - Check the Model Info panel for details

4. **Play animations** (if available)
   - Open the Animation panel
   - Select an animation from the dropdown
   - Click Play

5. **Inspect LOD levels**
   - Open the LOD panel
   - Toggle between LOD levels

## Obtaining W3D Files

W3D files come from **Command & Conquer: Generals** and **Zero Hour**:

1. Locate your game installation directory
2. Look in the `Data` folder
3. W3D files may be inside `.big` archives
4. Use a BIG extractor tool to access individual files

!!! warning "Legal Notice"
    Ensure you own a legitimate copy of the game. W3D files are copyrighted by Electronic Arts.

## Debug Mode

Enable verbose output for troubleshooting:

```bash
./VulkanW3DViewer model.w3d --debug
```

This prints detailed information about:

- File parsing progress
- Chunk types encountered
- Texture loading status
- Any warnings or errors

## Next Steps

- Learn more about [navigation controls](../user-guide/navigation-controls.md)
- Explore the [user interface](../user-guide/user-interface.md)
- Understand the [W3D format](../w3d-format/index.md)
