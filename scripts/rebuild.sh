#!/bin/bash
# Bash script to rebuild a CMake preset on Linux
# Usage: ./rebuild-preset.sh debug
# Available presets: debug, release, test

if [ $# -eq 0 ]; then
    echo "Usage: $0 <preset>"
    echo "Available presets: debug, release, test"
    exit 1
fi

PRESET=$1

# Validate preset
case $PRESET in
    debug|release|test)
        ;;
    *)
        echo "Error: Invalid preset '$PRESET'"
        echo "Available presets: debug, release, test"
        exit 1
        ;;
esac

BUILD_DIR="build/$PRESET"

echo -e "\033[1;33mDeleting build directory: $BUILD_DIR\033[0m"
if [ -d "$BUILD_DIR" ]; then
    rm -rf "$BUILD_DIR"
    echo -e "\033[1;32mBuild directory deleted.\033[0m"
else
    echo -e "\033[1;36mBuild directory does not exist.\033[0m"
fi

echo ""
echo -e "\033[1;33mRebuilding preset: $PRESET\033[0m"
cmake --preset "$PRESET"
if [ $? -eq 0 ]; then
    echo -e "\033[1;32mPreset configuration successful.\033[0m"
else
    echo -e "\033[1;31mPreset configuration failed.\033[0m"
    exit 1
fi

echo ""
echo -e "\033[1;33mBuilding preset: $PRESET\033[0m"
cmake --build --preset "$PRESET"
if [ $? -eq 0 ]; then
    echo -e "\033[1;32mBuild successful.\033[0m"
else
    echo -e "\033[1;31mBuild failed.\033[0m"
    exit 1
fi
