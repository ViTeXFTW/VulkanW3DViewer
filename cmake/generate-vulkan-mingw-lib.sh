#!/usr/bin/env bash
# generate-vulkan-mingw-lib.sh
#
# Downloads the official vulkan-1.def from the Khronos Vulkan-Loader repository
# and generates a MinGW-w64 import library (libvulkan-1.a) from it.
#
# At link time this tells the linker that Vulkan functions live in vulkan-1.dll.
# At runtime on the target Windows machine, vulkan-1.dll is installed by the
# GPU driver and dispatches calls to the actual Vulkan ICD.
#
# Usage:
#   bash cmake/generate-vulkan-mingw-lib.sh <output-dir>
#
# Output:
#   <output-dir>/vulkan-1.def
#   <output-dir>/libvulkan-1.a

set -euo pipefail

OUTPUT_DIR="${1:-.}"
DEF_FILE="${OUTPUT_DIR}/vulkan-1.def"
LIB_FILE="${OUTPUT_DIR}/libvulkan-1.a"

# The official .def file from KhronosGroup/Vulkan-Loader lists all ~300
# functions exported by vulkan-1.dll on Windows.
DEF_URL="https://raw.githubusercontent.com/KhronosGroup/Vulkan-Loader/main/loader/vulkan-1.def"

echo "==> Creating output directory: ${OUTPUT_DIR}"
mkdir -p "${OUTPUT_DIR}"

echo "==> Downloading vulkan-1.def from KhronosGroup/Vulkan-Loader..."
curl -fsSL "${DEF_URL}" -o "${DEF_FILE}"
echo "    Downloaded $(wc -l < "${DEF_FILE}") lines"

echo "==> Generating MinGW import library: ${LIB_FILE}"
# -k  strips the @N ordinal suffixes (standard for x86_64 builds)
x86_64-w64-mingw32-dlltool \
  --input-def  "${DEF_FILE}" \
  --output-lib "${LIB_FILE}" \
  -k

echo "==> Done: ${LIB_FILE}"
