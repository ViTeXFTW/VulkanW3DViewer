cmake_minimum_required(VERSION 3.28)
project(VulkanW3DViewer VERSION 0.3.0 LANGUAGES CXX)

# C++20 standard
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

# Common dependencies
# GLFW options - disable unnecessary builds
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# Add GLFW submodule
add_subdirectory(lib/glfw)

# Add GLM submodule
add_subdirectory(lib/glm)

# BigXtractor library for BIG archive support
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(INSTALL_STANDALONE OFF CACHE BOOL "" FORCE)
add_subdirectory(lib/BigXtractor)

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

# Common include directories for all targets
set(COMMON_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/Vulkan-Hpp/Vulkan-Headers/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/Vulkan-Hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/CLI11/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/json/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/BigXtractor/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/glfw/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui/backends
)

# Create w3d_lib static library
# Library contains: W3D parsing, BIG archive support, rendering logic (Vulkan-free), scene management
# Excludes: Vulkan graphics (gfx/), core app framework (core/), UI (ui/)
# Excludes: Vulkan-dependent loaders (loader.cpp, model_loader.cpp, hlod_model.cpp) which go to executable
file(GLOB_RECURSE LIB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/formats/w3d/animation_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/formats/w3d/chunk_types.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/formats/w3d/chunk_reader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/formats/w3d/hierarchy_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/formats/w3d/hlod_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/formats/w3d/mesh_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/formats/big/ini_extractor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/formats/big/archive.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/formats/big/reader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/scene/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/render/skeleton.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/render/raycast.cpp
)

add_library(w3d_lib STATIC ${LIB_SOURCES})

target_include_directories(w3d_lib PUBLIC
    ${COMMON_INCLUDE_DIRS}
)

target_link_libraries(w3d_lib PUBLIC
    glm::glm
    big::big
)

# Common compiler options function
function(set_common_compile_options target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endfunction()

# Apply common options to library
set_common_compile_options(w3d_lib)

# Main application executable (requires Vulkan)
if(NOT BUILD_TESTING)
    # Find Vulkan
    find_package(Vulkan REQUIRED)

    # Collect application source files
    # Executable contains: main.cpp, core framework, UI, Vulkan graphics, and Vulkan-dependent rendering
    # Note: skeleton.cpp and raycast.cpp are in w3d_lib (excluded here to avoid duplicates)
    file(GLOB_RECURSE APP_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/gfx/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/render/animation_player.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/render/bone_buffer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/render/hover_detector.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/render/mesh_converter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/render/renderable_mesh.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/render/skeleton_renderer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/formats/w3d/loader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/formats/w3d/model_loader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/formats/w3d/hlod_model.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/formats/big/asset_registry.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/formats/big/big_archive_manager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    )

    add_executable(${PROJECT_NAME} ${APP_SOURCES} ${IMGUI_SOURCES})

    target_include_directories(${PROJECT_NAME} PRIVATE
        ${COMMON_INCLUDE_DIRS}
    )

    target_link_libraries(${PROJECT_NAME} PRIVATE
        Vulkan::Vulkan
        glfw
        w3d_lib
        big::big
    )

    # Compiler-specific flags
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Debug>:/Od /Zi>
            $<$<CONFIG:Release>:/O2 /GL>
        )
        target_link_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Release>:/LTCG>
        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Debug>:-O0 -g>
            $<$<CONFIG:Release>:-O3>
        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Debug>:-O0 -g>
            $<$<CONFIG:Release>:-O3>
        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Intel")
        target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Debug>:-O0 -g>
            $<$<CONFIG:Release>:-O3 -ipo>
        )
    else()
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

    # Find glslc shader compiler
    find_program(GLSLC glslc HINTS "$ENV{VULKAN_SDK}/Bin")
    if(NOT GLSLC)
        message(FATAL_ERROR "glslc not found! Make sure Vulkan SDK is installed.")
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

        # Add generated directory to include path
        target_include_directories(${target} PRIVATE ${CMAKE_BINARY_DIR}/generated)
    endfunction()

    # Compile and embed shaders
    compile_shaders(${PROJECT_NAME})
endif()

# Testing with Google Test
if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(lib/googletest)
  add_subdirectory(tests)
endif()
