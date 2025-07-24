#!/bin/bash

# Exit on error
set -e

echo "==== Building Windows executable for NAVIX ===="

# Parse command line arguments
CLEAN_BUILD=0
for arg in "$@"; do
    case $arg in
        --clean)
            CLEAN_BUILD=1
            shift
            ;;
        *)
            # Unknown option
            ;;
    esac
done

# Check required tools
if ! command -v unzip &> /dev/null; then
    echo "unzip is not installed. Installing..."
    sudo pacman -S unzip --noconfirm
fi

if ! command -v zip &> /dev/null; then
    echo "zip is not installed. Installing..."
    sudo pacman -S zip --noconfirm
fi

# Create deps directory if it doesn't exist
mkdir -p deps

# Check if SDL2 is already downloaded or if clean build requested
if [ ! -d "deps/SDL2-2.26.5" ] || [ $CLEAN_BUILD -eq 1 ]; then
    if [ $CLEAN_BUILD -eq 1 ] && [ -d "deps/SDL2-2.26.5" ]; then
        echo "Clean build requested. Removing existing SDL2..."
        rm -rf deps/SDL2-2.26.5
    fi
    
    echo "Downloading SDL2..."
    cd deps
    wget https://github.com/libsdl-org/SDL/releases/download/release-2.26.5/SDL2-devel-2.26.5-mingw.tar.gz
    tar -xzf SDL2-devel-2.26.5-mingw.tar.gz
    rm SDL2-devel-2.26.5-mingw.tar.gz
    cd ..
    echo "SDL2 downloaded and extracted."
else
    echo "SDL2 already downloaded."
fi

# Check if GLM is already downloaded or if clean build requested
if [ ! -d "deps/glm" ] || [ $CLEAN_BUILD -eq 1 ]; then
    if [ $CLEAN_BUILD -eq 1 ] && [ -d "deps/glm" ]; then
        echo "Clean build requested. Removing existing GLM..."
        rm -rf deps/glm
    fi
    
    echo "Downloading GLM..."
    cd deps
    wget https://github.com/g-truc/glm/releases/download/0.9.9.8/glm-0.9.9.8.zip
    unzip glm-0.9.9.8.zip
    rm glm-0.9.9.8.zip
    cd ..
    echo "GLM downloaded and extracted."
else
    echo "GLM already downloaded."
fi

# Create or clean build directory
if [ $CLEAN_BUILD -eq 1 ] && [ -d "build_windows" ]; then
    echo "Clean build requested. Removing existing build directory..."
    rm -rf build_windows
fi

mkdir -p build_windows
cd build_windows

# Copy the Windows CMakeLists.txt if it exists
if [ -f "../CMakeLists_Windows.txt" ]; then
    echo "Using Windows-specific CMakeLists.txt"
    cp ../CMakeLists_Windows.txt ../CMakeLists.txt
fi

# Configure with CMake
echo "Configuring with CMake..."
cmake -DCMAKE_TOOLCHAIN_FILE=../mingw-w64-x86_64.cmake -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles" ..

# Build
echo "Building..."
cmake --build . --config Release -- -j$(nproc)

# Create a distribution directory
echo "Creating distribution package..."
mkdir -p dist/NAVIX
cp main.exe dist/NAVIX/
cp -r ../shaders dist/NAVIX/

# Copy SDL2.dll from our deps directory
cp ../deps/SDL2-2.26.5/x86_64-w64-mingw32/bin/SDL2.dll dist/NAVIX/

# Create a zip file of the distribution
echo "Creating ZIP archive..."
cd dist
zip -r NAVIX-Windows.zip NAVIX
mv NAVIX-Windows.zip ../../
cd ../..

echo "==== Build completed successfully ===="
echo "Windows executable is in build_windows/dist/NAVIX/"
echo "Windows distribution ZIP file: NAVIX-Windows.zip ($(du -h NAVIX-Windows.zip | cut -f1))"
echo ""
echo "To create a clean build, run: ./build_windows.sh --clean" 