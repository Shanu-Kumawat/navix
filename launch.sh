#!/bin/bash

# Define the build directory
BUILD_DIR="build"

# Check if the build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo "Build directory not found. Creating..."
    mkdir "$BUILD_DIR"
fi

# Enter the build directory
cd "$BUILD_DIR" || exit

# Run CMake
cmake ..

# Compile the project
make

# Run the compiled program
./main

# Return to the previous directory
cd ..
