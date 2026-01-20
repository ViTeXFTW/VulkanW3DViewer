# W3D Parser Tests

# Collect W3D source files needed for testing (parser module only, no Vulkan dependencies)
set(W3D_SOURCES
  ${CMAKE_SOURCE_DIR}/src/w3d/loader.cpp
  ${CMAKE_SOURCE_DIR}/src/w3d/mesh_parser.cpp
  ${CMAKE_SOURCE_DIR}/src/w3d/hierarchy_parser.cpp
  ${CMAKE_SOURCE_DIR}/src/w3d/animation_parser.cpp
  ${CMAKE_SOURCE_DIR}/src/w3d/hlod_parser.cpp
)

add_executable(w3d_tests
  w3d/test_chunk_reader.cpp
  w3d/test_mesh_parser.cpp
  w3d/test_loader.cpp
  ${W3D_SOURCES}
)

target_link_libraries(w3d_tests PRIVATE gtest gtest_main)

target_include_directories(w3d_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/src
)

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
  W3D_TEST_FIXTURES_DIR="${CMAKE_SOURCE_DIR}/legacy/models/Art/W3D"
)
