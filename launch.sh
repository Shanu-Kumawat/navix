#!/bin/bash

# Copy icons to build directory if they don't exist or are outdated
if [ ! -d "build/icons" ] || [ "icons" -nt "build/icons" ]; then
    cp -r icons build/
fi

# Update library path for Gmsh if it exists locally
if [ -d "$(pwd)/external/gmsh/lib" ]; then
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/external/gmsh/lib
fi

# Set directory so icons load relative to binary correctly
cd build
./main "$@"
