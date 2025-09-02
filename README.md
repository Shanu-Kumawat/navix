# Navix

Navix is a modern, extensible 3D modelling and visualization software built with C++ and OpenGL, featuring advanced modeling, rendering, and interactive tools. This project is designed for engineers, designers, and hobbyists who need a powerful yet user-friendly platform for creating and visualizing complex 3D mechanical components.

---

## ✨ Features

- **3D Model Viewer & Editor**
  - View, rotate, zoom, and interact with 3D models (Ball Bearings, Bellows, Springs, Shock Absorbers, and more)
  - Modular architecture for easy addition of new shapes and viewers
- **Custom Canvas & Camera Controls**
  - Intuitive camera navigation (pan, orbit, zoom)
  - High-performance rendering pipeline using OpenGL (GLAD)
- **Shader Support**
  - Custom shaders for realistic lighting and effects
  - Easily extendable shader system (see `shaders/`)
- **ImGui Integration**
  - Modern, responsive GUI for all controls and settings
  - Real-time parameter adjustment and feedback
- **Cross-Platform Build System**
  - CMake-based build for Linux and Windows
  - Pre-configured scripts for easy setup (`setup_ubuntu.sh`, `build_windows.sh`)
- **Extensible Design**
  - Add new 3D models and viewers by extending base classes
  - Organized source and include structure for maintainability
- **Resource Management**
  - Efficient handling of shaders, textures, and model data
- **Sample Shaders & Models**
  - Ready-to-use sample shaders and 3D models for quick prototyping

---

## 📂 Project Structure

```
├── main.cpp                  # Application entry point
├── src/                      # Source code for models, viewers, and utilities
├── include/                  # Public headers for all components
├── shaders/                  # GLSL shaders for rendering
├── imgui/                    # ImGui library and backends
├── build/                    # Build artifacts and logs
├── CMakeLists.txt            # Main CMake build script
├── build_windows.sh          # Windows build helper script
├── setup_ubuntu.sh           # Ubuntu setup helper script
└── windows_resources/        # Windows-specific resources
```

---

## 🚀 Getting Started

### Prerequisites
- C++17 or newer
- CMake 3.15+
- OpenGL development libraries
- [GLAD](https://glad.dav1d.de/) (included)
- [ImGui](https://github.com/ocornut/imgui) (included)

### Build on Ubuntu
```sh
./setup_ubuntu.sh
mkdir -p build && cd build
cmake ..
make
./main
```

### Build on Windows (MinGW)
```sh
./build_windows.sh
```

---

## 🛠️ Extending the Project
- Add new 3D models: Implement in `src/` and declare in `include/`
- Add new shaders: Place GLSL files in `shaders/`
- Add new GUI panels: Extend ImGui code in `main.cpp` or modularize in `src/`

---

## 🖥️ Wayland / Hyprland Support

Navix includes built-in support for high-DPI rendering on Wayland compositors like Hyprland. The application automatically detects and handles fractional scaling without any additional configuration needed.

---

## 🤝 Contributing
Pull requests, bug reports, and feature suggestions are welcome! Please open an issue or submit a PR.

---

## 📄 License
This project is licensed under the MIT License. See `imgui/LICENSE.txt` for ImGui's license.

---

## 🙏 Acknowledgements
- [ImGui](https://github.com/ocornut/imgui)
- [GLAD](https://glad.dav1d.de/)
- Open source contributors

---

> _Happy Modelling with Navix!_

