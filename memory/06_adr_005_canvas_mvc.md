# ADR-005: Break Down Canvas into MVC Components

- **Date**: February 27, 2026
- **Context**: `Canvas.cpp` is a massive God object (over 3,400 lines) that handles input processing, rendering, state management, and geometric logic. This violates the Single Responsibility Principle (SRP), makes the code fragile, and hinders the integration of advanced FEM features (like meshing and solvers).
- **Decision**: Refactor the `Canvas` class into a Model-View-Controller (MVC) architecture.
  - **`SceneModel` (Model)**: Manages the collection of shapes, selection state, and history (Undo/Redo). It will be completely independent of UI and rendering logic.
  - **`Renderer2D` (View)**: Handles all ImGui/ImDrawList rendering logic, including drawing the grid, shapes, control points, and bounding boxes.
  - **`InputController` (Controller)**: Processes mouse and keyboard input, updating the `SceneModel` accordingly.
  - **`Canvas` (Facade)**: To minimize immediate disruption to `main.cpp`, `Canvas` will temporarily act as a Facade/Coordinator that holds instances of the Model, View, and Controller.
- **Implications**:
  - Significant code movement from `Canvas.cpp` to new files (`SceneModel.hpp/cpp`, `Renderer2D.hpp/cpp`, `InputController.hpp/cpp`).
  - This will be done incrementally to ensure stability and maintain a working build at each step.
  - This paves the way for replacing the flat shape list with a proper topological data structure in Phase 2.
