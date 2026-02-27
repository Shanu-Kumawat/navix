#!/bin/bash

# Update library path for Gmsh if it exists locally
if [ -d "$(pwd)/external/gmsh/lib" ]; then
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/external/gmsh/lib
fi

# Set directory so icons load relative to binary correctly
cd build
./main "$@"
