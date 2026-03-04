#include "ApplicationContext.hpp"
#include <cstring>

namespace Core {

ApplicationContext::ApplicationContext()
    : activeMode(Drawing::DrawingMode::Select),
      snapEnabled(true),
      gridSize(Drawing::Constants::DEFAULT_GRID_SPACING),
      consoleMessage("Ready"),
      fixedLineLength(false),
      fixedCircleRadius(false),
      fixedSquareSize(false),
      fixedTriangleSize(false),
      fixedRectangleSize(false),
      // Formerly UIState globals
      userPropertyPanelWidth(280.0f),
      activeLayer(0),
      layerNames({"Layer 0"}),
      layerVisibility({true}),
      layerColors({IM_COL32(255, 255, 255, 255)}),
      units(Drawing::UnitSystem::Millimeters),
      unitScale(1.0f),
      currentWorkspace("2D Drafting"),
      darkMode(false),
      spring3DViewInitialized(false),
      shockAbsorberViewerInitialized(false),
      bellows3DViewInitialized(false),
      ballBearing3DViewInitialized(false),
      // 3D view show flags
      showSpring3DView(false),
      showShockAbsorber3DView(false),
      showBellows3DView(false),
      showBallBearing3DView(false),
      showShockAbsorber3DViewUnified(false),
      // Command line
      focusCommandLine(false),
      commandHistoryPos(-1),
      // View settings
      showRulers(true),
      showCoordinates(true),
      zoomLevel(1.0f),
      panOffset(0.0f, 0.0f),
      isConsoleMessageError(false)
{
    std::memset(commandBuffer, 0, sizeof(commandBuffer));
}

void ApplicationContext::addCommandToHistory(const std::string& command) {
    if (command.empty()) return;
    
    // Don't add if it's the same as the last command
    if (commandHistory.empty() || commandHistory.back() != command) {
        commandHistory.push_back(command);
    }
    
    // Reset history position
    commandHistoryPos = -1;
}

void ApplicationContext::setConsoleMessage(const std::string& message, bool isError) {
    consoleMessage = message;
    isConsoleMessageError = isError;
}

} // namespace Core
