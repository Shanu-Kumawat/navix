# Identified Issues & Technical Debt

This document outlines the critical architectural and code quality issues that must be addressed before advancing NAVIX to support meshing and FEA solvers.

## 1. Architectural Weaknesses & Design Flaws
- **God Objects**: `main.cpp` (2,456 lines) and `Canvas.cpp` (3,482 lines) handle too many responsibilities (UI, rendering, input, state, geometry). This violates the Single Responsibility Principle (SRP) and makes the code fragile.
- **Tight Coupling with UI**: Core geometric and physical logic directly uses `ImGui::GetIO()` and `ImGui::GetMousePos()`. The business logic has zero independence from the UI framework.
- **Global State**: The use of `namespace UIState` in `main.cpp` makes unit testing impossible and prevents future scalability (e.g., multi-threading, multiple documents).
- **Inadequate Data Structures for FEM**: Shapes are stored as a flat `std::vector<std::unique_ptr<Shape>>`. FEM requires topological relationships (Nodes, Edges, Faces, Volumes) to generate meshes and assemble stiffness matrices.

## 2. Code Quality & Maintainability Concerns
- **Precision Issues (Critical for FEA)**: The codebase heavily relies on 32-bit `float` (via `ImVec2` and custom math). FEM requires 64-bit `double` precision to prevent catastrophic cancellation and numerical instability during matrix inversion and stiffness assembly.
- **Reinventing the Wheel**: Custom implementations for Splines, Bezier curves, and basic vector math (`MathUtils.hpp`, `VectorMath.hpp`) are prone to edge-case bugs and lack the robustness of established geometry libraries.
- **Direct OpenGL Calls**: 3D viewers (`Base3DViewer`, etc.) mix business logic with raw OpenGL calls, complicating future rendering enhancements (e.g., stress contour mapping).

## 3. Missing Foundations for FEM
- **No Topological Data Structure**: Lack of a Half-Edge or similar data structure to store and query adjacent elements.
- **No Command Pattern**: Actions are not encapsulated, making robust Undo/Redo and state management difficult.
- **No Separation of Concerns**: Lack of a clear Model-View-Controller (MVC) or Model-View-ViewModel (MVVM) architecture.
