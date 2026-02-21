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

# Only build main application if not in tests-only mode
if(NOT BUILD_TESTING)
    # Find Vulkan
    find_package(Vulkan REQUIRED)

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

    # Collect source files
    file(GLOB_RECURSE SOURCES "src/*.cpp")
    file(GLOB_RECURSE HEADERS "include/*.hpp" "include/*.h")

    # Create executable
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${IMGUI_SOURCES})

    # Include directories
    # Use Vulkan headers from Vulkan-Hpp submodule (not system headers) to ensure version compatibility
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/Vulkan-Hpp/Vulkan-Headers/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/Vulkan-Hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui/backends
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/CLI11/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/json/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/BigXtractor/include
    )

    # Link libraries
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Vulkan::Vulkan
        glfw
        glm::glm
        bigx::bigx
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
