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
