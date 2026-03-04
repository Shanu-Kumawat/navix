# Current Architecture (Audited February 28, 2026)

## Overview
The application has a dual architecture: legacy monolithic code that actually works, and MVC/FEM scaffolding that exists as dead or pass-through code.

## Active Architecture (What actually runs)

### 1. `main.cpp` (366 lines)
- Clean entry point: SDL2 window, OpenGL context, ImGui setup, main loop
- Delegates to UI panel classes: `TopRibbon`, `CanvasView`, `PropertyPanel`, `StatusBar`, `Viewers3DUI`
- Still references `UIState::` globals for 3D view flags

### 2. `Canvas` (`Canvas.hpp` / `Canvas.cpp` — 2030+ lines) — **THE MONOLITH**
- **Still the God Object.** Owns ALL drawing logic, input handling, shape storage, selection, undo/redo, zoom/pan, grid/snap
- Data: `std::vector<std::unique_ptr<Shape>> shapes` — the real source of truth
- Input: `handleInput()` → `handleDrawing()` → 11 shape-specific handlers
- Rendering: delegates to `Renderer2D` for grid/shapes/previews
- Also owns (but doesn't truly use): `InputController`, `SceneModel`, `TopologyManager`, `MaterialManager`

### 3. `Renderer2D` (1210 lines) — **The real renderer**
- Renders grid, shapes, snap indicators, and live previews for all 11 shape types
- Uses `ImDrawList*` from ImGui directly
- Called by Canvas::render() and Canvas::handleDrawing()

### 4. UI Panels (`src/ui/`)
- `TopRibbon.cpp` (340 lines) — toolbar with icon buttons, grid/snap, 3D view buttons
- `PropertyPanel.cpp` (1095 lines) — context-sensitive shape properties, material assignment
- `CanvasView.cpp` (308 lines) — hosts canvas window, rulers, coordinate display
- `StatusBar.cpp` (104 lines) — bottom bar with coordinates, mode, messages
- `Viewers3DUI.cpp` (105 lines) — 3D view windows for Bellows, BallBearing, Spring, ShockAbsorber
- `UIHelpers.cpp` — SelectTool(), IconButton(), icon texture loading

### 5. 3D Visualization (Functional)
- `Base3DViewer` — framebuffer management, camera handling, texture display
- `BellowsViewer3D`, `BallBearingViewer3D`, `SpringViewer3D`, `ShockAbsorberViewer3D`
- Each uses `Base3DModel` derivatives for mesh generation
- Shader: `shaders/bellows.vs/fs` (shared across viewers)

### 6. State Management (DUAL SYSTEM)
- `ApplicationContext` — holds some state (active mode, snap, grid, console messages, 3D view flags)
- `namespace UIState` — STILL holds ~20 global variables (panel width, layers, units, 3D viewer instances, show flags)
- Both are used simultaneously by different parts of the UI

## Scaffolding (Exists but non-functional)

### MVC Components (Phase 2)
- `InputController` — pure delegation stub, all methods call canvas->sameMethod()
- `SceneModel` — has real undo/redo and shapes logic but Canvas's vector is used instead
- `Core::Renderer` / `ImGuiRenderer` — abstract interface + implementation, never instantiated

### Topology (Phase 2)
- `Node`, `Edge`, `Face`, `TopologyManager` — clean data structures, CRUD operations
- **Never populated** — drawing operations don't create topology entities

### Meshing (Phase 3)
- `GmshTranslator`, `GmshExtractor`, `Mesh` — real code behind `#ifdef USE_GMSH`
- `USE_GMSH` is NOT defined (Gmsh library missing from `external/gmsh/lib/`)
- All meshing code compiles to no-ops

### FEA (Phase 4)
- `MaterialManager` — functional, 4 materials, UI integration works
- Eigen3 — build system configured, zero actual usage in source code
- No solver, boundary conditions, loads, or post-processing code

## Data Flow (What actually happens)
```
User clicks tool → TopRibbon → UIHelpers::SelectTool()
  → appContext.activeMode = mode
  → canvas.setDrawingMode(mode)

User draws on canvas → CanvasView::Render()
  → ImGui::Begin("Canvas")
  → canvas.render(drawList)           [grid + existing shapes via Renderer2D]
  → canvas.handleInput()              [hover check → handleDrawing()]
    → handleXxxDrawing(drawList, pos) [click/drag → shapes.push_back()]
    → renderer->previewXxx(drawList)  [live preview during drawing]
  → ImGui::End()
```
