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

# Auto-detect sysroot from the compiler rather than hardcoding a distro-specific
# path like /usr/x86_64-w64-mingw32 (Debian/Ubuntu default).
execute_process(
  COMMAND ${CMAKE_CXX_COMPILER} -print-sysroot
  OUTPUT_VARIABLE _MINGW_SYSROOT
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
  RESULT_VARIABLE _SYSROOT_RESULT
)
if(_SYSROOT_RESULT EQUAL 0 AND _MINGW_SYSROOT)
  set(CMAKE_FIND_ROOT_PATH "${_MINGW_SYSROOT}")
else()
  # Fallback: standard location on Debian/Ubuntu
  set(CMAKE_FIND_ROOT_PATH /usr/${TOOLCHAIN_PREFIX})
endif()

# Search for *programs* (cmake, ninja, glslc) on the HOST (Linux), not in the
# Windows sysroot. glslc must run on the build machine to compile shaders.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for *libraries* and *headers* in the target sysroot only, so that
# CMake finds Windows libraries (e.g. gdi32, user32) and not Linux ones.
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
