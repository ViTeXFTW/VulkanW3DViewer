#!/bin/bash
# Bash script to rebuild a CMake configuration
# Usage: ./rebuild.sh <config> [-c|--compiler <compiler>] [-d]
#
# Arguments:
#   config          Build configuration: debug, release, or test
#
# Options:
#   -c, --compiler  Compiler to use: msvc, clang, or gcc (default: auto-detect)
#   -d              Delete build directory before rebuilding
#
# Examples:
#   ./rebuild.sh debug                  # Debug build (auto-detect compiler)
#   ./rebuild.sh release -c clang       # Release build with Clang
#   ./rebuild.sh debug -d               # Clean debug build

DELETE_BUILD=false
COMPILER=""
CONFIG=""

# Function to show help
show_help() {
    echo ""
    echo -e "\033[1;36mrebuild.sh - Build script for VulkanW3DViewer\033[0m"
    echo ""
    echo -e "\033[1;33mUSAGE:\033[0m"
    echo "  $0 <config> [-c|--compiler <compiler>] [-d]"
    echo "  $0 -h|--help"
    echo ""
    echo -e "\033[1;33mARGUMENTS:\033[0m"
    echo "  config          Build configuration: debug, release, or test"
    echo ""
    echo -e "\033[1;33mOPTIONS:\033[0m"
    echo "  -c, --compiler  Compiler to use: msvc, clang, or gcc (default: auto-detect)"
    echo "  -d              Delete build directory before rebuilding"
    echo "  -h, --help      Show this help message"
    echo ""
    echo -e "\033[1;33mEXAMPLES:\033[0m"
    echo "  $0 debug                  # Debug build (auto-detect compiler)"
    echo "  $0 release -c gcc         # Release build with GCC"
    echo "  $0 debug -d -c clang      # Clean debug build with Clang"
    echo "  $0 test                   # Build and run tests"
    echo ""
    exit 0
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            ;;
        -d)
            DELETE_BUILD=true
            shift
            ;;
        -c|--compiler)
            COMPILER="$2"
            shift 2
            ;;
        debug|release|test)
            CONFIG="$1"
            shift
            ;;
        *)
            echo "Error: Unknown argument '$1'"
            echo "Run '$0 --help' for usage information"
            exit 1
            ;;
    esac
done

# Validate config
if [ -z "$CONFIG" ]; then
    echo "Error: No configuration specified"
    echo "Run '$0 --help' for usage information"
    exit 1
fi

# Validate compiler if specified
if [ -n "$COMPILER" ]; then
    case $COMPILER in
        msvc|clang|gcc)
            ;;
        *)
            echo "Error: Invalid compiler '$COMPILER'"
            echo "Valid compilers: msvc, clang, gcc"
            exit 1
            ;;
    esac
fi

# Construct preset name
if [ -n "$COMPILER" ]; then
    PRESET="$COMPILER-$CONFIG"
else
    PRESET="$CONFIG"
fi

BUILD_DIR="build/$PRESET"

# Warn if trying to use MSVC on non-Windows
if [ "$COMPILER" = "msvc" ]; then
    echo -e "\033[1;33mWarning: MSVC is designed for Windows.\033[0m"
    echo -e "\033[1;33mConsider using clang or gcc on Linux/Mac.\033[0m"
fi

if [ "$DELETE_BUILD" = true ]; then
    echo -e "\033[1;33mDeleting build directory: $BUILD_DIR\033[0m"
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        echo -e "\033[1;32mBuild directory deleted.\033[0m"
    else
        echo -e "\033[1;36mBuild directory does not exist.\033[0m"
    fi
fi

echo ""
if [ -n "$COMPILER" ]; then
    echo -e "\033[1;33mBuilding $CONFIG with $COMPILER (preset: $PRESET)\033[0m"
else
    echo -e "\033[1;33mBuilding $CONFIG with auto-detected compiler (preset: $PRESET)\033[0m"
fi
cmake --preset "$PRESET"
if [ $? -eq 0 ]; then
    echo -e "\033[1;32mPreset configuration successful.\033[0m"
else
    echo -e "\033[1;31mPreset configuration failed.\033[0m"
    exit 1
fi

echo ""
echo -e "\033[1;33mBuilding...\033[0m"
cmake --build --preset "$PRESET"
if [ $? -eq 0 ]; then
    echo -e "\033[1;32mBuild successful.\033[0m"
else
    echo -e "\033[1;31mBuild failed.\033[0m"
    exit 1
fi
