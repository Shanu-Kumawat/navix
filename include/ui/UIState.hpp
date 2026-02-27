#pragma once

#include <vector>
#include <string>
#include <imgui.h>
#include "Constants.hpp"
#include "SpringViewer3D.hpp"
#include "ShockAbsorberViewer3D.hpp"
#include "BellowsViewer3D.hpp"
#include "BallBearingViewer3D.hpp"
#include "ComplexShape3DManager.hpp"
#include "ApplicationContext.hpp"

namespace UIState {
// Resizable property panel width (user can drag to resize)
extern float userPropertyPanelWidth;

// Layer system
extern int activeLayer;
extern std::vector<std::string> layerNames;
extern std::vector<bool> layerVisibility;
extern std::vector<ImU32> layerColors;

// Unit system
extern Drawing::UnitSystem units;
extern float unitScale;

// Recent files
extern std::vector<std::string> recentFiles;

// Workspace settings
extern std::string currentWorkspace;
extern bool darkMode;

// Add a global or static instance for the spring 3D viewer
extern SpringViewer3D springViewer;
extern bool spring3DViewInitialized;
// Add a flag for the new 3D Shock Absorber viewer
extern bool shockAbsorber3DViewInitialized;

// 3D Viewers for Bellows, Ball Bearing, and Shock Absorber
extern BellowsViewer3D bellowsViewer;
extern bool bellows3DViewInitialized;
extern BallBearingViewer3D ballBearingViewer;
extern bool ballBearing3DViewInitialized;
extern ShockAbsorberViewer3D shockAbsorberViewer;
extern bool shockAbsorberViewerInitialized;

// Unified 3D Manager for all complex shapes
extern ComplexShape3DManager shape3DManager;
extern bool showBellows3DView;
extern bool showBallBearing3DView;
extern bool showShockAbsorber3DViewUnified;
} // namespace UIState
