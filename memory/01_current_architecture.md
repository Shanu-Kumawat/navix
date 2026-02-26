# Current Architecture

## Overview
The current architecture of NAVIX is heavily UI-centric, with business logic, rendering, and state management tightly coupled within a few large classes.

## Key Components

### 1. `main.cpp` (The God Object)
- **Responsibilities**: Application entry point, window creation (SDL2), OpenGL context initialization, ImGui setup, main application loop, and rendering of all UI panels.
- **State Management**: Uses a global `namespace UIState` to store application state (active tool, snap settings, view settings, command line history).
- **Issues**: Violates Single Responsibility Principle (SRP), makes unit testing impossible, and prevents multi-threading or multi-document support.

### 2. `Canvas` (`Canvas.hpp` / `Canvas.cpp`)
- **Responsibilities**: Handles 2D drawing canvas, input processing (mouse/keyboard), coordinate transformations (pan/zoom), grid rendering, and shape manipulation.
- **Data Structure**: Stores shapes as a flat list: `std::vector<std::unique_ptr<Shape>> shapes`.
- **Issues**: Extremely large class (3,400+ lines). Mixes input handling, rendering, and geometric logic. The flat list of shapes lacks the topological relationships (Nodes, Edges, Faces) required for FEM.

### 3. 3D Visualization (`ComplexShape3DManager`, `Base3DViewer`, `Base3DModel`)
- **Responsibilities**: Manages 3D OpenGL viewports for complex shapes (Bellows, Ball Bearings, Shock Absorbers).
- **Architecture**: 
  - `ComplexShape3DManager` acts as a unified manager for all 3D viewers, standardizing initialization and input handling.
  - `Base3DViewer` encapsulates framebuffer management and camera handling.
  - `Base3DModel` provides standardized mesh handling, shader management, and material properties.
- **Issues**: Direct OpenGL calls are mixed with business logic, making it difficult to implement advanced rendering techniques (like stress contour mapping) later.

### 4. Geometry and Math (`shapes/`, `utils/`)
- **Responsibilities**: Defines basic shapes (`Point`, `Line`, `Circle`, `Spline`, `BezierCurve`) and complex shapes. Provides custom math utilities (`MathUtils.hpp`, `VectorMath.hpp`).
- **Precision**: Heavily relies on `float` precision (via ImGui's `ImVec2`).
- **Issues**: `float` precision is insufficient for FEA (requires 64-bit `double`). Custom geometry implementations are prone to edge-case bugs and lack the robustness of established geometry kernels.
