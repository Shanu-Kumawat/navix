# Architecture Decision Records (ADR)

This document logs all major architectural decisions, design changes, and subsystem modifications for NAVIX.

## ADR-001: Establish External Memory Layer
- **Date**: February 26, 2026
- **Context**: The project is transitioning from a basic drawing tool to a complex FEM software. AI agents and developers need a persistent, authoritative source of truth to maintain context across sessions.
- **Decision**: Create a `memory/` directory containing Markdown files that accurately reflect the current state, architecture, issues, and action plans of NAVIX.
- **Implications**: All major decisions, refactors, and architectural changes MUST be documented here before implementation. This ensures continuity and prevents context loss.

## ADR-002: Transition to Double Precision (64-bit)
- **Date**: February 26, 2026
- **Context**: The current codebase uses 32-bit `float` (via `ImVec2` and custom math). FEM requires high numerical stability during matrix inversion and stiffness assembly.
- **Decision**: Replace all core geometric and physical math with 64-bit `double` precision (e.g., using `glm::dvec2` and `glm::dvec3`). `ImVec2` will be restricted strictly to the UI rendering layer.
- **Implications**: Requires a comprehensive refactor of `Canvas`, `shapes/`, and `utils/`.

## ADR-003: Decouple UI from Core Logic (MVC/MVVM)
- **Date**: February 26, 2026
- **Context**: `main.cpp` and `Canvas.cpp` are God objects tightly coupled with ImGui and SDL2. Global state (`UIState`) is used extensively.
- **Decision**: Adopt a strict separation of concerns. The core Model (geometry, physics) must have zero knowledge of ImGui or OpenGL. Controllers will handle input, and Views will handle rendering. Global state will be encapsulated in an `ApplicationContext` or `Project` class.
- **Implications**: Requires breaking down `main.cpp` and `Canvas.cpp` into smaller, focused classes.

## ADR-006: Removal of OpenNURBS Dependency
- **Date**: February 28, 2026
- **Context**: The  library was present in the external directory but currently unused within the codebase or the build loop. As the focus shifts to internal topological representations prior to Gmsh injection, keeping an unused massive library adds unnecessary complexity.
- **Decision**: Remove the  submodule reference entirely to clean up the architecture (). If NURBS are required later, a leaner or better-integrated solution will be evaluated.
- **Implications**: Reduces repository weight and simplifies the build/vendor dependencies.

## ADR-006: Removal of OpenNURBS Dependency
- **Date**: February 28, 2026
- **Context**: The `OpenNURBS` library was present in the external directory but currently unused within the codebase or the build loop. As the focus shifts to internal topological representations prior to Gmsh injection, keeping an unused massive library adds unnecessary complexity.
- **Decision**: Remove the `OpenNURBS` submodule reference entirely to clean up the architecture (Phase 1, Priority 5). If NURBS are required later, a leaner or better-integrated solution will be evaluated.
- **Implications**: Reduces repository weight and simplifies the build/vendor dependencies.

## ADR-007: Command Pattern for Undo/Redo Engine
- **Context**: Transitioning from a primitive snapshot history list to a modular Command pattern architecture required scaffolding `CommandManager` alongside a baseline polymorphic `Command` structure.
- **Decision**: Implemented `CommandManager` maintaining independent undo/redo `std::vector<std::unique_ptr<Core::Commands::Command>>` stacks. A bridge command named `CanvasSnapshotCommand` was injected strictly into the legacy global swap mechanism (`saveToHistory()`) to establish the architectural foundation while minimizing logical risk.
- **Consequences**: Immediate robust structural backing for all action-based logic, allowing granular actions (e.g. `TranslateNodeCommand`) to be adopted naturally when Topological models roll-out in Phase 2. The legacy naive snapshot loop now seamlessly integrates across the new `CommandManager` ecosystem as a single `Command` variant.

## ADR-008: Topological Data Structures (`Node`, `Edge`, `Face`)
- **Context**: Transitioning towards a robust FEM (Finite Element Method) ecosystem requires dropping ad-hoc parameter arrays inside generic shapes in favor of mathematically addressable boundary-representation (B-REP) meshes. `Node`, `Edge`, and `Face` relationships are strictly required for tools like Gmsh.
- **Decision**: Developed `Core::Topology::Entity` abstract class alongside exact boundary classifications (`Node`, `Edge`, `Face`). Implemented `Core::Topology::TopologyManager` serving as a centralized ID factory and relationship dictionary across the active context. The `TopologyManager` currently exists globally on `Canvas` paralleling `std::vector<Shape>` to enable graceful migration.
- **Consequences**: Shapes natively transition towards pulling vertex data dynamically from `TopologyManager->getNode(id)` logic. Enables direct serialization of nodes to Gmsh C++ API geometries (Phase 3). Legacy explicit positioning logic inside `.cpp` files will progressively decay as shapes reference `Node` IDs instead.

## ADR-009: Renderer Virtual Interface Pattern
- **Context**: Drawing components (like `Canvas` and `Renderer2D`) were tightly coupled to `ImGui/ImDrawList` types. Moving toward drawing generated Finite Elements (Nodes, Gaussian quads) alongside existing `Drawing::Shape` components necessitates a unified pipeline where the generic objects don't explicitly command ImGui directly.
- **Decision**: Implemented `Core::Renderer` pure abstract class mapping raw visual primitives (`drawLine`, `drawCircle`) and contextual functions (`drawNode()`, `drawEdge()`). Provided an initial concrete target via `Core::Graphics::ImGuiRenderer`. `Renderer2D` logic will gradually transpose standard `ImDrawList*` operations onto `Renderer*` pointers.
- **Consequences**: Future rendering abstraction is assured. Should the underlying engine require pure OpenGL native calls, or Vulcan frameworks for large dense topological meshes, the `Renderer` derivative is completely interchangeable. The active layout sets the translation state centrally (handling pan/zoom matrix projections internally).

## ADR-010: Integrating Local Gmsh Build
- **Context**: Relying on package managers across distributions (Ubuntu `apt`, Arch `pacman`/`yay`) introduces non-deterministic outcomes and potential root/sudo requirements when linking complex finite-element SDKs.
- **Decision**: Curated a deterministic local dependency mapping inside `cmake/FindGmsh.cmake`. We downloaded `gmsh-4.13.1-Linux64-sdk` directly and mapped `GMSH_SEARCH_PATHS` locally to `external/gmsh`. 
- **Consequences**: Avoids corrupting system states on target machines. `USE_GMSH` configuration flag enables modular compilation, injecting the SDK precisely when the dynamic objects (`libgmsh.so`) are successfully mapped.

## ADR-010: Gmsh SDK Local Fallback & CMake Integration
- **Context**: Navix requires robust finite-element meshing. Gmsh is the industry standard open-source meshing API. Since package availability across distributions varies (e.g. Arch uses AUR), forcing `sudo apt install libgmsh-dev` makes bootstrapping difficult on certain Linux systems and Windows MSYS2 wrappers.
- **Decision**: Curated a `FindGmsh.cmake` custom module bridging direct system-linked paths and a prioritized local `./external/gmsh` vendored SDK location. Created a generic `GmshIntegration` static singleton class binding `<gmsh.h>` dynamically to `TopologyManager` if `USE_GMSH` is present.
- **Consequences**: Navix compiles on systems without root access instantly. Meshing behaves modularly: if the engine doesn't find Gmsh SDK headers or libraries it prints a warning and falls back gracefully strictly continuing its drawing runtime without solver states.

## ADR-010: Integrating Gmsh SDK Locally
- **Context**: Relying on system package managers across various Linux/Windows distributions is volatile for complex dependencies like `gmsh`. Furthermore, Arch Linux hosts it exclusively on the AUR, complicating CI checks. 
- **Decision**: Established a hybrid localized/system `CMake FindModule` inside `cmake/FindGmsh.cmake`. Automatically scans `./external/gmsh` first, then falls back to `/usr/lib` paths. Provided the `Gmsh 4.13.1` Linux SDK statically mapped inside the `external/` directory.
- **Consequences**: CMake executes robustly across cloned environments containing the `external/gmsh` archive without strictly mandating root administrator overrides. Development shifts immediately to interfacing with `gmsh.h` strictly via `#ifdef USE_GMSH` compiler guards.

## ADR-011: Topological to OCC C++ Translation Layer
- **Context**: The `Core::Topology::TopologyManager` holds agnostic engineering objects (Nodes, Edges, Faces). Gmsh requires these models fed exclusively into its OpenCASCADE (OCC) kernel dynamically before a mesh can be generated.
- **Decision**: Created `Core::Meshing::GmshTranslator`, acting as a bridge specifically compiling topology variables to Gmsh CAD structures via `gmsh::model::occ`. `Nodes` cast to Points, `Edges` to Lines, and `Faces` assemble via explicit `CurveLoops` bounding `PlaneSurfaces`.
- **Consequences**: Navix topology definitions remain decoupled independently in `/topology` while `/meshing` completely abstracts third-party `gmsh.h` imports exclusively via `#ifdef USE_GMSH` compiler blocks. Prevents external library rot into main UI structures. The user interface does not map meshing paths directly; it acts only on `GmsTranslator.translateTopologyToGmsh()`.
ADR-012: Phase 3 (Meshing) Data Extraction
Context: Standardizing outputs after meshing from Gmsh. Directly exporting native unstructured data.
Decision: Created Core::Meshing::GmshExtractor resolving gmsh outputs dynamically back into explicit Mesh structs (MeshNode, MeshElement).
Consequences: FEA and Rendering decoupled securely from standard Gmsh memory blocks. Meshing output is completely portable inside application topology variables.
ADR-013: Rendering Extracted Mesh
Context: Displaying parsed mesh data directly on the main viewport seamlessly mapped over B-REP elements.
Decision: Extended pure abstract Core::Renderer and specifically the Graphic::ImGuiRenderer backends to accept Meshing::Mesh inputs, rendering the explicit edge bounds of parsed unstructured Gmsh 2D outputs.
Consequences: Users can dynamically click buttons inside the active UI layout to push Topology, trigger Meshing explicitly, and retrieve the graphic outputs correctly overlaid against user shapes allowing rapid iteration/review loops directly within the Navix interface.

## ADR-014: Eigen3 Integration for FEA Solver
- **Date**: February 28, 2026
- **Context**: Navix requires a highly performant and stable linear algebra library to solve finite element sparse matrices structurally (stiffness matrix inversion).
- **Decision**: Integrated **Eigen3** via \CMakeLists.txt\. The script first looks for the standard system paths using \ind_package(Eigen3 QUIET)\. If it's isolated or not available (e.g. on Windows without proper paths), it securely falls back to CMake's \FetchContent\ to pull the exact \3.4.0\ headers gracefully. 
- **Consequences**: Zero required system dependencies. Ensures Phase 4 operations can directly include \<Eigen/Dense>\ and \<Eigen/Sparse>\ across the workspace automatically. Fast setup since Eigen handles headers locally without compiling complex targets.

## ADR-015: Material Manager Subsystem (Phase 4)
- **Date**: February 28, 2026
- **Context**: FEA solvers strictly require physical material parameters (Youngs Modulus, Poisson's Ratio, Density) mapped against discretized elements.
- **Decision**: Implemented \Core::FEM::MaterialManager\ mapped globally against \Canvas\ for accessibility. A Material holds baseline mechanical profiles. \Shape\ boundaries and explicit \Face\ topological entities both received a \uint32_t materialId\. 
- **Consequences**: Shapes on the 2D Viewport can have standard engineering materials assigned directly via the property panel. When \GmshTranslator\ pulls from these properties in future Phase 4 steps, elements generated across a Face will natively inherit this structural physical identity ensuring clean separation of FEA variables and pure geometry.

## ADR-016: Phase 2 Completion — Granular Commands, Topology Mapping, Unified Renderer
- **Date**: March 2, 2026
- **Context**: Phase 2 was partially complete. Three legacy operations (duplicate/rotate/scale) still used full-state CanvasSnapshotCommand. Topology had no shape↔entity mapping and wasn't synced on delete/move/rotate/scale. Renderer2D and Core::Renderer were parallel unrelated hierarchies.
- **Decision**: (1) Created `DuplicateShapeCommand`, `RotateShapeCommand`, `ScaleShapeCommand` as header-only granular commands — all support undo/redo by applying inverse operations. `duplicateSelectedShape()` now uses clone() + DuplicateShapeCommand. `rotateSelectedShape()` / `scaleSelectedShape()` compute bounding-box center and delegate to granular commands. Legacy CanvasSnapshotCommand retained ONLY for `clearAll()`. (2) Added `Canvas::ShapeTopology` struct and `shapeTopoMap` to track which topology IDs belong to each shape. `createTopologyForShape()` now records mapping for 8 types (added Spline/Bezier). Added `removeTopologyForShape()` (faces→edges→nodes) and `updateTopologyPositions()` (syncs node positions from shape geometry). Delete/move/rotate/scale all sync topology. (3) Made `Renderer2D : public Core::Renderer`. Implemented all 10 pure virtuals with ImDrawList backend. Added `beginFrame()` / `toScreen()` / transform state. `drawShape()` delegates to existing `renderShape()`.
- **Consequences**: All shape operations now have proper undo/redo via granular commands. Topology stays in sync with shape geometry at all times. A single unified renderer interface allows polymorphic rendering — both `ImGuiRenderer` and `Renderer2D` can be used interchangeably via `Core::Renderer*`.

## ADR-017: Phase 3 Completion — Gmsh SDK Integration, Meshing Pipeline, and UI
- **Date**: March 1, 2026
- **Context**: Phase 3 was scaffolding-only: headers existed but `libgmsh.so` was missing, `USE_GMSH` was never defined, and no UI code triggered meshing. `GmshTranslator.cpp` had misplaced `#include` directives inside function bodies. A legacy `GmshIntegration` class duplicated `GmshTranslator` poorly (only translated nodes, not edges/faces).
- **Decision**: (1) Downloaded and installed Gmsh 4.13.1 Linux SDK into `external/gmsh/lib/`. CMake now detects it and defines `USE_GMSH`. (2) Fixed `GmshTranslator.cpp` — removed 5 misplaced `#include` directives from inside function bodies. Fixed duplicate include in `.hpp`. Made `extractGeneratedMesh()` public. (3) Removed `GmshIntegration` from the build (legacy duplicate). (4) Added meshing pipeline to Canvas: `generateMesh()` orchestrates init→translate→generate→extract→finalize. `clearMesh()` and `hasMesh()` manage state. `Mesh` object stored as `currentMesh`. (5) Added "Mesh" panel to TopRibbon with Generate, Clear, Show toggle, Element Size slider. (6) Canvas::render() now draws mesh wireframe overlay via `Renderer2D::drawMesh()` when `showMesh` is true and mesh is non-empty.
- **Consequences**: Full meshing pipeline from drawing to mesh visualization is operational. Users can draw closed shapes (triangles, squares, rectangles), click Generate, and see the FEM mesh overlay. Phase 4 (FEA solver) can now consume `Mesh` data directly.
