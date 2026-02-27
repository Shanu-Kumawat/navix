#pragma once

#include <string>
#include <vector>
#include <imgui.h>
#include "Canvas.hpp"

namespace Core {

/**
 * @brief Encapsulates all application-level state.
 * 
 * This class replaces the global `namespace UIState` previously found in main.cpp.
 * It holds the current state of the UI, active tools, view settings, and command history.
 * By passing this context object to the components that need it, we eliminate global state,
 * making the application more testable and scalable.
 */
class ApplicationContext {
public:
    ApplicationContext();
    ~ApplicationContext() = default;

    // Tool state
    Drawing::DrawingMode activeMode;
    bool snapEnabled;
    float gridSize;
    std::string consoleMessage;
    
    // Dimension settings
    bool fixedLineLength;
    bool fixedCircleRadius;
    bool fixedSquareSize;
    bool fixedTriangleSize;
    bool fixedRectangleSize;

    // 3D view settings
    bool showSpring3DView;
    bool showShockAbsorber3DView;

    // Command line
    char commandBuffer[256];
    bool focusCommandLine;
    std::vector<std::string> commandHistory;
    int commandHistoryPos;

    // View settings
    bool showRulers;
    bool showCoordinates;
    float zoomLevel;
    ImVec2 panOffset;

    // Helper methods
    void addCommandToHistory(const std::string& command);
    void setConsoleMessage(const std::string& message, bool isError = false);
    
    // Optional: Add a flag to indicate if the message is an error for UI styling
    bool isConsoleMessageError;
};

} // namespace Core
