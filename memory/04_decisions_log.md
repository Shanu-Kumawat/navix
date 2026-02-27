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
