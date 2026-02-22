cmake_minimum_required(VERSION 3.28)
project(VulkanW3DViewer VERSION 0.3.0 LANGUAGES CXX)

# C++26 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# Export compile commands for IDE support (clangd)
# Note: This is also set in CMakePresets.json, but we set it here as a fallback
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Create symlink to compile_commands.json in project root for clangd
# This is deferred to build time since compile_commands.json doesn't exist at configure time
if(CMAKE_EXPORT_COMPILE_COMMANDS)
    add_custom_target(compile_commands_symlink ALL
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json
        COMMENT "Creating symlink to compile_commands.json in project root"
        VERBATIM
    )
endif()

# Testing with Google Test
option(BUILD_TESTING "Build tests" OFF)

# Find Vulkan (headers always from submodule, library needed for linking)
if(BUILD_TESTING)
  # For tests: Only headers needed (from Vulkan-Hpp submodule)
  # We'll create a dummy Vulkan::Vulkan target for tests
  find_package(Vulkan)
  if(NOT Vulkan_FOUND)
    # Create a header-only interface library for tests
    add_library(Vulkan::Vulkan INTERFACE IMPORTED)
    set_target_properties(Vulkan::Vulkan PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/lib/Vulkan-Hpp/Vulkan-Headers/include"
    )
    message(STATUS "Tests: Using Vulkan headers from submodule (no SDK required)")
  endif()
else()
  # For main application: Full Vulkan SDK required
  find_package(Vulkan REQUIRED)
endif()

# GLFW options - disable unnecessary builds
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# Add GLFW submodule
add_subdirectory(lib/glfw)

# Add GLM submodule
add_subdirectory(lib/glm)

# VulkanMemoryAllocator submodule
add_subdirectory(lib/VulkanMemoryAllocator)

# BigXtractor library for BIG archive support
set(BUILD_TESTING_SAVED ${BUILD_TESTING})
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(INSTALL_STANDALONE OFF CACHE BOOL "" FORCE)
add_subdirectory(lib/BigXtractor)
set(BUILD_TESTING ${BUILD_TESTING_SAVED})

# ImGui sources
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

# ============================================================================
# Shader Compilation (only needed for application, not for tests)
# ============================================================================

if(NOT BUILD_TESTING)
  # Find glslc shader compiler
  find_program(GLSLC glslc HINTS "$ENV{VULKAN_SDK}/Bin")
  if(NOT GLSLC)
      message(FATAL_ERROR "glslc not found! Make sure Vulkan SDK is installed.")
  endif()
else()
  # For tests: Create dummy glslc command (shaders pre-compiled or not needed)
  set(GLSLC "${CMAKE_COMMAND}" CACHE FILEPATH "Dummy glslc for tests" FORCE)
  message(STATUS "Tests: Shader compilation skipped (using embedded shaders)")
endif()

# Shader compilation and embedding function
function(compile_shaders target)
    file(GLOB SHADER_SOURCES "${CMAKE_SOURCE_DIR}/shaders/*.vert" "${CMAKE_SOURCE_DIR}/shaders/*.frag")
    foreach(SHADER ${SHADER_SOURCES})
        get_filename_component(SHADER_NAME "${SHADER}" NAME)
        set(SPIRV_OUTPUT "${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME}.spv")
        add_custom_command(
            OUTPUT "${SPIRV_OUTPUT}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/shaders"
            COMMAND "${GLSLC}" "${SHADER}" -o "${SPIRV_OUTPUT}"
            DEPENDS "${SHADER}"
            COMMENT "Compiling ${SHADER_NAME} to SPIR-V"
        )
        list(APPEND SPIRV_OUTPUTS "${SPIRV_OUTPUT}")
    endforeach()

    # Generate embedded shaders header
    set(EMBEDDED_HEADER "${CMAKE_BINARY_DIR}/generated/embedded_shaders.hpp")
    set(SHADER_LIST_FILE "${CMAKE_BINARY_DIR}/generated/shader_list.txt")

    # Write shader list to a file (more reliable than command-line args)
    file(WRITE "${SHADER_LIST_FILE}" "")
    foreach(SHADER_FILE ${SPIRV_OUTPUTS})
        file(APPEND "${SHADER_LIST_FILE}" "${SHADER_FILE}\n")
    endforeach()

    add_custom_command(
        OUTPUT "${EMBEDDED_HEADER}"
        COMMAND ${CMAKE_COMMAND}
            "-DSHADER_LIST_FILE=${SHADER_LIST_FILE}"
            "-DOUTPUT_FILE=${EMBEDDED_HEADER}"
            -P "${CMAKE_SOURCE_DIR}/cmake/EmbedShaders.cmake"
        DEPENDS ${SPIRV_OUTPUTS} "${CMAKE_SOURCE_DIR}/cmake/EmbedShaders.cmake"
        COMMENT "Embedding shaders into C++ header"
    )

    add_custom_target(${target}_shaders DEPENDS ${SPIRV_OUTPUTS} "${EMBEDDED_HEADER}")
    add_dependencies(${target} ${target}_shaders)
endfunction()

# ============================================================================
# w3d_lib: Reusable static library (parsing + rendering)
# ============================================================================

# Collect library source files (src/lib/ and src/render/)
file(GLOB_RECURSE LIB_SOURCES 
    "src/lib/*.cpp"
    "src/render/*.cpp"
)

# Create static library
add_library(w3d_lib STATIC ${LIB_SOURCES})

# Library include directories
target_include_directories(w3d_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/Vulkan-Hpp/Vulkan-Headers/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/Vulkan-Hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/VulkanMemoryAllocator/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/BigXtractor/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui/backends
    ${CMAKE_BINARY_DIR}/generated
)

# Library dependencies
target_link_libraries(w3d_lib PUBLIC
    Vulkan::Vulkan
    glfw
    glm::glm
    bigx::bigx
    VulkanMemoryAllocator
)

# Library compile definitions
target_compile_definitions(w3d_lib PUBLIC
    VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
    $<$<CONFIG:Debug>:W3D_DEBUG>
)

# Compiler-specific flags for library
if(MSVC)
    target_compile_options(w3d_lib PRIVATE
        /permissive-
        /utf-8
        $<$<CONFIG:Debug>:/Od /Zi /W4>
        $<$<CONFIG:Release>:/O2 /GL>
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(w3d_lib PRIVATE
        $<$<CONFIG:Debug>:-O0 -g -Wall -Wextra -Wpedantic>
        $<$<CONFIG:Release>:-O3>
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(w3d_lib PRIVATE
        $<$<CONFIG:Debug>:-O0 -g -Wall -Wextra -Wpedantic>
        $<$<CONFIG:Release>:-O3>
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Intel")
    target_compile_options(w3d_lib PRIVATE
        $<$<CONFIG:Debug>:-O0 -g -Wall -Wextra>
        $<$<CONFIG:Release>:-O3 -ipo>
    )
else()
    message(WARNING "Unknown compiler: ${CMAKE_CXX_COMPILER_ID}")
    target_compile_options(w3d_lib PRIVATE
        $<$<CONFIG:Debug>:-O0 -g>
        $<$<CONFIG:Release>:-O3>
    )
endif()

# Compile and embed shaders for the library (skip for tests - use stubs)
if(NOT BUILD_TESTING)
  compile_shaders(w3d_lib)
else()
  # For tests: Create a stub embedded_shaders.hpp that matches the real API
  file(WRITE "${CMAKE_BINARY_DIR}/generated/embedded_shaders.hpp"
    "// Stub shader header for tests - auto-generated\n"
    "#pragma once\n"
    "#include <cstddef>\n"
    "#include <cstdint>\n"
    "#include <span>\n"
    "#include <string_view>\n"
    "namespace w3d::shaders {\n"
    "  inline std::span<const uint8_t> getShader([[maybe_unused]] std::string_view name) {\n"
    "    return {}; // Tests don't need real shader data\n"
    "  }\n"
    "}\n"
  )
  message(STATUS "Tests: Using stub embedded_shaders.hpp")
endif()

# ============================================================================
# VulkanW3DViewer: Main application executable
# ============================================================================

if(NOT BUILD_TESTING)
    # Collect application source files (src/core/ and src/ui/ and src/main.cpp)
    file(GLOB_RECURSE APP_SOURCES 
        "src/core/*.cpp"
        "src/ui/*.cpp"
        "src/main.cpp"
    )
    file(GLOB_RECURSE HEADERS "include/*.hpp" "include/*.h")

    # Create executable
    add_executable(${PROJECT_NAME} ${APP_SOURCES} ${HEADERS} ${IMGUI_SOURCES})

    # Include directories
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui/backends
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/CLI11/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/json/include
    )

    # Link libraries (now includes w3d_lib)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        w3d_lib
        glfw
    )

    # Compiler-specific flags
    if(MSVC)
        # MSVC (Microsoft Visual C++)
        target_compile_options(${PROJECT_NAME} PRIVATE
            /permissive-           # Standards conformance
            /utf-8                 # UTF-8 source and execution charset
            $<$<CONFIG:Debug>:/Od /Zi /W4 >
            $<$<CONFIG:Release>:/O2 /GL>
        )
        target_link_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Release>:/LTCG>
        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        # GCC
        target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Debug>:-O0 -g -Wall -Wextra -Wpedantic -Werror>
            $<$<CONFIG:Release>:-O3>
        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # Clang or AppleClang
        target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Debug>:-O0 -g -Wall -Wextra -Wpedantic -Werror>
            $<$<CONFIG:Release>:-O3>
        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Intel")
        # Intel C++ Compiler
        target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Debug>:-O0 -g -Wall -Wextra -Werror>
            $<$<CONFIG:Release>:-O3 -ipo>
        )
    else()
        # Unknown compiler - use basic flags
        message(WARNING "Unknown compiler: ${CMAKE_CXX_COMPILER_ID}")
        target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Debug>:-O0 -g>
            $<$<CONFIG:Release>:-O3>
        )
    endif()

    # Debug/Release definitions
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
        $<$<CONFIG:Debug>:W3D_DEBUG>
    )
endif()

# Testing with Google Test
if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(lib/googletest)
  add_subdirectory(tests)
endif()
