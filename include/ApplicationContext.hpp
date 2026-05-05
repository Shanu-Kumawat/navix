#include <glm/glm.hpp>
#pragma once

#include <string>
#include <vector>
#include <imgui.h>
#include "Constants.hpp"
#include "SpringViewer3D.hpp"
#include "ShockAbsorberViewer3D.hpp"
#include "BellowsViewer3D.hpp"
#include "BallBearingViewer3D.hpp"
#include "ComplexShape3DManager.hpp"
#include "modeling3d/Viewport3D.hpp"

namespace Core {

/**
 * @brief 2D/3D workspace mode for the main canvas area.
 */
enum class WorkspaceMode {
    Mode2D,  ///< Traditional 2D drafting canvas
    Mode3D   ///< 3D modeling canvas (Blender/ANSYS-like)
};

/**
 * @brief Encapsulates all application-level state.
 * 
 * Replaces the former global `namespace UIState`. Holds the current state of the UI,
 * active tools, view settings, command history, layers, units, and 3D viewers.
 * Passed by reference to components that need it, eliminating global state.
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

    // --- Formerly UIState globals ---

    // Resizable property panel width (user can drag to resize)
    float userPropertyPanelWidth;

    // Layer system
    int activeLayer;
    std::vector<std::string> layerNames;
    std::vector<bool> layerVisibility;
    std::vector<ImU32> layerColors;

    // Unit system
    Drawing::UnitSystem units;
    float unitScale;

    // Recent files
    std::vector<std::string> recentFiles;

    // Workspace settings
    std::string currentWorkspace;
    bool darkMode;

    // 2D/3D mode switch
    WorkspaceMode currentWorkspaceMode{WorkspaceMode::Mode2D};

    // 3D Modeling Viewport (lazy-initialized on first use)
    Modeling3D::Viewport3D viewport3D;
    bool viewport3DInitialized{false};

    // 3D viewer instances
    SpringViewer3D springViewer;
    bool spring3DViewInitialized;
    ShockAbsorberViewer3D shockAbsorberViewer;
    bool shockAbsorberViewerInitialized;
    BellowsViewer3D bellowsViewer;
    bool bellows3DViewInitialized;
    BallBearingViewer3D ballBearingViewer;
    bool ballBearing3DViewInitialized;

    // Unified 3D Manager for all complex shapes
    ComplexShape3DManager shape3DManager;

    // 3D view show flags
    bool showSpring3DView;
    bool showShockAbsorber3DView;
    bool showBellows3DView;
    bool showBallBearing3DView;
    bool showShockAbsorber3DViewUnified;

    // FEM Analysis view flag
    bool showFEMAnalysisView;

    // Command line
    char commandBuffer[256];
    bool focusCommandLine;
    std::vector<std::string> commandHistory;
    int commandHistoryPos;

    // View settings
    bool showRulers;
    bool showCoordinates;
    float zoomLevel;
    glm::dvec2 panOffset;

    // Helper methods
    void addCommandToHistory(const std::string& command);
    void setConsoleMessage(const std::string& message, bool isError = false);
    
    // Flag to indicate if the message is an error for UI styling
    bool isConsoleMessageError;
};

} // namespace Core
