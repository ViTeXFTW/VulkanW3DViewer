# Navigation Controls

This guide covers camera controls and viewport interaction.

## Camera System

VulkanW3DViewer uses an **orbital camera** that rotates around a focal point. This is ideal for inspecting 3D models from all angles.

## Mouse Controls

### Rotation

**Left Mouse Button + Drag**

- Rotates the camera around the model
- Horizontal drag: Orbit left/right (azimuth)
- Vertical drag: Orbit up/down (elevation)

!!! tip "Smooth Rotation"
    Slow, steady movements give the best control. The camera rotation speed is calibrated for precise inspection.

### Zoom

**Scroll Wheel**

- Scroll up: Zoom in (move closer)
- Scroll down: Zoom out (move away)

The zoom operates by adjusting the camera distance from the focal point.

### Pan

**Middle Mouse Button + Drag**

- Moves the camera and focal point together
- Useful for centering on specific parts of a model

## Camera Behavior

### Auto-Framing

When a model loads, the camera automatically frames it:

- Calculates model bounding box
- Positions camera to show entire model
- Sets appropriate zoom distance

### Orbit Constraints

The orbital camera has some constraints:

- **Elevation**: Limited to prevent flipping
- **Distance**: Minimum and maximum zoom limits
- **Smooth interpolation**: Gradual camera movements

## Viewport Interaction

### Model Hovering

When you hover over mesh geometry:

- The hovered mesh is highlighted
- Tooltip shows mesh name
- Useful for identifying sub-objects

### Viewport Focus

Click in the viewport to give it focus. This ensures:

- Mouse controls are captured
- Keyboard shortcuts work
- UI panels don't intercept input

## Camera Settings Panel

Access camera settings through the UI:

| Setting | Description |
|---------|-------------|
| **Field of View** | Perspective FOV angle |
| **Near Plane** | Near clipping distance |
| **Far Plane** | Far clipping distance |
| **Rotation Speed** | Mouse rotation sensitivity |
| **Zoom Speed** | Scroll wheel sensitivity |

## Tips for Model Inspection

### Viewing Details

1. Zoom in close to the area of interest
2. Use slow rotation movements
3. Pan to recenter if needed

### Viewing Overall Shape

1. Zoom out for full model view
2. Rotate to see silhouette
3. Good for checking LOD levels

### Inspecting Bones

1. Enable skeleton visualization
2. Zoom to bone area
3. Play animation slowly to see bone movement

### Checking UV/Textures

1. Position camera perpendicular to surface
2. Zoom to fill viewport with textured area
3. Check for texture stretching or seams

## Common Patterns

### 360° Model Review

1. Position at comfortable zoom
2. Hold left mouse button
3. Drag steadily in one direction
4. Complete full rotation

### Top-Down View

1. Rotate camera to look straight down
2. Continue dragging up until viewing from above
3. Useful for checking model footprint

### Front/Side/Back Views

1. Rotate to approximate view
2. Fine-tune with small adjustments
3. Zoom to fill viewport

## Troubleshooting

### Camera Not Responding

- Click in the viewport to ensure focus
- Check if a UI panel is capturing input
- Try closing popup windows

### Zoom Too Fast/Slow

- Adjust zoom speed in Camera Settings panel
- Default sensitivity is calibrated for most mice

### Model Not Visible

- Model may have loaded off-center
- Try zooming out significantly
- Reset camera may help (if available)

### Camera Inside Model

- Happens when zooming too far in
- Zoom out until model reappears
- Pan to move focal point outside geometry
