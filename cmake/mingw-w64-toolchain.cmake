# Cross-compilation toolchain for building Windows x64 binaries from Linux
# using the MinGW-w64 GCC cross-compiler.
#
# Usage:
#   cmake -B build/mingw-release \
#     -G Ninja \
#     -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-toolchain.cmake \
#     ...

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(TOOLCHAIN_PREFIX x86_64-w64-mingw32)

set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
set(CMAKE_RC_COMPILER  ${TOOLCHAIN_PREFIX}-windres)

# Target sysroot (MinGW-w64 libraries and headers)
set(CMAKE_FIND_ROOT_PATH /usr/${TOOLCHAIN_PREFIX})

# Search for *programs* (cmake, ninja, glslc) on the HOST (Linux), not in the
# Windows sysroot. glslc must run on the build machine to compile shaders.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for *libraries* and *headers* in the target sysroot only, so that
# CMake finds Windows libraries (e.g. gdi32, user32) and not Linux ones.
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
