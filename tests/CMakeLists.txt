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
