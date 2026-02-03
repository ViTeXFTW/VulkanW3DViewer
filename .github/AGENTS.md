## Documentation

Full documentation is available at [vitexftw.github.io/VulkanW3DViewer](https://vitexftw.github.io/VulkanW3DViewer/)

## Dev environment tips
The project is an updated implementation of viewing the WestWood W3D format for old Command & Conquer series.
The project aims to update the old codebase to newer supported frameworks.

- If the legacy code is in the codebase, it will be located under `PROJECT_ROOT/legacy/`.
- Use `cmake --preset $preset` to configure.
- Use `cmake --build --preset $preset` to build.
- Use `build/$preset/VulkanW3DViewer(.exe) $modelPath` to load a model on application startup.
- Use `build/$preset/VulkanW3DViewer(.exe) -d` for debug.
- Use `build/$preset/VulkanW3DViewer(.exe) -h` to get a list of the commandline arguments.

## W3D Format Quirks

Unintuitive behaviors in the W3D format that require special handling:

- **UV V-Flip**: V-coordinate is inverted (`v = 1.0 - v`) during parsing. W3D uses V=0 at bottom, Vulkan uses V=0 at top. See `src/w3d/mesh_parser.cpp`.
- **Quaternion Order**: W3D stores (x,y,z,w), GLM expects (w,x,y,z). Reorder in `src/render/skeleton.cpp`.
- **Root Bone Sentinel**: Parent index `0xFFFFFFFF` indicates root bone (no parent). See `src/render/skeleton.cpp`.
- **Container Bit**: High bit (`0x80000000`) of chunk size field indicates container chunk. Mask off to get actual size. See `src/w3d/chunk_reader.hpp`.
- **Per-Face UV Indices**: Some meshes use per-face UV indices, requiring mesh "unrolling" (vertex duplication). See `src/render/mesh_converter.cpp`.


## Testing instructions
- Where it make sense create unit tests for new code or updated existing code.
- Attempt to keep test coverage high, while not adding graphic dependencies.
- To compile test run the `test` preset.
- Use `ctest --preset test` to run the tests.

## PR instructions
Uses the conventional commit format with the default types.
- Title format: `<type>: <description>`
- Run Clang-format and run the application before committing. 
