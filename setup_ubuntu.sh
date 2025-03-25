#!/bin/bash

echo "Installing required packages for the OpenGL drawing project..."

# Update package lists
sudo apt-get update

# Install development tools
sudo apt-get install -y build-essential cmake pkg-config git

# Install OpenGL libraries
sudo apt-get install -y libgl1-mesa-dev libglu1-mesa-dev

# Install SDL2
sudo apt-get install -y libsdl2-2.0-0 libsdl2-dev

# Install GLM
sudo apt-get install -y libglm-dev

# Download ImGui if not present
if [ ! -f "imgui/imgui.cpp" ]; then
    echo "ImGui not found, downloading..."
    
    # Backup current imgui directory if it exists
    if [ -d "imgui" ]; then
        mv imgui imgui_backup_$(date +%Y%m%d_%H%M%S)
    fi
    
    # Clone ImGui repository
    git clone https://github.com/ocornut/imgui.git
    
    # No need to manually copy the backends, they're already in the cloned repository
    echo "ImGui downloaded successfully"
fi

# Create build directory if it doesn't exist
mkdir -p build
cd build

# Configure CMake for Ubuntu
cmake ..

echo "Setup complete! You can now build the project with:"
echo "cd build && make" 