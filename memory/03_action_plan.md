# Prioritized Action Plan

This document outlines the phased approach to refactoring NAVIX and preparing it for meshing and FEA solver integration.

**Last audited: March 1, 2026 (evening session 3)** — Status verified against actual source code.

## Phase 1: Stabilization & Decoupling (COMPLETE)
**Goal**: Establish a clean, modular architecture and eliminate technical debt.
- **Priority 1**: [COMPLETED] Eradicate global state. `namespace UIState` fully removed. All globals migrated into `ApplicationContext`.
- **Priority 2**: [COMPLETED] Break down `Canvas.cpp`. Canvas is now ~1100 lines (thin facade). `InputController` (~800 lines) owns all input handling and drawing tool logic. `SceneModel` is canonical owner of shapes. `Renderer2D` (1209 lines) handles all rendering.
- **Priority 3**: [COMPLETED] Break down `main.cpp`. UI panels extracted into 5 files. main.cpp is 364 lines.
- **Priority 4**: [COMPLETED] Replace custom math with `glm::dvec2`/`glm::dvec3`. Zero `ImVec2` in core logic.
- **Priority 5**: [COMPLETED] Decouple OpenNURBS. No dependency found.

## Phase 2: Architectural Foundation for FEM (COMPLETE)
**Goal**: Implement the necessary data structures and patterns for robust engineering software.
- **Priority 1**: [COMPLETED] Command Pattern. `CommandManager` + 6 granular commands: `AddShapeCommand`, `DeleteShapeCommand`, `MoveShapeCommand`, `DuplicateShapeCommand`, `RotateShapeCommand`, `ScaleShapeCommand`. All wired into Canvas. `duplicateSelectedShape()` uses clone() + `DuplicateShapeCommand`. `rotateSelectedShape()` / `scaleSelectedShape()` use `RotateShapeCommand` / `ScaleShapeCommand` with bounding-box center. Legacy `CanvasSnapshotCommand` retained only for `clearAll()`.
- **Priority 2**: [COMPLETED] Topological data structures. `Node`, `Edge`, `Face`, `TopologyManager` fully coded with CRUD + validation. `Canvas::shapeTopoMap` maps `Shape*` → `{nodeIds, edgeIds, faceIds}`. Topology created for 8 shape types (Point, Line, Circle, Triangle, Square, Rectangle, Spline, Bezier). Topology synced: `removeTopologyForShape()` on delete, `updateTopologyPositions()` on move/rotate/scale. Complex shapes (Bellows, BallBearing, Spring2D) left as TBD — parametric, not suitable for simple node/edge topology.
- **Priority 3**: [COMPLETED] Abstract Renderer interface. `Core::Renderer` abstract class exists. `ImGuiRenderer : public Renderer` implements all virtuals (topology overlay). `Renderer2D : public Renderer` now inherits the abstract interface. Implements all pure virtuals (`drawLine`, `drawCircle`, `drawPolygon`, `drawText`, `drawShape→renderShape`, `drawNode`, `drawEdge`, `drawMesh`). Has `beginFrame(ImDrawList*)` + `setTransform()` for the abstract pipeline. Retains all existing high-level shape/grid/preview rendering.

## Phase 3: Meshing Integration (COMPLETE)
**Goal**: Enable the conversion of geometric models into finite element meshes.
- **Priority 1**: [COMPLETED] Gmsh build integration. `cmake/FindGmsh.cmake` finds the local SDK. `external/gmsh/lib/libgmsh.so` (4.13.1) installed. `USE_GMSH` is defined at compile time. Build compiles and links successfully against Gmsh.
- **Priority 2**: [COMPLETED] Translation layer. `GmshTranslator` translates topology (nodes→points, edges→lines, faces→curveloops+planesurfaces) via `gmsh::model::occ`. Fixed bugs: removed misplaced `#include` directives inside function bodies, removed duplicate header include. Made `extractGeneratedMesh()` public.
- **Priority 3**: [COMPLETED] Mesh class. `Mesh.hpp/cpp` (MeshNode, MeshElement) fully Gmsh-independent. `GmshExtractor` extracts nodes/elements from Gmsh model.
- **Priority 4**: [COMPLETED] Render mesh. `Renderer2D::drawMesh()` renders mesh edge wireframe on canvas. Canvas::render() integrates mesh overlay when `showMesh` is true.
- **Priority 5**: [COMPLETED] Meshing UI. TopRibbon "Mesh" panel with Generate, Clear Mesh buttons, Show toggle, and Element Size slider. Canvas exposes `generateMesh()`, `clearMesh()`, `hasMesh()`, `setMeshElementSize()`, `setMeshVisible()`.
- **Cleanup**: Legacy `GmshIntegration` class removed from build (was an inferior duplicate of `GmshTranslator`; only translated nodes, not edges/faces).

## Phase 4: FEA Solver & Post-Processing (Next Focus)
**Goal**: Perform structural analysis and visualize results.
- **Priority 1**: [PARTIAL] Eigen3 integration. CMakeLists correctly finds system Eigen3 or fetches 3.4.0 via FetchContent. **Zero source files actually `#include <Eigen/...>`**. No FEA compute code exists.
- **Priority 2**: [COMPLETED] Material Manager APIs and UI. `MaterialManager` with 4 preloaded materials. `PropertyPanel` shows material assignment combo box. `Shape::materialId` field exists. Limitation: Cannot create/edit materials from UI (read-only library).
- **Priority 3**: [NOT STARTED] Boundary Conditions (Dirichlet/Fixities).
- **Priority 4**: [NOT STARTED] Load interfaces (Neumann/Force vectors).
- **Priority 5**: [NOT STARTED] Stiffness matrix assembly and solver.
- **Priority 6**: [NOT STARTED] Post-processing rendering (stress contours).
- **Priority 7**: [NOT STARTED] Complete UI/UX Redesign.

## What Actually Works (March 1, 2026)
These features are **verified functional** in the running application:

### Drawing Tools (all with live previews)
- Point, Line, Circle, Triangle, Square, Rectangle
- Spline (left-click to place points, right-click to finalize)
- Bezier Curve (4 control points)
- Bellows, Ball Bearing, Spring2D

### Meshing (NEW — Phase 3)
- Generate 2D mesh from drawn shapes via Gmsh 4.13.1 SDK
- Mesh wireframe overlay on canvas (toggle show/hide)
- Element size control (1.0–50.0 slider)
- Clear mesh

### 3D Visualization
- Bellows 3D viewer (select bellows → click "3D Bellows" button)
- Ball Bearing 3D viewer (select ball bearing → click "3D Ball Bearing" button)
- Spring 3D viewer (select spring → click "3D Spring" button)
- Shock Absorber 3D viewer (requires assembly: spring + top/bottom end)

### Core Features
- Undo/Redo (Ctrl+Z / Ctrl+Y, 6 granular commands + snapshot for clearAll)
- Grid display toggle
- Snap to grid
- Shape selection and property editing
- Pan (middle mouse) and Zoom (scroll wheel)
- Material assignment to shapes

### Known Gaps in Running App
- No property panel for Spline/Bezier selected shapes
- No Dimension drawing mode (shape type exists but inaccessible)
- Meshing requires closed shapes with faces — open lines/points produce only point meshes
- No FEA computation (Eigen included but unused)
- Topology for complex shapes (Bellows, BallBearing, Spring2D) not yet mapped
