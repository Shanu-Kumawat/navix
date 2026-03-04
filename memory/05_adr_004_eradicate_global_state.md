# ADR-004: Eradicate Global State (UIState)

- **Date**: February 27, 2026
- **Context**: The application currently relies heavily on a global `namespace UIState` defined in `main.cpp`. This global state manages everything from the active drawing tool to view settings and command line history. This design pattern violates the Single Responsibility Principle, makes unit testing impossible, and prevents future scalability (e.g., supporting multiple documents or multi-threading).
- **Decision**: Introduce an `ApplicationContext` (or `Project`) class to encapsulate all application-level state. This context object will be instantiated once in `main()` and passed by reference or pointer to the components that need it (e.g., `Canvas`, UI panels).
- **Implications**:
  - `namespace UIState` will be completely removed from `main.cpp`.
  - A new `ApplicationContext.hpp` and `ApplicationContext.cpp` will be created.
  - `Canvas` and other components will need to be updated to accept a reference to `ApplicationContext`.
  - This is the first step towards a proper Model-View-Controller (MVC) architecture.
