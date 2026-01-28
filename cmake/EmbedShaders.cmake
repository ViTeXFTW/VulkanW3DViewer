# EmbedShaders.cmake - Generate C++ header with embedded SPIR-V shaders
# Usage: cmake -DSHADER_FILES="file1;file2;..." -DOUTPUT_FILE="output.hpp" -P EmbedShaders.cmake

if(NOT SHADER_FILES)
  message(FATAL_ERROR "SHADER_FILES not specified")
endif()

if(NOT OUTPUT_FILE)
  message(FATAL_ERROR "OUTPUT_FILE not specified")
endif()

# Start generating the header file
file(WRITE ${OUTPUT_FILE} "// Auto-generated file - DO NOT EDIT\n")
file(APPEND ${OUTPUT_FILE} "// Generated from shader SPIR-V files\n\n")
file(APPEND ${OUTPUT_FILE} "#pragma once\n\n")
file(APPEND ${OUTPUT_FILE} "#include <cstdint>\n")
file(APPEND ${OUTPUT_FILE} "#include <cstddef>\n")
file(APPEND ${OUTPUT_FILE} "#include <string_view>\n")
file(APPEND ${OUTPUT_FILE} "#include <span>\n\n")
file(APPEND ${OUTPUT_FILE} "namespace w3d::shaders {\n\n")

# Track shader names for the registry
set(SHADER_NAMES "")

foreach(SHADER_FILE ${SHADER_FILES})
  if(NOT EXISTS ${SHADER_FILE})
    message(WARNING "Shader file not found: ${SHADER_FILE}")
    continue()
  endif()

  # Get the base name without path and extension
  get_filename_component(SHADER_NAME ${SHADER_FILE} NAME)
  string(REPLACE "." "_" SAFE_NAME ${SHADER_NAME})

  # Read the file as hex string
  file(READ ${SHADER_FILE} SHADER_HEX HEX)

  # Get file size
  file(SIZE ${SHADER_FILE} SHADER_SIZE)

  # Convert hex string to comma-separated byte array
  string(LENGTH ${SHADER_HEX} HEX_LENGTH)

  file(APPEND ${OUTPUT_FILE} "// ${SHADER_NAME} (${SHADER_SIZE} bytes)\n")
  file(APPEND ${OUTPUT_FILE} "inline constexpr uint8_t ${SAFE_NAME}_data[] = {\n  ")

  set(BYTE_COUNT 0)
  set(LINE_COUNT 0)

  math(EXPR HEX_LENGTH_MINUS_ONE "${HEX_LENGTH} - 1")
  foreach(i RANGE 0 ${HEX_LENGTH_MINUS_ONE} 2)
    string(SUBSTRING ${SHADER_HEX} ${i} 2 BYTE)
    file(APPEND ${OUTPUT_FILE} "0x${BYTE}")

    math(EXPR BYTE_COUNT "${BYTE_COUNT} + 1")
    math(EXPR LINE_COUNT "${LINE_COUNT} + 1")

    if(NOT BYTE_COUNT EQUAL SHADER_SIZE)
      file(APPEND ${OUTPUT_FILE} ", ")
      if(LINE_COUNT EQUAL 12)
        file(APPEND ${OUTPUT_FILE} "\n  ")
        set(LINE_COUNT 0)
      endif()
    endif()
  endforeach()

  file(APPEND ${OUTPUT_FILE} "\n};\n\n")
  file(APPEND ${OUTPUT_FILE} "inline constexpr std::span<const uint8_t> ${SAFE_NAME}() {\n")
  file(APPEND ${OUTPUT_FILE} "  return std::span<const uint8_t>{${SAFE_NAME}_data, ${SHADER_SIZE}};\n")
  file(APPEND ${OUTPUT_FILE} "}\n\n")

  list(APPEND SHADER_NAMES ${SHADER_NAME})
endforeach()

# Create a getter function that returns shader data by name
file(APPEND ${OUTPUT_FILE} "// Get shader by name\n")
file(APPEND ${OUTPUT_FILE} "inline std::span<const uint8_t> getShader(std::string_view name) {\n")

set(FIRST_SHADER TRUE)
foreach(SHADER_NAME ${SHADER_NAMES})
  string(REPLACE "." "_" SAFE_NAME ${SHADER_NAME})

  if(FIRST_SHADER)
    file(APPEND ${OUTPUT_FILE} "  if (name == \"${SHADER_NAME}\") return ${SAFE_NAME}();\n")
    set(FIRST_SHADER FALSE)
  else()
    file(APPEND ${OUTPUT_FILE} "  else if (name == \"${SHADER_NAME}\") return ${SAFE_NAME}();\n")
  endif()
endforeach()

file(APPEND ${OUTPUT_FILE} "  return {}; // Shader not found\n")
file(APPEND ${OUTPUT_FILE} "}\n\n")

file(APPEND ${OUTPUT_FILE} "} // namespace w3d::shaders\n")

message(STATUS "Generated embedded shaders header: ${OUTPUT_FILE}")
