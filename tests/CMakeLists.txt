# W3D Parser Tests

add_executable(w3d_tests
  w3d/test_chunk_reader.cpp
  w3d/test_mesh_parser.cpp
  w3d/test_hierarchy_parser.cpp
  w3d/test_animation_parser.cpp
  w3d/test_hlod_parser.cpp
  w3d/test_loader.cpp
)

target_link_libraries(w3d_tests PRIVATE w3d_lib gtest gtest_main)

# Compiler-specific flags (match main project)
if(MSVC)
  target_compile_options(w3d_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(w3d_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# Register with CTest
add_test(NAME w3d_tests COMMAND w3d_tests)

# Define path to test fixtures (real W3D files)
target_compile_definitions(w3d_tests PRIVATE
  W3D_TEST_FIXTURES_DIR="${CMAKE_SOURCE_DIR}/tests/resources/models"
)

# Texture loading tests (no Vulkan dependencies)
add_executable(texture_tests
  render/test_texture_loading.cpp
)

target_link_libraries(texture_tests PRIVATE w3d_lib gtest gtest_main)

if(MSVC)
  target_compile_options(texture_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(texture_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME texture_tests COMMAND texture_tests)

target_compile_definitions(texture_tests PRIVATE
  TEXTURE_TEST_FIXTURES_DIR="${CMAKE_SOURCE_DIR}/tests/resources/textures"
)

# Mesh converter tests (requires GLM, no Vulkan)

add_executable(mesh_converter_tests
  render/test_mesh_converter.cpp
)

target_link_libraries(mesh_converter_tests PRIVATE w3d_lib gtest gtest_main)

# Use stubs directory first to override core/pipeline.hpp with Vulkan-free version
target_include_directories(mesh_converter_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/tests/stubs
)

if(MSVC)
  target_compile_options(mesh_converter_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(mesh_converter_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME mesh_converter_tests COMMAND mesh_converter_tests)

# Skeleton pose tests (requires GLM, no Vulkan)
add_executable(skeleton_tests
  render/test_skeleton_pose.cpp
  render/test_animation_player.cpp
)

target_link_libraries(skeleton_tests PRIVATE w3d_lib gtest gtest_main)

if(MSVC)
  target_compile_options(skeleton_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(skeleton_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME skeleton_tests COMMAND skeleton_tests)

# Bounding box tests (requires GLM, no Vulkan)
add_executable(bounding_box_tests
  render/test_bounding_box.cpp
)

target_link_libraries(bounding_box_tests PRIVATE w3d_lib gtest gtest_main)

if(MSVC)
  target_compile_options(bounding_box_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(bounding_box_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME bounding_box_tests COMMAND bounding_box_tests)

# Raycast tests (requires GLM, no Vulkan)
add_executable(raycast_tests
  render/raycast_test.cpp
)

target_link_libraries(raycast_tests PRIVATE w3d_lib gtest gtest_main)

if(MSVC)
  target_compile_options(raycast_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(raycast_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME raycast_tests COMMAND raycast_tests)

# HLod hover tests (requires GLM, no Vulkan)
add_executable(hlod_hover_tests
  render/test_hlod_hover.cpp
)

target_link_libraries(hlod_hover_tests PRIVATE w3d_lib gtest gtest_main)

# Use stubs directory first to override Vulkan-dependent headers
target_include_directories(hlod_hover_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/tests/stubs
)

if(MSVC)
  target_compile_options(hlod_hover_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(hlod_hover_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME hlod_hover_tests COMMAND hlod_hover_tests)

# File browser tests
# Note: This test uses UI code which is NOT in w3d_lib, so it still needs direct source inclusion
add_executable(file_browser_tests
  ui/test_file_browser.cpp
  ${CMAKE_SOURCE_DIR}/src/ui/file_browser.cpp
)

target_link_libraries(file_browser_tests PRIVATE gtest gtest_main)

# Use stubs directory first to override ImGui with stub for testing
target_include_directories(file_browser_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/tests/stubs
  ${CMAKE_SOURCE_DIR}/src
)

if(MSVC)
  target_compile_options(file_browser_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(file_browser_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME file_browser_tests COMMAND file_browser_tests)

# Mesh visibility tests (lightweight state logic tests, no Vulkan)
add_executable(mesh_visibility_tests
  render/test_mesh_visibility.cpp
)

target_link_libraries(mesh_visibility_tests PRIVATE gtest gtest_main)

target_include_directories(mesh_visibility_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/src
)

if(MSVC)
  target_compile_options(mesh_visibility_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(mesh_visibility_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME mesh_visibility_tests COMMAND mesh_visibility_tests)

# DynamicBuffer tests (lightweight logic tests, no Vulkan initialization)
add_executable(dynamic_buffer_tests
  gfx/test_dynamic_buffer.cpp
)

target_link_libraries(dynamic_buffer_tests PRIVATE gtest gtest_main)

target_include_directories(dynamic_buffer_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/src
)

if(MSVC)
  target_compile_options(dynamic_buffer_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(dynamic_buffer_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME dynamic_buffer_tests COMMAND dynamic_buffer_tests)

# Mipmap calculation tests (pure math, no dependencies)
add_executable(mipmap_tests
  render/test_mipmap_calculation.cpp
)

target_link_libraries(mipmap_tests PRIVATE gtest gtest_main)

if(MSVC)
  target_compile_options(mipmap_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(mipmap_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME mipmap_tests COMMAND mipmap_tests)

# Pipeline create info tests (pure struct tests, no Vulkan initialization)
add_executable(pipeline_create_info_tests
  gfx/test_pipeline_create_info.cpp
)

target_link_libraries(pipeline_create_info_tests PRIVATE w3d_lib gtest gtest_main)

if(MSVC)
  target_compile_options(pipeline_create_info_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(pipeline_create_info_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME pipeline_create_info_tests COMMAND pipeline_create_info_tests)

# RTS camera tests (logic tests - needs ImGui)
add_executable(rts_camera_tests
  gfx/test_rts_camera.cpp
)

# RTS camera uses ImGui::GetIO() for input checking, so we need to link against ImGui
target_sources(rts_camera_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/lib/imgui/imgui.cpp
  ${CMAKE_SOURCE_DIR}/lib/imgui/imgui_draw.cpp
  ${CMAKE_SOURCE_DIR}/lib/imgui/imgui_tables.cpp
  ${CMAKE_SOURCE_DIR}/lib/imgui/imgui_widgets.cpp
)

target_link_libraries(rts_camera_tests PRIVATE w3d_lib gtest gtest_main)

target_include_directories(rts_camera_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/lib/imgui
)

if(MSVC)
  target_compile_options(rts_camera_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(rts_camera_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME rts_camera_tests COMMAND rts_camera_tests)

# Map file parser tests (no Vulkan dependencies)
add_executable(map_tests
  map/test_data_chunk_reader.cpp
  map/test_heightmap_parser.cpp
  map/test_blend_tile_parser.cpp
  map/test_objects_parser.cpp
  map/test_triggers_parser.cpp
  map/test_lighting_parser.cpp
  map/test_worldinfo_sideslist_parser.cpp
  map/test_map_loader.cpp
  map/test_refpack.cpp
)

target_link_libraries(map_tests PRIVATE w3d_lib gtest gtest_main)

if(MSVC)
  target_compile_options(map_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(map_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME map_tests COMMAND map_tests)

# INI parser tests (no Vulkan dependencies)
add_executable(ini_tests
  ini/test_ini_parser.cpp
  ini/test_terrain_types.cpp
  ini/test_water_settings.cpp
)

target_link_libraries(ini_tests PRIVATE w3d_lib gtest gtest_main)

if(MSVC)
  target_compile_options(ini_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(ini_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME ini_tests COMMAND ini_tests)

# Terrain mesh generation tests (requires GLM, no Vulkan)
add_executable(terrain_mesh_tests
  terrain/test_terrain_mesh.cpp
)

target_link_libraries(terrain_mesh_tests PRIVATE w3d_lib gtest gtest_main)

if(MSVC)
  target_compile_options(terrain_mesh_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(terrain_mesh_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME terrain_mesh_tests COMMAND terrain_mesh_tests)

# Frustum culling tests (requires GLM, no Vulkan)
add_executable(frustum_tests
  terrain/test_frustum.cpp
)

target_link_libraries(frustum_tests PRIVATE w3d_lib gtest gtest_main)

if(MSVC)
  target_compile_options(frustum_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(frustum_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME frustum_tests COMMAND frustum_tests)

# Terrain atlas tests (requires GLM, no Vulkan)
add_executable(terrain_atlas_tests
  terrain/test_terrain_atlas.cpp
)

target_link_libraries(terrain_atlas_tests PRIVATE w3d_lib gtest gtest_main)

if(MSVC)
  target_compile_options(terrain_atlas_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(terrain_atlas_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME terrain_atlas_tests COMMAND terrain_atlas_tests)

# Terrain blend pattern tests (requires GLM, no Vulkan)
add_executable(terrain_blend_tests
  terrain/test_terrain_blend.cpp
)

target_link_libraries(terrain_blend_tests PRIVATE w3d_lib gtest gtest_main)

if(MSVC)
  target_compile_options(terrain_blend_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(terrain_blend_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME terrain_blend_tests COMMAND terrain_blend_tests)

# Water mesh generation tests (requires GLM, no Vulkan)
add_executable(water_mesh_tests
  water/test_water_mesh.cpp
)

target_link_libraries(water_mesh_tests PRIVATE w3d_lib gtest gtest_main)

if(MSVC)
  target_compile_options(water_mesh_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(water_mesh_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME water_mesh_tests COMMAND water_mesh_tests)

# Scene graph tests (SceneNode, Quadtree, SceneGraph - no Vulkan initialization)
add_executable(scene_tests
  scene/test_scene_node.cpp
  scene/test_quadtree.cpp
  scene/test_scene_graph.cpp
)

target_link_libraries(scene_tests PRIVATE w3d_lib gtest gtest_main)

if(MSVC)
  target_compile_options(scene_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(scene_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME scene_tests COMMAND scene_tests)

# Object placement utils tests (Vulkan-free static utility functions)
add_executable(object_resolver_tests
  scene/test_object_resolver.cpp
  ${CMAKE_SOURCE_DIR}/src/render/object_placement_utils.cpp
)

target_link_libraries(object_resolver_tests PRIVATE glm::glm gtest gtest_main)

target_include_directories(object_resolver_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/lib/Vulkan-Hpp/Vulkan-Headers/include
  ${CMAKE_SOURCE_DIR}/lib/Vulkan-Hpp
)

if(MSVC)
  target_compile_options(object_resolver_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(object_resolver_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME object_resolver_tests COMMAND object_resolver_tests)

# LightingState tests (Phase 6.1/6.2/6.3 – no Vulkan dependency)
add_executable(lighting_state_tests
  render/test_lighting_state.cpp
)

target_link_libraries(lighting_state_tests PRIVATE w3d_lib gtest gtest_main)

if(MSVC)
  target_compile_options(lighting_state_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(lighting_state_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME lighting_state_tests COMMAND lighting_state_tests)

# Terrain minimap tests (Phase 6.4 – no Vulkan dependency)
add_executable(terrain_minimap_tests
  terrain/test_terrain_minimap.cpp
)

target_link_libraries(terrain_minimap_tests PRIVATE w3d_lib gtest gtest_main)

if(MSVC)
  target_compile_options(terrain_minimap_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(terrain_minimap_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME terrain_minimap_tests COMMAND terrain_minimap_tests)

# Map browser and map UI tests (Phase 7)
# UI code is NOT in w3d_lib, so source files are included directly
add_executable(map_browser_tests
  ui/test_map_browser.cpp
  ${CMAKE_SOURCE_DIR}/src/ui/map_browser.cpp
)

target_link_libraries(map_browser_tests PRIVATE w3d_lib gtest gtest_main)

target_include_directories(map_browser_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/tests/stubs
  ${CMAKE_SOURCE_DIR}/src
)

if(MSVC)
  target_compile_options(map_browser_tests PRIVATE /W3 /permissive-)
else()
  target_compile_options(map_browser_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME map_browser_tests COMMAND map_browser_tests)

# Terrain resource manager tests (Phase 1.1 - loads Terrain.ini from BIG)
add_executable(terrain_resource_manager_tests
  terrain/test_terrain_resource_manager.cpp
)

target_link_libraries(terrain_resource_manager_tests PRIVATE w3d_lib gtest gtest_main)

if(MSVC)
  target_compile_options(terrain_resource_manager_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(terrain_resource_manager_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME terrain_resource_manager_tests COMMAND terrain_resource_manager_tests)
