#include "ui/UIState.hpp"

namespace UIState {
float userPropertyPanelWidth = 280.0f;

int activeLayer = 0;
std::vector<std::string> layerNames = {"Layer 0"};
std::vector<bool> layerVisibility = {true};
std::vector<ImU32> layerColors = {IM_COL32(255, 255, 255, 255)};

Drawing::UnitSystem units = Drawing::UnitSystem::Millimeters;
float unitScale = 1.0f;

std::vector<std::string> recentFiles;

std::string currentWorkspace = "2D Drafting";
bool darkMode = false;

SpringViewer3D springViewer;
bool spring3DViewInitialized = false;

bool shockAbsorber3DViewInitialized = false;

BellowsViewer3D bellowsViewer;
bool bellows3DViewInitialized = false;
BallBearingViewer3D ballBearingViewer;
bool ballBearing3DViewInitialized = false;
ShockAbsorberViewer3D shockAbsorberViewer;
bool shockAbsorberViewerInitialized = false;

ComplexShape3DManager shape3DManager;
bool showBellows3DView = false;
bool showBallBearing3DView = false;
bool showShockAbsorber3DViewUnified = false;
} // namespace UIState
