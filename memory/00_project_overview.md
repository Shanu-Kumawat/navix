# NAVIX Project Overview

## Vision
NAVIX is a Finite Element Method (FEM) analysis software designed with a strong emphasis on usability and accessibility for engineers. The goal is to enable users to easily work with predefined engineering components (e.g., beams, shock absorbers, ball bearings) and perform robust finite element analysis.

## Current State (Audited February 28, 2026)
The application currently functions as a **2D drawing and 3D visualization tool**. The FEM pipeline (topology, meshing, solving) exists as scaffolding but is **not functional**.

### What Works
- 11 shape drawing tools with live previews (Point, Line, Circle, Triangle, Square, Rectangle, Spline, Bezier, Bellows, Ball Bearing, Spring2D)
- 3D viewers for Bellows, Ball Bearing, Spring, Shock Absorber
- Undo/Redo (snapshot-based via CommandManager wrapper)
- Grid display and snap-to-grid
- Shape selection and property panel editing
- Pan/zoom navigation
- Material assignment UI (4 preloaded materials)

### What Exists as Scaffolding Only (Not Functional)
- `InputController` — pure delegation stub, Canvas still handles all input
- `SceneModel` — exists but Canvas's own shapes vector is the source of truth
- `TopologyManager` with Node/Edge/Face — never populated by drawing operations
- Abstract `Renderer` / `ImGuiRenderer` — dead code, Canvas uses `Renderer2D`
- Gmsh integration — headers present, **library missing**, all code behind `#ifdef USE_GMSH`
- `GmshTranslator`, `GmshExtractor`, `Mesh` class — real code but compiled to no-ops
- Eigen3 — build finds it but zero source files include Eigen headers
- `namespace UIState` — still alive with ~20 globals alongside `ApplicationContext`

## Future Goals
- **Complete Phase 1**: Truly eradicate UIState globals, complete Canvas MVC decomposition
- **Complete Phase 2**: Wire topology into drawing operations, implement granular commands
- **Complete Phase 3**: Download Gmsh library, enable `USE_GMSH`, wire UI trigger for meshing
- **Phase 4**: Boundary conditions, loads, stiffness matrix assembly, solver, post-processing
- **UI/UX**: Professional redesign with docking, modern toolbars, dynamic property panels

## Core Technologies
- **Language**: C++17
- **UI Framework**: Dear ImGui
- **Windowing/Input**: SDL2
- **Graphics API**: OpenGL (via GLAD)
- **Math**: GLM (double precision `glm::dvec2` / `glm::dvec3`)
- **Build**: CMake, WSL Ubuntu
