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
