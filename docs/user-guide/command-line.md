# Command Line Interface

This guide covers all command line options for VulkanW3DViewer.

## Usage

```bash
VulkanW3DViewer [model] [OPTIONS]
```

## Arguments

### model

Optional positional argument specifying a W3D file to load on startup.

```bash
./VulkanW3DViewer path/to/model.w3d
```

If not provided, the viewer starts with the file browser open.

## Options

### -h, --help

Display help message and exit.

```bash
./VulkanW3DViewer --help
```

Output:
```
W3D Viewer - A Vulkan-based viewer for W3D 3D model files from Command & Conquer Generals
Usage: VulkanW3DViewer [model] [OPTIONS]

Arguments:
  model                    W3D model file to load on startup

Options:
  -h,--help               Display help message and exit
  -t,--textures PATH      Set custom texture search path
  -d,--debug              Enable verbose debug output
```

### -t, --textures PATH

Set a custom texture search path.

```bash
./VulkanW3DViewer model.w3d -t /path/to/textures
./VulkanW3DViewer model.w3d --textures /path/to/textures
```

**Purpose**: Specify where to look for texture files referenced by the W3D model.

**Validation**: Path must be an existing directory.

**Search Order**:

1. Same directory as the W3D file
2. Custom texture path (this option)

### -d, --debug

Enable verbose debug output.

```bash
./VulkanW3DViewer model.w3d --debug
./VulkanW3DViewer model.w3d -d
```

**Output includes**:

- W3D chunk parsing details
- Texture loading status
- Vulkan resource creation
- Frame timing information

## Examples

### Basic Launch

Open the viewer with the file browser:

```bash
./VulkanW3DViewer
```

### Load Specific Model

Open a model directly:

```bash
./VulkanW3DViewer soldier.w3d
```

### Load with Texture Path

Specify custom texture location:

```bash
./VulkanW3DViewer vehicle.w3d -t /games/generals/textures
```

### Debug Mode

Enable verbose output for troubleshooting:

```bash
./VulkanW3DViewer broken_model.w3d --debug
```

### Full Example

Combine all options:

```bash
./VulkanW3DViewer tank.w3d --textures /data/textures --debug
```

## Exit Codes

| Code | Meaning |
|------|---------|
| 0 | Success |
| 1 | Error (invalid arguments, file not found, etc.) |

## Environment Variables

### VULKAN_SDK

Required for shader compilation. Set automatically by Vulkan SDK installer.

```bash
export VULKAN_SDK=/path/to/vulkansdk
```

## Path Handling

### Relative Paths

Relative paths are resolved from the current working directory:

```bash
./VulkanW3DViewer ./models/unit.w3d
```

### Absolute Paths

Absolute paths work as expected:

```bash
./VulkanW3DViewer /home/user/models/unit.w3d
```

### Paths with Spaces

Quote paths containing spaces:

```bash
./VulkanW3DViewer "path/with spaces/model.w3d"
```

## Shell Integration

### Bash Completion

Tab completion works for file paths:

```bash
./VulkanW3DViewer mod<TAB>  # Completes to model.w3d
```

### File Associations

Associate `.w3d` files with the viewer:

=== "Linux"

    Create a `.desktop` file or use `xdg-mime`:
    ```bash
    xdg-mime default vulkanw3dviewer.desktop application/x-w3d
    ```

=== "Windows"

    Right-click a `.w3d` file → Open With → Choose VulkanW3DViewer.exe

## Scripting

### Batch Processing

Process multiple files (basic example):

```bash
for file in models/*.w3d; do
    ./VulkanW3DViewer "$file" --debug 2>&1 | grep "Error"
done
```

!!! note "Viewer is Interactive"
    The viewer is primarily interactive. For batch processing, you may need to add support for headless mode or automated screenshot capture.

## Troubleshooting

### "File not found"

- Check the file path is correct
- Verify file exists: `ls -la path/to/model.w3d`
- Use absolute path to eliminate ambiguity

### "Invalid texture path"

- Path must be a directory, not a file
- Directory must exist
- Check permissions

### No Output

- Add `--debug` flag for verbose output
- Check stderr: `./VulkanW3DViewer model.w3d 2>&1`
