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

## ADR-018: 3D FEM Mesh Infrastructure in Base3DModel
- **Date**: March 3, 2026
- **Context**: 3D viewers (Bellows, BallBearing, ShockAbsorber) only rendered parametric geometry. Users needed to see FEM meshes directly on 3D surfaces while drawing/viewing, similar to professional FEM software.
- **Decision**: Added FEM mesh infrastructure to `Base3DModel`: virtual `generateFEMMesh()`, mesh storage (`femNodes`, `femElements`), wireframe buffer setup (`setupMeshWireframeBuffers()`), and `renderFEMMeshWireframe()` rendering bright green wireframe (0.0, 1.0, 0.4) with full ambient. Set `showFEMMesh` default to `true`. Each 3D viewer auto-generates the FEM mesh via `if (!model->hasFEMMesh()) { model->generateFEMMesh(...); }` after `generateMesh()`.
- **Consequences**: 3D models display FEM wireframe overlay immediately upon opening viewers. The mesh is generated once and cached. Infrastructure is reusable for any future 3D model types.

## ADR-019: Bellows FEM Mesh via OCC Revolve of Profile Wire
- **Date**: March 3, 2026
- **Context**: Bellows geometry is inherently axisymmetric — a corrugated profile revolved 360° around the X-axis. Directly creating OCC solid primitives would be complex and lose the parametric profile fidelity.
- **Decision**: `BellowsModel3D::generateFEMMesh()` extracts the bellows profile points (axial position, radius), de-duplicates consecutive points within `1e-6` tolerance, creates OCC points and lines (with individual try/catch per line for robustness), builds a wire, then revolves 2π around the X-axis via `gmsh::model::occ::revolve`. Fixed "Could not create line" errors caused by near-coincident profile points.
- **Consequences**: Produces high-quality surface mesh (678 nodes, 1216 elements) that precisely matches the bellows geometry. The de-duplication and per-line error handling makes it robust against floating-point profile point clustering.

## ADR-020: ShockAbsorber Coil Spring Mesh via Stacked Tori
- **Date**: March 3, 2026
- **Context**: The shock absorber coil spring is rendered as a 3D helix. Initial approaches to mesh it — (1) BSpline helix pipe via OCC `addPipe`, (2) OCC spline with fewer points — both caused Gmsh/OCC to hang indefinitely due to the complexity of helical pipe operations.
- **Decision**: Replaced the helical pipe with stacked tori (one torus per coil). Each torus is created at origin with Z-axis orientation, rotated 90° around the X-axis to align with the Y-axis, then translated to the correct vertical position (`centerY`). This preserves the visual approximation of a coil while being computationally tractable. The full shock absorber model generates 10 OCC components: lower seat, damper tube, piston rod, tori coils, upper seat, collar, nut, cap, bottom disc, and U-mount legs.
- **Consequences**: Produces a complete shock absorber mesh (1619 nodes, 2376 elements) in reasonable time. The tori don't perfectly match the helical coil but provide a sufficiently accurate FEM mesh approximation for structural analysis. If higher fidelity is needed, a segmented approach (half-torus arcs offset per coil) could be explored.

## ADR-021: CST+DKT Shell Element Formulation for Bellows FEA
- **Date**: March 6, 2026
- **Context**: Bellows are thin-walled axisymmetric pressure vessels. A full 3D solid element approach would require extremely fine through-thickness meshing and be computationally prohibitive. Shell elements are the industry-standard choice for thin-walled structures.
- **Decision**: Implemented a combined **Constant Strain Triangle (CST)** membrane element and **Discrete Kirchhoff Triangle (DKT)** bending element, yielding 6 DOFs per node (3 translations + 3 rotations). The DKT formulation follows Batoz, Bathe & Ho (1980) with 3-point Gauss quadrature for stiffness integration. Stress recovery computes membrane + bending contributions at element centroids, then von Mises equivalent stress. Node stresses are area-weighted averages of surrounding element stresses. Files: `include/fem/ShellElement.hpp`, `src/fem/ShellElement.cpp` (~350 lines).
- **Consequences**: Accurate thin-shell structural analysis with minimal mesh requirements. The 6-DOF formulation captures both membrane stretching and plate bending behaviour. The CST membrane component is simple but sufficient for the triangular surface meshes produced by Gmsh. DKT provides Kirchhoff-accurate bending without shear locking.

## ADR-022: Eigen3 Sparse Solver with Penalty Boundary Conditions
- **Date**: March 6, 2026
- **Context**: The assembled global stiffness matrix for bellows FEM (~4000+ DOFs) is large and sparse. Boundary conditions must be enforced without destroying matrix symmetry or sparsity patterns.
- **Decision**: (1) Assembly uses `Eigen::SparseMatrix<double>` built from `Eigen::Triplet<double>` lists for efficient COO→CSC construction. (2) `Eigen::SparseLU` direct solver chosen for robustness over iterative methods. (3) Boundary conditions enforced via the **penalty method**: diagonal entries for constrained DOFs are set to `1e8 × max(diag)`, force vector entries set to `penalty × prescribed_value`. This preserves matrix structure and avoids row/column elimination. (4) Two `solve()` overloads: one accepting explicit force vector, one accepting pressure + axial force with automatic load vector assembly. Files: `include/fem/FEASolver.hpp`, `src/fem/FEASolver.cpp`.
- **Consequences**: Robust direct solver handles arbitrary mesh sizes. Penalty BCs are simple to implement and maintain sparsity. The 1e8 multiplier provides sufficient accuracy (constraint error ~1e-8 relative). SparseLU handles non-symmetric systems gracefully, supporting future extension to non-symmetric formulations.

## ADR-023: Bellows-Specific FEM Analysis Orchestrator
- **Date**: March 6, 2026
- **Context**: The bellows FEM workflow requires domain-specific steps: identifying structural boundaries (cuff A and cuff B), applying physically meaningful BCs (fixed at one cuff, loaded at the other), and converting mesh data between the Gmsh-produced format and the FEA solver input.
- **Decision**: Created `BellowsFEMAnalysis` as an orchestrator class. It (1) identifies cuff A nodes as those at the axial minimum (within 1% tolerance of total axial span), (2) identifies cuff B nodes at the axial maximum similarly, (3) applies all-DOF-fixed Dirichlet BCs at cuff A, (4) runs the solver with pressure distributed to all elements and axial force concentrated equally on cuff B nodes, (5) returns a `FEMResult` struct with displacement field, element/node stresses, and summary statistics. Files: `include/fem/BellowsFEMAnalysis.hpp`, `src/fem/BellowsFEMAnalysis.cpp`.
- **Consequences**: Clean separation between generic FEM infrastructure (`ShellElement`, `FEASolver`) and bellows-specific analysis logic. The same solver/element infrastructure can be reused for other component types (ball bearings, shock absorbers) by creating analogous orchestrators.

## ADR-024: Stress Contour Visualization with Shader-Based Vertex Colouring
- **Date**: March 6, 2026
- **Context**: Post-processing stress results need to be displayed as colour-mapped contours on the 3D model surface. The existing bellows shader pipeline renders solid-colour geometry with Phong lighting.
- **Decision**: (1) Added a `useVertexColor` integer uniform to `bellows.vs/fs`. When `useVertexColor == 1`, attribute 1 is interpreted as RGB colour (instead of normal), and the fragment shader uses flat normals via `cross(dFdx(FragPos), dFdy(FragPos))` for per-triangle shading. (2) `BellowsModel3D::setupStressBuffers()` builds a separate VAO/VBO with interleaved position+colour data, where colour is computed from node von Mises stress via a jet colourmap (blue→cyan→green→yellow→red). (3) PropertyPanel displays results (max displacement in mm, max stress in MPa, safety factor) and an interactive colour legend drawn via ImDrawList with per-pixel gradient. (4) Material selector (Steel/SS/Inconel 718) with live E, ν, yield strength display.
- **Consequences**: Professional-grade stress visualization integrated into the existing rendering pipeline with minimal shader changes. The separate stress VAO avoids interfering with the base geometry buffers. Jet colourmap is the de-facto standard in engineering FEM visualization.

## ADR-025: Deformed Shape Overlay
- **Date**: March 6, 2026
- **Context**: Engineers need to visualize structural deformations under load. Deformation magnitudes are typically small relative to geometry, so a scale factor is essential.
- **Decision**: (1) Separate `deformedVAO/VBO` with displaced node positions (original + scale × displacement). (2) Rendered as cyan wireframe (`GL_LINE` polygon mode) overlaid on undeformed geometry. (3) UI provides toggle + drag-float scale (1–5000×). (4) `setupDeformedBuffers()` called when scale changes, reusing the same VAO/VBO pattern as stress contours.
- **Consequences**: Clear visual comparison of deformed vs reference configuration. Wireframe avoids occluding stress contours. Scale factor allows examination at any deformation level.

## ADR-026: Reaction Force Computation and Display
- **Date**: March 6, 2026
- **Context**: Reaction forces at support BCs are critical for design verification (equilibrium check, support sizing).
- **Decision**: (1) `FEASolver::computeReactionForces()` reassembles K (without BC penalties) and computes R = K·u − F at constrained DOFs. (2) Results stored as `ReactionForce` structs per constrained node in `FEMResult`. (3) PropertyPanel displays total resultant (Rx, Ry, Rz, |R|) and count of support nodes.
- **Consequences**: Equilibrium can be verified in the UI. Total reaction magnitude gives an intuitive validation check against applied loads.

## ADR-027: Modal Analysis via Standard Eigenvalue Reformulation
- **Date**: March 6, 2026
- **Context**: Natural frequency analysis is essential for vibration-sensitive bellows designs. Need to solve generalised eigenvalue problem K·φ = ω²·M·φ.
- **Decision**: (1) `ShellElement::massMatrix()` returns lumped (diagonal) mass matrix: translational mass = ρ·t·A/3 per node, rotational inertia = m·t²/12. (2) `FEASolver::solveModal()` eliminates constrained DOFs, then transforms to standard form A = M^{−½}·K·M^{−½} and uses `Eigen::SelfAdjointEigenSolver`. (3) Mode shapes stored in `ModalResult` with frequency and normalized mode vector. (4) Visualized by displacing mesh with mode shape (jet-coloured by displacement magnitude), with mode selector slider and frequency table in UI.
- **Consequences**: Suitable for moderate DOF counts (Eigen dense solver). Lumped mass is unconditionally positive-diagonal, avoiding M-singularity. M^{−½} reformulation guarantees real eigenvalues for the symmetric positive-definite K, M pair.

## ADR-028: Automated Mesh Convergence Study
- **Date**: March 6, 2026
- **Context**: Mesh convergence is required to demonstrate solution independence from mesh density — a fundamental requirement for credible FEM results.
- **Decision**: (1) `BellowsModel3D::runConvergenceStudy()` iterates predefined element sizes (0.15→0.025), regenerates mesh and reruns FEM at each level. (2) Collects numNodes, numElements, maxStress (MPa), maxDisplacement (mm) per refinement level. (3) Restores original mesh after completion. (4) PropertyPanel shows results in a 4-column table (h, Nodes, σ, u).
- **Consequences**: Users can verify mesh independence systematically. Automated workflow prevents manual tedium. Restoring original mesh avoids side-effects.
