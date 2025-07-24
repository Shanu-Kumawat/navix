#include "Canvas.hpp"
#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>
#include <iostream>
#include "BellowsViewer3D.hpp"
#include "BallBearingViewer3D.hpp"

// Replace icon definitions with text labels
#define ICON_LINE "Line"
#define ICON_CIRCLE "Circle"
#define ICON_RECTANGLE "Rect"
#define ICON_POINT "Point"
#define ICON_TRIANGLE "Tri"
#define ICON_SQUARE "Square"
#define ICON_SPLINE "Spline"
#define ICON_BEZIER "Bezier"
#define ICON_SELECT "Select"
#define ICON_UNDO "Undo"
#define ICON_REDO "Redo"
#define ICON_PAN "Pan"
#define ICON_ZOOM "Zoom"
#define ICON_MEASURE "Measure"
#define ICON_ANNOTATION "Text"

// Window dimensions
const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;

// UI Colors - Professional Light Theme
namespace UIColors {
// Professional light theme with teal/blue-green accents
const ImVec4 BACKGROUND = ImVec4(0.98f, 0.98f, 0.98f, 1.0f);             // Almost white background
const ImVec4 PANEL = ImVec4(0.96f, 0.96f, 0.96f, 1.0f);                  // Very light gray panels
const ImVec4 DARK_PANEL = ImVec4(0.92f, 0.92f, 0.92f, 1.0f);             // Subtle darker gray for contrast
const ImVec4 HEADER = ImVec4(0.18f, 0.69f, 0.69f, 1.0f);                 // Teal header
const ImVec4 BORDER = ImVec4(0.82f, 0.82f, 0.82f, 1.0f);                 // Light gray borders
const ImVec4 TEXT = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);                   // Near-black text for contrast
const ImVec4 TEXT_DIM = ImVec4(0.50f, 0.50f, 0.50f, 1.0f);               // Medium gray dimmed text
const ImVec4 BUTTON = ImVec4(0.94f, 0.94f, 0.94f, 1.0f);                 // Light gray buttons
const ImVec4 BUTTON_HOVERED = ImVec4(0.90f, 0.90f, 0.90f, 1.0f);         // Slightly darker when hovered
const ImVec4 BUTTON_ACTIVE = ImVec4(0.18f, 0.69f, 0.69f, 1.0f);          // Teal when active
const ImVec4 BUTTON_TEXT = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);            // Dark button text
const ImVec4 ACCENT = ImVec4(0.18f, 0.69f, 0.69f, 1.0f);                 // Teal accent color
const ImVec4 TAB_ACTIVE = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);                // White active tab background
const ImVec4 GRID_BACKGROUND = ImVec4(0.96f, 0.96f, 0.96f, 1.0f);        // Very light background for grid area
const ImVec4 COMMAND_BG = ImVec4(0.90f, 0.90f, 0.90f, 1.0f);             // Light gray command line background
const ImVec4 COMMAND_TEXT = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);           // Dark command line text
const ImVec4 SUCCESS = ImVec4(0.20f, 0.70f, 0.50f, 1.0f);                // Green-teal for success/confirmation
const ImVec4 WARNING = ImVec4(0.90f, 0.65f, 0.20f, 1.0f);                // Orange for warnings
const ImVec4 ERROR = ImVec4(0.85f, 0.25f, 0.20f, 1.0f);                  // Red for errors
} // namespace UIColors

// Global state for active tool and settings
namespace UIState {
// Tool state
static Drawing::DrawingMode activeMode = Drawing::DrawingMode::Select;
static bool snapEnabled = true;
static float gridSize = Drawing::Constants::DEFAULT_GRID_SPACING;
static std::string consoleMessage = "Ready";
static bool fixedLineLength = false;  // Set to false by default for dynamic line length
static bool fixedCircleRadius = false;  // Set to false by default for dynamic circle radius
static bool fixedSquareSize = false;  // Set to false by default for dynamic square size
static bool fixedTriangleSize = false;  // Set to false by default for dynamic triangle size
static bool fixedRectangleSize = false;  // Set to false by default for dynamic rectangle size

// 3D view settings
static bool show3DView = false;
static bool bellows3DViewInitialized = false;
static bool ballBearing3DViewInitialized = false;

// Command line
static char commandBuffer[256] = "";
static bool focusCommandLine = false;
static std::vector<std::string> commandHistory;
static int commandHistoryPos = -1;

// View settings
static bool showRulers = true;
static bool showCoordinates = true;
static float zoomLevel = 1.0f;
static ImVec2 panOffset = ImVec2(0.0f, 0.0f);

// Layer system
static int activeLayer = 0;
static std::vector<std::string> layerNames = {"Layer 0"};
static std::vector<bool> layerVisibility = {true};
static std::vector<ImU32> layerColors = {IM_COL32(255, 255, 255, 255)};

// Unit system
static Drawing::UnitSystem units = Drawing::UnitSystem::Millimeters;
static float unitScale = 1.0f;

// Recent files
static std::vector<std::string> recentFiles;

// Workspace settings
static std::string currentWorkspace = "2D Drafting";
static bool darkMode = false;
} // namespace UIState

// Function declaration prototypes
void SelectTool(Drawing::DrawingMode mode, Drawing::Canvas &canvas, const std::string &message);
void SetupImGuiStyle();
void RenderTopRibbon(Drawing::Canvas &canvas);
void RenderPropertyPanel(Drawing::Canvas &canvas);
void RenderStatusBar(Drawing::Canvas &canvas);
void RenderCanvas(Drawing::Canvas &canvas);
void HandleKeyboardShortcuts(Drawing::Canvas &canvas);

// Forward declare the 3D view functions
void Render3DViewWindow(Drawing::Canvas &canvas);
void Handle3DViewerInput(const SDL_Event& event);

// Helper function to handle tool selection
void SelectTool(Drawing::DrawingMode mode, Drawing::Canvas &canvas,
                const std::string &message) {
  UIState::activeMode = mode;
  canvas.setDrawingMode(mode);
  UIState::consoleMessage = message;
}

void SetupImGuiStyle() {
  ImGuiStyle &style = ImGui::GetStyle();

  // Colors - Professional Light Theme
  ImVec4 *colors = style.Colors;
  colors[ImGuiCol_Text] = UIColors::TEXT;
  colors[ImGuiCol_TextDisabled] = UIColors::TEXT_DIM;
  colors[ImGuiCol_WindowBg] = UIColors::PANEL;
  colors[ImGuiCol_ChildBg] = UIColors::PANEL;
  colors[ImGuiCol_PopupBg] = UIColors::PANEL;
  colors[ImGuiCol_Border] = UIColors::BORDER;
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_FrameBg] = UIColors::DARK_PANEL;
  colors[ImGuiCol_FrameBgHovered] = UIColors::BUTTON_HOVERED;
  colors[ImGuiCol_FrameBgActive] = UIColors::BUTTON_ACTIVE;
  colors[ImGuiCol_TitleBg] = UIColors::HEADER;
  colors[ImGuiCol_TitleBgActive] = UIColors::HEADER;
  colors[ImGuiCol_TitleBgCollapsed] = UIColors::HEADER;
  colors[ImGuiCol_MenuBarBg] = UIColors::DARK_PANEL;
  colors[ImGuiCol_ScrollbarBg] = UIColors::DARK_PANEL;
  colors[ImGuiCol_ScrollbarGrab] = UIColors::BUTTON;
  colors[ImGuiCol_ScrollbarGrabHovered] = UIColors::BUTTON_HOVERED;
  colors[ImGuiCol_ScrollbarGrabActive] = UIColors::BUTTON_ACTIVE;
  colors[ImGuiCol_CheckMark] = UIColors::ACCENT;
  colors[ImGuiCol_SliderGrab] = UIColors::ACCENT;
  colors[ImGuiCol_SliderGrabActive] = UIColors::ACCENT;
  colors[ImGuiCol_Button] = UIColors::BUTTON;
  colors[ImGuiCol_ButtonHovered] = UIColors::BUTTON_HOVERED;
  colors[ImGuiCol_ButtonActive] = UIColors::BUTTON_ACTIVE;
  colors[ImGuiCol_Header] = UIColors::HEADER;
  colors[ImGuiCol_HeaderHovered] = UIColors::BUTTON_HOVERED;
  colors[ImGuiCol_HeaderActive] = UIColors::BUTTON_ACTIVE;
  colors[ImGuiCol_Separator] = UIColors::BORDER;
  colors[ImGuiCol_SeparatorHovered] = UIColors::BORDER;
  colors[ImGuiCol_SeparatorActive] = UIColors::BORDER;
  colors[ImGuiCol_Tab] = UIColors::BUTTON;
  colors[ImGuiCol_TabHovered] = UIColors::BUTTON_HOVERED;
  colors[ImGuiCol_TabActive] = UIColors::BUTTON_ACTIVE;
  colors[ImGuiCol_TabUnfocused] = UIColors::DARK_PANEL;
  colors[ImGuiCol_TabUnfocusedActive] = UIColors::BUTTON;
  colors[ImGuiCol_PlotLines] = UIColors::ACCENT;
  colors[ImGuiCol_PlotLinesHovered] = UIColors::ACCENT;
  colors[ImGuiCol_PlotHistogram] = UIColors::ACCENT;
  colors[ImGuiCol_PlotHistogramHovered] = UIColors::ACCENT;
  colors[ImGuiCol_TableHeaderBg] = UIColors::DARK_PANEL;
  colors[ImGuiCol_TableBorderStrong] = UIColors::BORDER;
  colors[ImGuiCol_TableBorderLight] = UIColors::BORDER;
  colors[ImGuiCol_TableRowBg] = UIColors::PANEL;
  colors[ImGuiCol_TableRowBgAlt] = UIColors::DARK_PANEL;

  // Styles - Modern Light Theme
  style.WindowPadding = ImVec2(10, 10);
  // Increase frame padding for button text to have more space around it
  style.FramePadding = ImVec2(10, 8);
  style.CellPadding = ImVec2(6, 4);
  style.ItemSpacing = ImVec2(10, 8);
  style.ItemInnerSpacing = ImVec2(8, 6);
  style.TouchExtraPadding = ImVec2(2, 2);
  style.IndentSpacing = 22;
  style.ScrollbarSize = 14;
  style.GrabMinSize = 14;

  // Borders and rounding
  style.WindowBorderSize = 1;
  style.ChildBorderSize = 1;
  style.PopupBorderSize = 1;
  style.FrameBorderSize = 1;
  style.TabBorderSize = 0;

  style.WindowRounding = 6;
  style.ChildRounding = 4;
  style.FrameRounding = 4;
  style.PopupRounding = 4;
  style.ScrollbarRounding = 4;
  style.GrabRounding = 4;
  style.TabRounding = 4;
  
  // Window title alignment
  style.WindowTitleAlign = ImVec2(0.5f, 0.5f); // Center window titles
  
  // Extra style tweaks
  style.DisplaySafeAreaPadding = ImVec2(8, 8);
  style.DisplayWindowPadding = ImVec2(8, 8);
  
  // More button-specific style adjustments
  style.ButtonTextAlign = ImVec2(0.5f, 0.5f); // Center text in buttons
}

void RenderTopRibbon(Drawing::Canvas &canvas) {
  // Professional style ribbon at the top
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, 110));

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 6));
  ImGui::Begin("Ribbon", nullptr,
               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

  // Calculate adaptive panel and button widths based on screen size
  const float availWidth = ImGui::GetContentRegionAvail().x;
  const float panelSpacing = 8.0f;
  
  // Total width minus space for 3D button (120px) divided by 3 panels
  const float panelWidth = (availWidth - 130 - panelSpacing*2) / 3;
  
  // Calculate button sizes based on panel width and content
  const float panelHeight = 120.0f;
  
  // Measure standard text size for a button
  const float textWidth = ImGui::CalcTextSize("Rectangle").x;
  // Ensure button width is at least text + padding
  const float minButtonWidth = textWidth + 16.0f; 
  // Calculate how many buttons per row based on available width
  const int buttonsPerRow = std::max(2, static_cast<int>(panelWidth / (minButtonWidth + 8.0f)));
  // Calculate button width based on available space and buttons per row
  const float buttonWidth = (panelWidth - (buttonsPerRow-1)*8.0f - 16.0f) / buttonsPerRow;
  // Fixed button height
  const float buttonHeight = 30.0f;
  const float buttonSpacing = 8.0f;
  
  // Draw Panel
  ImGui::BeginGroup();
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.98f, 0.98f, 0.98f, 1.0f));
  ImGui::BeginChild("DrawPanel", ImVec2(panelWidth, panelHeight), true);
  
  ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
  ImGui::PushStyleColor(ImGuiCol_Text, UIColors::TEXT_DIM);
  ImGui::Text("Draw");
  ImGui::PopStyleColor();
  ImGui::PopFont();
  
  ImGui::Dummy(ImVec2(0, 2));
  
  // Row 1: Basic drawing tools - use buttonWidth instead of smallBtnSize
  float y = ImGui::GetCursorPosY();
  bool selected;
  
  selected = UIState::activeMode == Drawing::DrawingMode::Point;
  if (ImGui::Button(ICON_POINT, ImVec2(buttonWidth, buttonHeight))) {
    SelectTool(Drawing::DrawingMode::Point, canvas, "Point Tool: Click to place points");
  }
  
  ImGui::SameLine(0, buttonSpacing);
  selected = UIState::activeMode == Drawing::DrawingMode::Line;
  if (ImGui::Button(ICON_LINE, ImVec2(buttonWidth, buttonHeight))) {
    SelectTool(Drawing::DrawingMode::Line, canvas, "Line Tool: Click to set start and end points");
  }
  
  if (buttonsPerRow > 2) {
    ImGui::SameLine(0, buttonSpacing);
  } else {
    ImGui::Dummy(ImVec2(0, 3));
  }
  
  selected = UIState::activeMode == Drawing::DrawingMode::Circle;
  if (ImGui::Button(ICON_CIRCLE, ImVec2(buttonWidth, buttonHeight))) {
    SelectTool(Drawing::DrawingMode::Circle, canvas, "Circle Tool: Click to set center and radius");
  }

  if (buttonsPerRow > 3) {
    ImGui::SameLine(0, buttonSpacing);
  } else {
    ImGui::Dummy(ImVec2(0, 3));
  }
  
  selected = UIState::activeMode == Drawing::DrawingMode::Triangle;
  if (ImGui::Button(ICON_TRIANGLE, ImVec2(buttonWidth, buttonHeight))) {
    SelectTool(Drawing::DrawingMode::Triangle, canvas, "Triangle Tool: Click to set point and direction (dimensions in properties)");
  }

  if (buttonsPerRow > 4) {
    ImGui::SameLine(0, buttonSpacing);
  } else {
    ImGui::Dummy(ImVec2(0, 3));
  }
  
  selected = UIState::activeMode == Drawing::DrawingMode::Square;
  if (ImGui::Button(ICON_SQUARE, ImVec2(buttonWidth, buttonHeight))) {
    SelectTool(Drawing::DrawingMode::Square, canvas, "Square Tool: Click to set corner and direction (side length in properties)");
  }

  if (buttonsPerRow > 5) {
    ImGui::SameLine(0, buttonSpacing);
  } else {
    ImGui::Dummy(ImVec2(0, 3));
  }
  
  selected = UIState::activeMode == Drawing::DrawingMode::Rectangle;
  if (ImGui::Button(ICON_RECTANGLE, ImVec2(buttonWidth, buttonHeight))) {
    SelectTool(Drawing::DrawingMode::Rectangle, canvas, "Rectangle Tool: Click to set corner and direction (dimensions in properties)");
  }

  // Start new row for additional tools
  ImGui::Dummy(ImVec2(0, 2));
  
  // Row 2: Additional drawing tools
  selected = UIState::activeMode == Drawing::DrawingMode::Spline;
  if (ImGui::Button(ICON_SPLINE, ImVec2(buttonWidth, buttonHeight))) {
    SelectTool(Drawing::DrawingMode::Spline, canvas, "Spline Tool: Click to add control points, double-click to finish");
  }
  
  ImGui::SameLine(0, buttonSpacing);
  selected = UIState::activeMode == Drawing::DrawingMode::BezierCurve;
  if (ImGui::Button(ICON_BEZIER, ImVec2(buttonWidth, buttonHeight))) {
    SelectTool(Drawing::DrawingMode::BezierCurve, canvas, "Bezier Tool: Click to add control points, double-click to finish");
  }

  if (buttonsPerRow > 2) {
    ImGui::SameLine(0, buttonSpacing);
  } else {
    ImGui::Dummy(ImVec2(0, 3));
  }
  
  selected = UIState::activeMode == Drawing::DrawingMode::Bellows;
  if (ImGui::Button("Bellows", ImVec2(buttonWidth, buttonHeight))) {
    SelectTool(Drawing::DrawingMode::Bellows, canvas, "Bellows Tool: Click to create a parametric bellows");
  }
  
  if (buttonsPerRow > 2) {
    ImGui::SameLine(0, buttonSpacing);
  } else {
    ImGui::Dummy(ImVec2(0, 3));
  }
  
  selected = UIState::activeMode == Drawing::DrawingMode::BallBearing;
  if (ImGui::Button("Ball Bearing", ImVec2(buttonWidth, buttonHeight))) {
    SelectTool(Drawing::DrawingMode::BallBearing, canvas, "Ball Bearing Tool: Click to create a parametric ball bearing");
  }
  
  ImGui::EndChild();
  ImGui::PopStyleColor();
  ImGui::EndGroup();
  
  // Edit Panel
  ImGui::SameLine(0, panelSpacing);
  ImGui::BeginGroup();
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.98f, 0.98f, 0.98f, 1.0f));
  ImGui::BeginChild("EditPanel", ImVec2(panelWidth, panelHeight), true);
  
  ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
  ImGui::PushStyleColor(ImGuiCol_Text, UIColors::TEXT_DIM);
  ImGui::Text("Edit");
  ImGui::PopStyleColor();
  ImGui::PopFont();
  
  ImGui::Dummy(ImVec2(0, 2));
  
  // Edit tools - use buttonWidth for consistency
  selected = UIState::activeMode == Drawing::DrawingMode::Select;
  if (ImGui::Button(ICON_SELECT, ImVec2(buttonWidth, buttonHeight))) {
    SelectTool(Drawing::DrawingMode::Select, canvas, "Select Tool: Click to select objects");
  }
  
  ImGui::SameLine(0, buttonSpacing);
  if (ImGui::Button(ICON_UNDO, ImVec2(buttonWidth, buttonHeight))) {
    canvas.undo();
    UIState::consoleMessage = "Undo";
  }
  
  if (buttonsPerRow > 2) {
    ImGui::SameLine(0, buttonSpacing);
  } else {
    ImGui::Dummy(ImVec2(0, 3));
  }
  
  if (ImGui::Button(ICON_REDO, ImVec2(buttonWidth, buttonHeight))) {
    canvas.redo();
    UIState::consoleMessage = "Redo";
  }
  
  if (buttonsPerRow > 2) {
    ImGui::SameLine(0, buttonSpacing);
  } else {
    ImGui::Dummy(ImVec2(0, 3));
  }
  
  // Add Clear All button without custom styling
  if (ImGui::Button("Clear All", ImVec2(buttonWidth, buttonHeight))) {
    canvas.clearAll();
    UIState::consoleMessage = "All shapes cleared";
  }
  
  ImGui::EndChild();
  ImGui::PopStyleColor();
  ImGui::EndGroup();
  
  // View Panel
  ImGui::SameLine(0, panelSpacing);
  ImGui::BeginGroup();
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.98f, 0.98f, 0.98f, 1.0f));
  ImGui::BeginChild("ViewPanel", ImVec2(panelWidth, panelHeight), true);
  
  ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
  ImGui::PushStyleColor(ImGuiCol_Text, UIColors::TEXT_DIM);
  ImGui::Text("View");
  ImGui::PopStyleColor();
  ImGui::PopFont();
  
  ImGui::Dummy(ImVec2(0, 2));
  
  // Grid and snap toggles with reduced width elements
  bool showGrid = canvas.isGridVisible();
  ImGui::PushItemWidth(panelWidth * 0.7f);
  if (ImGui::Checkbox("Grid", &showGrid)) {
    canvas.setShowGrid(showGrid);
    UIState::consoleMessage = showGrid ? "Grid: ON" : "Grid: OFF";
  }
  
  bool snapToGrid = canvas.isSnapToGridEnabled();
  if (ImGui::Checkbox("Snap", &snapToGrid)) {
    canvas.setSnapToGrid(snapToGrid);
    UIState::snapEnabled = snapToGrid;
    UIState::consoleMessage = snapToGrid ? "Snap to Grid: ON" : "Snap to Grid: OFF";
  }
  
  // Grid size slider
  float gridSize = UIState::gridSize;
  if (ImGui::SliderFloat("Size", &gridSize, 5.0f, 50.0f, "%.0f")) {
    UIState::gridSize = gridSize;
    canvas.setGridSpacing(gridSize);
    UIState::consoleMessage = "Grid Size Updated";
  }
  ImGui::PopItemWidth();
  
  ImGui::EndChild();
  ImGui::PopStyleColor();
  ImGui::EndGroup();
  
  // Right-aligned section for 3D view button
  ImGui::SameLine();
  ImGui::PushStyleColor(ImGuiCol_Button, UIColors::ACCENT);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.73f, 0.73f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
  
  // Increase 3D View button width
  if (ImGui::Button("3D View", ImVec2(100, 30))) {
    UIState::show3DView = true;
    UIState::bellows3DViewInitialized = false;
    UIState::ballBearing3DViewInitialized = false;
    UIState::consoleMessage = "Opening 3D View";
  }
  
  ImGui::PopStyleColor(3);
  
  ImGui::End();
  ImGui::PopStyleVar();
}

void RenderStatusBar(Drawing::Canvas &canvas) {
  // AutoCAD-style status bar at the bottom (without command line)
  const float statusBarHeight = 28.0f;
  
  // Get screen width for better scaling
  const float screenWidth = ImGui::GetIO().DisplaySize.x;
  
  // Status bar - full width
  ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetIO().DisplaySize.y - statusBarHeight));
  ImGui::SetNextWindowSize(ImVec2(screenWidth, statusBarHeight));
  ImGui::PushStyleColor(ImGuiCol_WindowBg, UIColors::DARK_PANEL);
  ImGui::Begin("StatusBar", nullptr,
               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

  // Status sections with separators - calculate based on available width
  ImVec2 mousePos = ImGui::GetMousePos();
  float windowWidth = ImGui::GetWindowWidth();
  
  // Calculate section widths proportionally based on window width
  const float sectionPadding = 10.0f;
  
  // Calculate widths that better accommodate text (min 150px for coord section)
  float coordSectionWidth = std::max(150.0f, windowWidth * 0.20f);
  float statusSectionWidth = std::max(150.0f, windowWidth * 0.45f);
  float zoomSectionWidth = std::max(100.0f, windowWidth * 0.15f);
  float gridSectionWidth = std::max(100.0f, windowWidth * 0.15f);
  
  // Units-aware coordinate display
  std::string unitSuffix;
  switch (UIState::units) {
    case Drawing::UnitSystem::Pixels:
      unitSuffix = "px";
      break;
    case Drawing::UnitSystem::Millimeters:
      unitSuffix = "mm";
      break;
    case Drawing::UnitSystem::Centimeters:
      unitSuffix = "cm";
      break;
    case Drawing::UnitSystem::Inches:
      unitSuffix = "in";
      break;
  }
  
  float scaledX = mousePos.x / UIState::unitScale;
  float scaledY = mousePos.y / UIState::unitScale;
  
  // Section 1: Coordinates
  ImGui::Text("X: %.1f%s  Y: %.1f%s", scaledX, unitSuffix.c_str(), scaledY, unitSuffix.c_str());
  
  ImGui::SameLine(coordSectionWidth);
  ImGui::Text("|");
  ImGui::SameLine(coordSectionWidth + sectionPadding);
  
  // Section 2: Status message - clipped if too long
  // Calculate how much space we have for the status message
  float statusMsgWidth = statusSectionWidth - sectionPadding*2;
  std::string statusMsg = UIState::consoleMessage;
  ImVec2 textSize = ImGui::CalcTextSize(statusMsg.c_str());
  
  // If the text is too long, add ellipsis
  if (textSize.x > statusMsgWidth) {
    // Find index to truncate
    float ellipsisWidth = ImGui::CalcTextSize("...").x;
    int maxChars = 0;
    float accWidth = 0;
    for (int i = 0; i < statusMsg.size(); i++) {
      char tmp[2] = {statusMsg[i], '\0'};
      float charWidth = ImGui::CalcTextSize(tmp).x;
      if (accWidth + charWidth + ellipsisWidth > statusMsgWidth)
        break;
      accWidth += charWidth;
      maxChars = i + 1;
    }
    statusMsg = statusMsg.substr(0, maxChars) + "...";
  }
  
  ImGui::Text("%s", statusMsg.c_str());
  
  ImGui::SameLine(coordSectionWidth + statusSectionWidth);
  ImGui::Text("|");
  ImGui::SameLine(coordSectionWidth + statusSectionWidth + sectionPadding);
  
  // Section 3: Zoom level
  ImGui::Text("Zoom: %.0f%%", UIState::zoomLevel * 100.0f);
  
  ImGui::SameLine(coordSectionWidth + statusSectionWidth + zoomSectionWidth);
  ImGui::Text("|");
  ImGui::SameLine(coordSectionWidth + statusSectionWidth + zoomSectionWidth + sectionPadding);
  
  // Section 4: Grid size indicator
  ImGui::Text("Grid: %d%s", (int)UIState::gridSize, unitSuffix.c_str());

  ImGui::End();
  ImGui::PopStyleColor();
}

void RenderPropertyPanel(Drawing::Canvas &canvas) {
  // Calculate property panel width - make it responsive
  const float screenWidth = ImGui::GetIO().DisplaySize.x;
  const float propertyPanelWidth = std::min(350.0f, screenWidth * 0.25f); // Increased from 300.0f and 0.2f for wider panel
  const float propertyPanelMinWidth = 300.0f; // Increased from 250.0f for better text accommodation
  const float actualPanelWidth = std::max(propertyPanelMinWidth, propertyPanelWidth);
  
  // AutoCAD-style properties panel on the right
  ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - actualPanelWidth, 95));
  ImGui::SetNextWindowSize(ImVec2(actualPanelWidth, ImGui::GetIO().DisplaySize.y - 150));

  ImGui::Begin("Properties", nullptr,
               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoCollapse);

  // Get the selected shape
  const Drawing::Shape* selectedShape = canvas.getSelectedShape();

  ImGui::PushStyleColor(ImGuiCol_Header, UIColors::HEADER);
  
  // Tool-specific properties - use the available width effectively
  const float availableWidth = ImGui::GetContentRegionAvail().x;
  const float inputWidth = availableWidth * 0.65f;
  
  if (UIState::activeMode == Drawing::DrawingMode::Line) {
    if (ImGui::CollapsingHeader("Line Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Indent(10);
      
      // Toggle for fixed/dynamic line length
      bool fixedLength = UIState::fixedLineLength;
      if (ImGui::Checkbox("Fixed Length", &fixedLength)) {
        UIState::fixedLineLength = fixedLength;
        canvas.setFixedLineLength(fixedLength);
        UIState::consoleMessage = fixedLength ? "Line length: Fixed" : "Line length: Dynamic (use mouse to draw)";
      }
      
      if (UIState::fixedLineLength) {
        // Get current line length from canvas
        static float lineLength = canvas.getLineLength();
        
        // Input field for line length
        ImGui::Text("Length:");
        ImGui::SameLine(70);
        ImGui::PushItemWidth(inputWidth);
        if (ImGui::InputFloat("##LineLength", &lineLength, 1.0f, 10.0f, "%.1f")) {
          // Ensure length is positive
          if (lineLength < 0.1f) lineLength = 0.1f;
          // Update canvas with new line length
          canvas.setLineLength(lineLength);
        }
        ImGui::PopItemWidth();
      } else {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Click and drag to draw");
      }
      
      ImGui::Unindent(10);
    }
  }
  else if (UIState::activeMode == Drawing::DrawingMode::Circle) {
    if (ImGui::CollapsingHeader("Circle Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Indent(10);
      
      // Toggle for fixed/dynamic circle radius
      bool fixedRadius = UIState::fixedCircleRadius;
      if (ImGui::Checkbox("Fixed Radius", &fixedRadius)) {
        UIState::fixedCircleRadius = fixedRadius;
        canvas.setFixedCircleRadius(fixedRadius);
        UIState::consoleMessage = fixedRadius ? "Circle radius: Fixed" : "Circle radius: Dynamic (use mouse to draw)";
      }
      
      if (UIState::fixedCircleRadius) {
        // Get current circle radius from canvas
        static float circleRadius = canvas.getCircleRadius();
        
        // Input field for circle radius
        ImGui::Text("Radius:");
        ImGui::SameLine(70);
        ImGui::PushItemWidth(inputWidth);
        if (ImGui::InputFloat("##CircleRadius", &circleRadius, 1.0f, 10.0f, "%.1f")) {
          // Ensure radius is positive
          if (circleRadius < 0.1f) circleRadius = 0.1f;
          // Update canvas with new radius
          canvas.setCircleRadius(circleRadius);
        }
        ImGui::PopItemWidth();
      } else {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Click and drag to draw circle");
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Release to complete");
      }
      
      ImGui::Unindent(10);
    }
  }
  else if (UIState::activeMode == Drawing::DrawingMode::Square) {
    if (ImGui::CollapsingHeader("Square Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Indent(10);
      
      // Toggle for fixed/dynamic square size
      bool fixedSize = UIState::fixedSquareSize;
      if (ImGui::Checkbox("Fixed Size", &fixedSize)) {
        UIState::fixedSquareSize = fixedSize;
        canvas.setFixedSquareSize(fixedSize);
        UIState::consoleMessage = fixedSize ? "Square size: Fixed" : "Square size: Dynamic (use mouse to draw)";
      }
      
      if (UIState::fixedSquareSize) {
        // Get current square size from canvas
        static float squareSize = canvas.getSquareSize();
        
        // Input field for square size
        ImGui::Text("Side Length:");
        ImGui::SameLine(70);
        ImGui::PushItemWidth(inputWidth);
        if (ImGui::InputFloat("##SquareSize", &squareSize, 1.0f, 10.0f, "%.1f")) {
          // Ensure size is positive
          if (squareSize < 0.1f) squareSize = 0.1f;
          // Update canvas with new size
          canvas.setSquareSize(squareSize);
        }
        ImGui::PopItemWidth();
      } else {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Click and drag to draw square");
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Release to complete");
      }
      
      ImGui::Unindent(10);
    }
  }
  else if (UIState::activeMode == Drawing::DrawingMode::Rectangle) {
    if (ImGui::CollapsingHeader("Rectangle Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Indent(10);
      
      // Toggle for fixed/dynamic rectangle size
      bool fixedSize = UIState::fixedRectangleSize;
      if (ImGui::Checkbox("Fixed Size", &fixedSize)) {
        UIState::fixedRectangleSize = fixedSize;
        canvas.setFixedRectangleSize(fixedSize);
        UIState::consoleMessage = fixedSize ? "Rectangle size: Fixed" : "Rectangle size: Dynamic (use mouse to draw)";
      }
      
      if (UIState::fixedRectangleSize) {
        // Get current rectangle dimensions from canvas
        static float rectangleWidth = canvas.getRectangleWidth();
        static float rectangleHeight = canvas.getRectangleHeight();
        
        // Input field for rectangle width
        ImGui::Text("Width:");
        ImGui::SameLine(70);
        ImGui::PushItemWidth(inputWidth);
        if (ImGui::InputFloat("##RectWidth", &rectangleWidth, 1.0f, 10.0f, "%.1f")) {
          // Ensure width is positive
          if (rectangleWidth < 0.1f) rectangleWidth = 0.1f;
          // Update canvas with new width
          canvas.setRectangleWidth(rectangleWidth);
        }
        ImGui::PopItemWidth();
        
        // Input field for rectangle height
        ImGui::Text("Height:");
        ImGui::SameLine(70);
        ImGui::PushItemWidth(inputWidth);
        if (ImGui::InputFloat("##RectHeight", &rectangleHeight, 1.0f, 10.0f, "%.1f")) {
          // Ensure height is positive
          if (rectangleHeight < 0.1f) rectangleHeight = 0.1f;
          // Update canvas with new height
          canvas.setRectangleHeight(rectangleHeight);
        }
        ImGui::PopItemWidth();
      } else {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Click and drag to draw rectangle");
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Release to complete");
      }
      
      ImGui::Unindent(10);
    }
  }
  else if (UIState::activeMode == Drawing::DrawingMode::Triangle) {
    if (ImGui::CollapsingHeader("Triangle Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Indent(10);
      
      // Toggle for fixed/dynamic triangle size
      bool fixedSize = UIState::fixedTriangleSize;
      if (ImGui::Checkbox("Fixed Size", &fixedSize)) {
        UIState::fixedTriangleSize = fixedSize;
        canvas.setFixedTriangleSize(fixedSize);
        UIState::consoleMessage = fixedSize ? "Triangle size: Fixed" : "Triangle size: Dynamic (use mouse to draw)";
      }
      
      if (UIState::fixedTriangleSize) {
        // Get current triangle side length from canvas
        static float triangleSide = canvas.getTriangleSide();
        
        // Input field for triangle side length
        ImGui::Text("Side Length:");
        ImGui::SameLine(70);
        ImGui::PushItemWidth(inputWidth);
        if (ImGui::InputFloat("##TriangleSide", &triangleSide, 1.0f, 10.0f, "%.1f")) {
          // Ensure side length is positive
          if (triangleSide < 0.1f) triangleSide = 0.1f;
          // Update canvas with new side length
          canvas.setTriangleSide(triangleSide);
        }
        ImGui::PopItemWidth();
      } else {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Click and drag to draw triangle");
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Release to complete");
      }
      
      ImGui::Unindent(10);
    }
  }
  else if (UIState::activeMode == Drawing::DrawingMode::Bellows) {
    if (ImGui::CollapsingHeader("Bellows Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Indent(10);
      
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Click to set start point");
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Click again to set length");
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Adjust parameters in properties");
      ImGui::Separator();
      
      ImGui::Text("Select an existing bellows to configure its parameters");
      
      ImGui::Unindent(10);
    }
  }
  else if (UIState::activeMode == Drawing::DrawingMode::BallBearing) {
    if (ImGui::CollapsingHeader("Ball Bearing Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Indent(10);
      
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Click to set center point");
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Click again to set size");
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Adjust parameters in properties");
      ImGui::Separator();
      
      ImGui::Text("Select an existing ball bearing to configure its parameters");
      
      ImGui::Unindent(10);
    }
  }
  
  // Selected object properties
  if (ImGui::CollapsingHeader("Object Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Indent(10);
    
    // Get selected shape
    Drawing::Shape *selectedShape = canvas.getSelectedShape();

    if (selectedShape) {
      // Add Bellows shape properties
      if (selectedShape->type == Drawing::ShapeType::BELLOWS) {
        Drawing::Bellows* bellows = static_cast<Drawing::Bellows*>(selectedShape);
        
        // Ensure the bellows is properly selected
        bellows->isSelected = true;
        
        ImGui::SeparatorText("Bellows Design Module");
        
        // Overall dimensions with validation
        float overallLength = bellows->calculateOverallLength();
        
        // Calculate available width for each control
        const float availWidth = ImGui::GetContentRegionAvail().x * 0.85f;
        ImGui::PushItemWidth(availWidth);
        
        if (ImGui::DragFloat("Overall Length (mm)", &overallLength, 1.0f, 20.0f, 1000.0f, "%.1f")) {
          bellows->updateFromOverallLength(overallLength);
        }
        ImGui::PopItemWidth();
        
        ImGui::Separator();
        
        // Cuff parameters section with validation feedback
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.2f, 0.3f, 0.7f));
        if (ImGui::CollapsingHeader("Cuff Dimensions", ImGuiTreeNodeFlags_DefaultOpen)) {
          ImGui::PushItemWidth(availWidth);
          
          bool validCuffDiameters = bellows->cuffAInnerDiameter <= bellows->baseConvolutionDiameter &&
                                   bellows->cuffBInnerDiameter <= bellows->baseConvolutionDiameter;
          
          if (!validCuffDiameters) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
            ImGui::TextWrapped("Warning: Cuff diameters should be less than Base Diameter");
            ImGui::PopStyleColor();
          }
          
          // Ensure min/max limits are enforced
          if (ImGui::DragFloat("Cuff A Inner Diam", &bellows->cuffAInnerDiameter, 0.5f, 5.0f, 200.0f, "%.1f")) {
            // Validate input - ensure it's positive
            if (bellows->cuffAInnerDiameter < 5.0f) bellows->cuffAInnerDiameter = 5.0f;
          }
          
          if (ImGui::DragFloat("Cuff B Inner Diam", &bellows->cuffBInnerDiameter, 0.5f, 5.0f, 200.0f, "%.1f")) {
            // Validate input - ensure it's positive
            if (bellows->cuffBInnerDiameter < 5.0f) bellows->cuffBInnerDiameter = 5.0f;
          }
          
          if (ImGui::DragFloat("Cuff A Length", &bellows->cuffALength, 0.5f, 5.0f, 100.0f, "%.1f")) {
            // Validate input - ensure it's positive
            if (bellows->cuffALength < 5.0f) bellows->cuffALength = 5.0f;
            
            // Update overall length
            overallLength = bellows->calculateOverallLength();
          }
          
          if (ImGui::DragFloat("Cuff B Length", &bellows->cuffBLength, 0.5f, 5.0f, 100.0f, "%.1f")) {
            // Validate input - ensure it's positive
            if (bellows->cuffBLength < 5.0f) bellows->cuffBLength = 5.0f;
            
            // Update overall length
            overallLength = bellows->calculateOverallLength();
          }
          
          ImGui::PopItemWidth();
        }
        ImGui::PopStyleColor();
        
        // Convolution parameters section
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.2f, 0.3f, 0.7f));
        if (ImGui::CollapsingHeader("Convolution Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
          ImGui::PushItemWidth(availWidth);
          
          bool validDiameters = bellows->baseConvolutionDiameter < bellows->peakConvolutionDiameter;
          if (!validDiameters) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
            ImGui::TextWrapped("Warning: Peak Diameter must be greater than Base Diameter");
            ImGui::PopStyleColor();
          }
          
          // Calculate minimum base diameter based on cuff diameters
          float minBaseDiameter = std::max(bellows->cuffAInnerDiameter, bellows->cuffBInnerDiameter);
          if (minBaseDiameter < 10.0f) minBaseDiameter = 10.0f;
          
          if (ImGui::DragFloat("Base Diameter", &bellows->baseConvolutionDiameter, 0.5f, 
                           minBaseDiameter, 300.0f, "%.1f")) {
            // Ensure peak diameter is always greater than base
            if (bellows->peakConvolutionDiameter <= bellows->baseConvolutionDiameter) {
              bellows->peakConvolutionDiameter = bellows->baseConvolutionDiameter + 5.0f;
            }
          }
          
          if (ImGui::DragFloat("Peak Diameter", &bellows->peakConvolutionDiameter, 0.5f, 
                           bellows->baseConvolutionDiameter + 0.1f, 350.0f, "%.1f")) {
            // Enforce the constraint directly in the UI
            if (bellows->peakConvolutionDiameter <= bellows->baseConvolutionDiameter) {
              bellows->peakConvolutionDiameter = bellows->baseConvolutionDiameter + 0.1f;
            }
          }
          
          // Prevent negative or zero convolution section length
          if (ImGui::DragFloat("Section Length", &bellows->convolutedSectionLength, 1.0f, 10.0f, 500.0f, "%.1f")) {
            if (bellows->convolutedSectionLength < 10.0f) {
              bellows->convolutedSectionLength = 10.0f;
            }
            
            // Update overall length
            overallLength = bellows->calculateOverallLength();
          }
          
          // Enforce reasonable limits for number of convolutions
          if (ImGui::DragInt("Convolutions", &bellows->numConvolutions, 1, 1, 30)) {
            // Keep numConvolutions at least 1
            bellows->numConvolutions = std::max(1, bellows->numConvolutions);
            
            // Limit based on convoluted section length - prevent too many convolutions
            float minSpacing = 5.0f; // Minimum spacing between convolutions
            int maxConvolutions = static_cast<int>(bellows->convolutedSectionLength / minSpacing);
            if (maxConvolutions < 1) maxConvolutions = 1;
            
            if (bellows->numConvolutions > maxConvolutions) {
              bellows->numConvolutions = maxConvolutions;
            }
          }
          
          // Calculate and display convolution spacing
          float convolutionSpacing = bellows->convolutedSectionLength / bellows->numConvolutions;
          ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Spacing: %.1f mm", convolutionSpacing);
          
          ImGui::PopItemWidth();
        }
        ImGui::PopStyleColor();
        
        // Wall thickness with validation
        ImGui::PushItemWidth(availWidth);
        
        // Calculate reasonable limits for wall thickness
        float maxWallThickness = std::min(bellows->baseConvolutionDiameter / 4.0f, 
                                       (bellows->peakConvolutionDiameter - bellows->baseConvolutionDiameter) / 2.0f);
        if (maxWallThickness < 0.5f) maxWallThickness = 0.5f;
        
        if (ImGui::DragFloat("Wall Thickness", &bellows->wallThickness, 0.1f, 0.5f, maxWallThickness, "%.1f")) {
          // Ensure wall thickness stays within reasonable limits
          if (bellows->wallThickness < 0.5f) bellows->wallThickness = 0.5f;
          if (bellows->wallThickness > maxWallThickness) bellows->wallThickness = maxWallThickness;
        }
        
        bool validWallThickness = bellows->wallThickness <= bellows->baseConvolutionDiameter / 4.0f &&
                                 bellows->wallThickness <= (bellows->peakConvolutionDiameter - bellows->baseConvolutionDiameter) / 2.0f;
        
        if (!validWallThickness) {
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
          ImGui::TextWrapped("Warning: Wall thickness too large");
          ImGui::PopStyleColor();
        }
        
        ImGui::PopItemWidth();
        
        // Display options
        ImGui::Checkbox("Show Dimensions", &bellows->showDimensions);
        
        ImGui::Separator();
        
        // Action buttons - arrange in two rows if panel is narrow
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.6f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.5f, 0.7f, 1.0f));
        
        const float btnWidth = availWidth / 2 - 4;
        
        // Reset parameters button
        if (ImGui::Button("Reset Parameters", ImVec2(btnWidth, 30))) {
          bellows->resetParameters();
        }
        
        // Fit to view button
        if (ImGui::Button("Fit to View", ImVec2(btnWidth, 30))) {
          canvas.fitBellowsToView();
        }
        
        ImGui::PopStyleColor(2);
      }
      // Add Ball Bearing shape properties
      else if (selectedShape->type == Drawing::ShapeType::BALL_BEARING) {
        Drawing::BallBearing* ballBearing = static_cast<Drawing::BallBearing*>(selectedShape);
        
        // Ensure the ball bearing is properly selected
        ballBearing->isSelected = true;
        
        ImGui::SeparatorText("Ball Bearing Design Module");
        
        // Calculate available width for each control
        const float availWidth = ImGui::GetContentRegionAvail().x * 0.85f;
        ImGui::PushItemWidth(availWidth);
        
        // Overall dimensions with validation
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.2f, 0.3f, 0.7f));
        if (ImGui::CollapsingHeader("Main Dimensions", ImGuiTreeNodeFlags_DefaultOpen)) {
          ImGui::PushItemWidth(availWidth);
          
          bool validDiameters = ballBearing->innerDiameter < ballBearing->outerDiameter &&
                               ballBearing->ballDiameter < (ballBearing->outerDiameter - ballBearing->innerDiameter) / 2.0f;
          
          if (!validDiameters) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
            ImGui::TextWrapped("Warning: Check diameter relationships");
            ImGui::PopStyleColor();
          }
          
          if (ImGui::DragFloat("Outer Diameter (mm)", &ballBearing->outerDiameter, 1.0f, 10.0f, 500.0f, "%.1f")) {
            if (ballBearing->outerDiameter <= ballBearing->innerDiameter) {
              ballBearing->outerDiameter = ballBearing->innerDiameter + 10.0f;
            }
          }
          
          if (ImGui::DragFloat("Inner Diameter (mm)", &ballBearing->innerDiameter, 1.0f, 5.0f, ballBearing->outerDiameter - 5.0f, "%.1f")) {
            if (ballBearing->innerDiameter >= ballBearing->outerDiameter) {
              ballBearing->innerDiameter = ballBearing->outerDiameter - 5.0f;
            }
          }
          
          ImGui::DragFloat("Width (mm)", &ballBearing->width, 0.5f, 1.0f, 100.0f, "%.1f");
          
          ImGui::PopItemWidth();
        }
        ImGui::PopStyleColor();
        
        // Ball parameters
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.3f, 0.2f, 0.7f));
        if (ImGui::CollapsingHeader("Ball Configuration", ImGuiTreeNodeFlags_DefaultOpen)) {
          ImGui::PushItemWidth(availWidth);
          
          float maxBallDiameter = (ballBearing->outerDiameter - ballBearing->innerDiameter) / 2.5f;
          if (ImGui::DragFloat("Ball Diameter (mm)", &ballBearing->ballDiameter, 0.5f, 1.0f, maxBallDiameter, "%.1f")) {
            if (ballBearing->ballDiameter > maxBallDiameter) {
              ballBearing->ballDiameter = maxBallDiameter;
            }
          }
          
          int numBalls = ballBearing->numBalls;
          if (ImGui::InputInt("Number of Balls", &numBalls, 1, 2)) {
            ballBearing->numBalls = std::max(3, std::min(50, numBalls));
          }
          
          ImGui::DragFloat("Race Radius (mm)", &ballBearing->raceRadius, 0.1f, 0.5f, 10.0f, "%.1f");
          ImGui::DragFloat("Contact Angle (°)", &ballBearing->contactAngle, 1.0f, 0.0f, 45.0f, "%.1f");
          
          ImGui::PopItemWidth();
        }
        ImGui::PopStyleColor();
        
        // Display options
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.1f, 0.3f, 0.7f));
        if (ImGui::CollapsingHeader("Display Options", ImGuiTreeNodeFlags_DefaultOpen)) {
          ImGui::Checkbox("Show Balls", &ballBearing->showBalls);
          ImGui::Checkbox("Show Cage", &ballBearing->showCage);
          ImGui::Checkbox("Show Dimensions", &ballBearing->showDimensions);
        }
        ImGui::PopStyleColor();
        
        ImGui::PopItemWidth();
        
        ImGui::Separator();
        
        // Action buttons
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.6f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.5f, 0.7f, 1.0f));
        
        const float btnWidth = availWidth / 2 - 4;
        
        // Reset parameters button
        if (ImGui::Button("Reset Parameters", ImVec2(btnWidth, 30))) {
          ballBearing->resetParameters();
        }
        
        ImGui::SameLine();
        
        // Fit to view button
        if (ImGui::Button("Fit to View", ImVec2(btnWidth, 30))) {
          // TODO: Implement fitBallBearingToView similar to fitBellowsToView
        }
        
        ImGui::PopStyleColor(2);
      }
    }
    
    ImGui::Unindent(10);
  }
  
  ImGui::PopStyleColor(); // Header color
  
  // Bellows section
  if (selectedShape && selectedShape->type == Drawing::ShapeType::BELLOWS) {
    ImGui::Separator();
    ImGui::Text("Bellows Properties:");
    ImGui::PushStyleColor(ImGuiCol_Header, UIColors::HEADER);
    
    // Cast to Bellows - we need to remove const and then cast
    const Drawing::Bellows* const_bellows = static_cast<const Drawing::Bellows*>(selectedShape);
    Drawing::Bellows* bellows = const_cast<Drawing::Bellows*>(const_bellows);
    
    if (bellows) {
      ImGui::Indent(15); // Changed from 10 to 15 to move the content a bit to the left
      
      // Position and angle
      float position[2] = {bellows->position.x, bellows->position.y};
      if (ImGui::DragFloat2("Position", position, 1.0f)) {
        bellows->position.x = position[0];
        bellows->position.y = position[1];
      }
      
      float angle = bellows->angle * 180.0f / M_PI; // Convert to degrees for display
      if (ImGui::SliderFloat("Rotation", &angle, 0.0f, 360.0f, "%.1f°")) {
        bellows->angle = angle * M_PI / 180.0f; // Convert back to radians
      }
      
      ImGui::Separator();
      
      // Dimension parameters
      if (ImGui::CollapsingHeader("Dimensions", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(10);
        
        // Overall length calculation
        float overallLength = bellows->calculateOverallLength();
        ImGui::Text("Overall Length: %.1f mm", overallLength);
        
        // Show dimensions toggle
        ImGui::Checkbox("Show Dimensions", &bellows->showDimensions);
        
        ImGui::Separator();
        
        // Cuffs
        ImGui::Text("Cuffs:");
        ImGui::DragFloat("Cuff A Inner Diameter", &bellows->cuffAInnerDiameter, 0.5f, 10.0f, 100.0f, "%.1f mm");
        ImGui::DragFloat("Cuff B Inner Diameter", &bellows->cuffBInnerDiameter, 0.5f, 10.0f, 100.0f, "%.1f mm");
        ImGui::DragFloat("Cuff A Length", &bellows->cuffALength, 0.5f, 5.0f, 50.0f, "%.1f mm");
        ImGui::DragFloat("Cuff B Length", &bellows->cuffBLength, 0.5f, 5.0f, 50.0f, "%.1f mm");
        
        ImGui::Separator();
        
        // Convolutions
        ImGui::Text("Convolutions:");
        ImGui::DragFloat("Base Diameter", &bellows->baseConvolutionDiameter, 0.5f, bellows->cuffAInnerDiameter, bellows->peakConvolutionDiameter, "%.1f mm");
        ImGui::DragFloat("Peak Diameter", &bellows->peakConvolutionDiameter, 0.5f, bellows->baseConvolutionDiameter, 150.0f, "%.1f mm");
        ImGui::DragFloat("Convoluted Section Length", &bellows->convolutedSectionLength, 1.0f, 10.0f, 300.0f, "%.1f mm");
        
        int numConvs = bellows->numConvolutions;
        if (ImGui::InputInt("Number of Convolutions", &numConvs, 1, 2)) {
          bellows->numConvolutions = std::max(1, numConvs);
        }
        
        ImGui::DragFloat("Wall Thickness", &bellows->wallThickness, 0.1f, 0.5f, 5.0f, "%.1f mm");
        
        ImGui::Unindent(10);
      }
      
      // ... rest of the bellows properties section ...
    }
    
    // Ensure we pop the style color for header
    ImGui::PopStyleColor();
  }
  
  ImGui::End();
}

void RenderCanvas(Drawing::Canvas &canvas) {
  // Adjust canvas position and size to account for the ribbon and status bar
  // Using simplified UI layout (no tabs, single row of tools)
  const float ribbonHeight = 95.0f; // Updated height for our more compact ribbon
  const float statusBarHeight = 28.0f; // Just status bar now, no command line
  
  // Calculate property panel width - make it responsive
  const float screenWidth = ImGui::GetIO().DisplaySize.x;
  const float propertyPanelWidth = std::min(350.0f, screenWidth * 0.25f); // Increased from 300.0f and 0.2f for wider panel
  const float propertyPanelMinWidth = 300.0f; // Increased from 250.0f for better text accommodation
  const float actualPanelWidth = std::max(propertyPanelMinWidth, propertyPanelWidth);
  
  float canvasX = 0.0f;
  float canvasY = ribbonHeight;
  float canvasWidth = ImGui::GetIO().DisplaySize.x - actualPanelWidth;
  float canvasHeight = ImGui::GetIO().DisplaySize.y - ribbonHeight - statusBarHeight;

  ImGui::SetNextWindowPos(ImVec2(canvasX, canvasY));
  ImGui::SetNextWindowSize(ImVec2(canvasWidth, canvasHeight));

  // Set canvas background color for the light theme
  ImGui::PushStyleColor(ImGuiCol_WindowBg, UIColors::GRID_BACKGROUND);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  
  ImGui::Begin("Canvas", nullptr,
               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
                   ImGuiWindowFlags_NoTitleBar);

  // Get the canvas window position for proper coordinate transformation
  ImVec2 canvasPos = ImGui::GetWindowPos();
  ImVec2 canvasSize = ImGui::GetWindowSize();

  // Update canvas with window information for proper coordinate transformation
  canvas.setWindowInfo(canvasPos.x, canvasPos.y, canvasSize.x, canvasSize.y);

  // Create clipping rectangle for the drawing area
  ImGui::PushClipRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), true);
  
  // Handle canvas input and rendering
  canvas.handleInput();

  ImDrawList *drawList = ImGui::GetWindowDrawList();
  canvas.render(drawList);
  
  // Add ruler indicators if enabled (AutoCAD has rulers)
  if (UIState::showRulers) {
    const float rulerSize = 20.0f;
    const float majorTickHeight = 10.0f;
    const float minorTickHeight = 5.0f;
    const float tickSpacing = UIState::gridSize * UIState::zoomLevel;
    
    // Background for rulers
    drawList->AddRectFilled(
      ImVec2(canvasPos.x, canvasPos.y),
      ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + rulerSize),
      IM_COL32(45, 45, 48, 255) // Dark gray background for rulers
    );
    
    drawList->AddRectFilled(
      ImVec2(canvasPos.x, canvasPos.y),
      ImVec2(canvasPos.x + rulerSize, canvasPos.y + canvasSize.y),
      IM_COL32(45, 45, 48, 255) // Dark gray background for rulers
    );
    
    // Horizontal ruler ticks
    float originX = canvasPos.x + UIState::panOffset.x * UIState::zoomLevel;
    for (float x = originX; x < canvasPos.x + canvasSize.x; x += tickSpacing) {
      int majorTick = (int)std::round((x - originX) / tickSpacing);
      if (majorTick % 5 == 0) {
        // Major tick
        drawList->AddLine(
          ImVec2(x, canvasPos.y),
          ImVec2(x, canvasPos.y + majorTickHeight),
          IM_COL32(200, 200, 200, 255), // Light color for ticks
          1.0f
        );
        
        // Label for major ticks
        char label[16];
        int value = majorTick * UIState::gridSize / UIState::unitScale;
        sprintf(label, "%d", value);
        drawList->AddText(
          ImVec2(x + 2, canvasPos.y + 2),
          IM_COL32(200, 200, 200, 255), // Light color for text
          label
        );
      } else {
        // Minor tick
        drawList->AddLine(
          ImVec2(x, canvasPos.y),
          ImVec2(x, canvasPos.y + minorTickHeight),
          IM_COL32(150, 150, 150, 255), // Lighter color for minor ticks
          1.0f
        );
      }
    }
    
    // Vertical ruler ticks
    float originY = canvasPos.y + UIState::panOffset.y * UIState::zoomLevel;
    for (float y = originY; y < canvasPos.y + canvasSize.y; y += tickSpacing) {
      int majorTick = (int)std::round((y - originY) / tickSpacing);
      if (majorTick % 5 == 0) {
        // Major tick
        drawList->AddLine(
          ImVec2(canvasPos.x, y),
          ImVec2(canvasPos.x + majorTickHeight, y),
          IM_COL32(200, 200, 200, 255), // Light color for ticks
          1.0f
        );
        
        // Label for major ticks
        char label[16];
        int value = majorTick * UIState::gridSize / UIState::unitScale;
        sprintf(label, "%d", value);
        drawList->AddText(
          ImVec2(canvasPos.x + 2, y + 2),
          IM_COL32(200, 200, 200, 255), // Light color for text
          label
        );
      } else {
        // Minor tick
        drawList->AddLine(
          ImVec2(canvasPos.x, y),
          ImVec2(canvasPos.x + minorTickHeight, y),
          IM_COL32(150, 150, 150, 255), // Lighter color for minor ticks
          1.0f
        );
      }
    }
    
    // Corner square
    drawList->AddRectFilled(
      ImVec2(canvasPos.x, canvasPos.y),
      ImVec2(canvasPos.x + rulerSize, canvasPos.y + rulerSize),
      IM_COL32(50, 50, 55, 255) // Slightly darker for corner
    );
    
    // Origin indicator
    float originScreenX = canvasPos.x + UIState::panOffset.x * UIState::zoomLevel;
    float originScreenY = canvasPos.y + UIState::panOffset.y * UIState::zoomLevel;
    if (originScreenX >= canvasPos.x && originScreenX <= canvasPos.x + canvasSize.x &&
        originScreenY >= canvasPos.y && originScreenY <= canvasPos.y + canvasSize.y) {
      // Draw origin indicator
      drawList->AddCircleFilled(
        ImVec2(originScreenX, originScreenY),
        3.0f,
        IM_COL32(0, 170, 255, 255) // Blue dot for origin
      );
    }
  }
  
  // Mouse coordinates display at cursor if enabled
  if (UIState::showCoordinates) {
    ImVec2 mousePos = ImGui::GetMousePos();
    if (ImGui::IsWindowHovered() && 
        mousePos.x > canvasPos.x && mousePos.x < canvasPos.x + canvasSize.x &&
        mousePos.y > canvasPos.y && mousePos.y < canvasPos.y + canvasSize.y) {
      
      // Calculate world coordinates
      float worldX = (mousePos.x - canvasPos.x - UIState::panOffset.x * UIState::zoomLevel) / UIState::zoomLevel;
      float worldY = (mousePos.y - canvasPos.y - UIState::panOffset.y * UIState::zoomLevel) / UIState::zoomLevel;
      
      // Apply unit scaling
      worldX /= UIState::unitScale;
      worldY /= UIState::unitScale;
      
      // Format the coordinates
      char coordinates[64];
      std::string unitSuffix;
      switch (UIState::units) {
        case Drawing::UnitSystem::Pixels:
          unitSuffix = "px";
          break;
        case Drawing::UnitSystem::Millimeters:
          unitSuffix = "mm";
          break;
        case Drawing::UnitSystem::Centimeters:
          unitSuffix = "cm";
          break;
        case Drawing::UnitSystem::Inches:
          unitSuffix = "in";
          break;
      }
      
      sprintf(coordinates, "X: %.2f%s  Y: %.2f%s", worldX, unitSuffix.c_str(), worldY, unitSuffix.c_str());
      
      // Draw coordinates near cursor
      ImVec2 textSize = ImGui::CalcTextSize(coordinates);
      ImVec2 textPos = ImVec2(mousePos.x + 15, mousePos.y + 10);
      
      // Ensure text stays within canvas
      if (textPos.x + textSize.x > canvasPos.x + canvasSize.x)
        textPos.x = mousePos.x - textSize.x - 10;
      if (textPos.y + textSize.y > canvasPos.y + canvasSize.y)
        textPos.y = mousePos.y - textSize.y - 10;
      
      // Background for better readability
      drawList->AddRectFilled(
        ImVec2(textPos.x - 4, textPos.y - 2),
        ImVec2(textPos.x + textSize.x + 4, textPos.y + textSize.y + 2),
        IM_COL32(40, 40, 45, 220) // Dark background
      );
      
      // Text
      drawList->AddText(
        textPos,
        IM_COL32(220, 220, 220, 255), // Light text color
        coordinates
      );
    }
  }
  
  ImGui::PopClipRect();
  ImGui::End();
  ImGui::PopStyleVar();
  ImGui::PopStyleColor();
}


// Handle keyboard shortcuts
void HandleKeyboardShortcuts(Drawing::Canvas &canvas) {
  ImGuiIO& io = ImGui::GetIO();
  
  // AutoCAD-like keyboard shortcuts
  
  // ESC to cancel current operation or clear selection
  if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
    SelectTool(Drawing::DrawingMode::Select, canvas, "Ready");
    canvas.clearSelection();
  }
  
  // Drawing tools
  if (!io.KeyCtrl && !io.KeyShift && !io.KeyAlt) {
    // Simple key presses for common tools
    if (ImGui::IsKeyPressed(ImGuiKey_L)) {
      SelectTool(Drawing::DrawingMode::Line, canvas, "Line Tool: Click to set start point, second click sets direction");
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_C)) {
      SelectTool(Drawing::DrawingMode::Circle, canvas, "Circle Tool: Click to set center and direction (radius in properties)");
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_R)) {
      SelectTool(Drawing::DrawingMode::Rectangle, canvas, "Rectangle Tool: Click to set corner and direction (dimensions in properties)");
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_P)) {
      SelectTool(Drawing::DrawingMode::Point, canvas, "Point Tool: Click to place points");
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_T)) {
      SelectTool(Drawing::DrawingMode::Triangle, canvas, "Triangle Tool: Click to set point and direction (dimensions in properties)");
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_S)) {
      SelectTool(Drawing::DrawingMode::Square, canvas, "Square Tool: Click to set corner and direction (side length in properties)");
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_G)) {
      bool currentGridState = canvas.isGridVisible();
      canvas.setShowGrid(!currentGridState);
      UIState::consoleMessage = !currentGridState ? "Grid turned ON" : "Grid turned OFF";
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
      canvas.deleteSelectedShape();
      UIState::consoleMessage = "Deleted selected shape";
    }
  }
  
  // Modifier key combinations
  if (io.KeyCtrl) {
    if (ImGui::IsKeyPressed(ImGuiKey_Z)) {
      canvas.undo();
      UIState::consoleMessage = "Undo last action";
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_A)) {
      // Select all - would need implementation
      UIState::consoleMessage = "Select All (not implemented)";
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_S)) {
      // Save - would need implementation
      UIState::consoleMessage = "Save Drawing (not implemented)";
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_O)) {
      // Open - would need implementation
      UIState::consoleMessage = "Open Drawing (not implemented)";
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_N)) {
      // New drawing - would need implementation
      canvas.clearAll();
      UIState::consoleMessage = "New Drawing - All shapes cleared";
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_Equal)) {
      // Zoom in
      UIState::zoomLevel = std::min(UIState::zoomLevel * 1.2f, 5.0f);
      UIState::consoleMessage = "Zoom In";
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_Minus)) {
      // Zoom out
      UIState::zoomLevel = std::max(UIState::zoomLevel / 1.2f, 0.2f);
      UIState::consoleMessage = "Zoom Out";
    }
  }
  
  if (io.KeyShift) {
    if (ImGui::IsKeyPressed(ImGuiKey_L)) {
      // AutoCAD-like: Shift+L for polyline
      UIState::consoleMessage = "Polyline command (not implemented)";
    }
  }
}

int main(int, char **) {
  // Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
    std::cerr << "Error initializing SDL: " << SDL_GetError() << std::endl;
    return -1;
  }

  // Setup SDL window
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_WindowFlags window_flags =
      (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                        SDL_WINDOW_ALLOW_HIGHDPI);
  SDL_Window *window = SDL_CreateWindow(
      "NAVIX", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      WINDOW_WIDTH, WINDOW_HEIGHT, window_flags);
  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
  SDL_GL_MakeCurrent(window, gl_context);
  SDL_GL_SetSwapInterval(1); // Enable vsync

  // Initialize GLAD
  if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
    std::cerr << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Reverted: Docking not available/enabled

  ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL3_Init("#version 130");

  SetupImGuiStyle();

  Drawing::Canvas canvas;
  bool running = true;

  // Initialize canvas settings
  canvas.setSnapToGrid(UIState::snapEnabled);
  canvas.setGridSpacing(UIState::gridSize);

  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      if (event.type == SDL_QUIT)
        running = false;
      if (event.type == SDL_WINDOWEVENT &&
          event.window.event == SDL_WINDOWEVENT_CLOSE &&
          event.window.windowID == SDL_GetWindowID(window))
        running = false;
      
      // Handle 3D viewer input
      if (UIState::show3DView) {
        Handle3DViewerInput(event);
      }
      
      // Handle mouse wheel for zoom (AutoCAD-like)
      if (event.type == SDL_MOUSEWHEEL) {
        // Only process wheel events for the canvas if mouse is over the canvas
        ImVec2 mousePos = ImGui::GetMousePos();
        bool isOverCanvas = false;
        
        // Calculate dimensions matching those in RenderCanvas
        const float ribbonHeight = 95.0f;
        const float statusBarHeight = 28.0f; // Just status bar height
        const float screenWidth = ImGui::GetIO().DisplaySize.x;
        const float propertyPanelWidth = std::min(350.0f, screenWidth * 0.25f);
        const float propertyPanelMinWidth = 300.0f;
        const float actualPanelWidth = std::max(propertyPanelMinWidth, propertyPanelWidth);
        
        float canvasX = 0.0f;
        float canvasY = ribbonHeight;
        float canvasWidth = screenWidth - actualPanelWidth;
        float canvasHeight = ImGui::GetIO().DisplaySize.y - ribbonHeight - statusBarHeight;
        
        // Check if mouse is over the canvas area
        if (mousePos.x >= canvasX && mousePos.x <= canvasX + canvasWidth &&
            mousePos.y >= canvasY && mousePos.y <= canvasY + canvasHeight) {
          isOverCanvas = true;
        }
        
        // Only apply zoom/pan if over canvas or if in 3D view mode
        if (isOverCanvas || UIState::show3DView) {
          if (io.KeyCtrl) {
            // Zoom in/out with Ctrl+Wheel
            float zoom_delta = event.wheel.y > 0 ? 1.1f : 0.9f;
            UIState::zoomLevel = std::clamp(UIState::zoomLevel * zoom_delta, 0.2f, 5.0f);
            UIState::consoleMessage = event.wheel.y > 0 ? "Zoom In" : "Zoom Out";
          } else if (io.KeyShift) {
            // Pan horizontally with Shift+Wheel
            UIState::panOffset.x += event.wheel.y * 10.0f;
            UIState::consoleMessage = "Pan Horizontally";
          } else {
            // Pan vertically with just Wheel
            UIState::panOffset.y += event.wheel.y * 10.0f;
            UIState::consoleMessage = "Pan Vertically";
          }
        }
      }
      
      // Handle middle mouse button panning (AutoCAD-like)
      if (event.type == SDL_MOUSEMOTION && (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_MIDDLE))) {
        // Calculate dimensions matching those in RenderCanvas
        const float ribbonHeight = 95.0f;
        const float statusBarHeight = 28.0f; // Just status bar height
        const float screenWidth = ImGui::GetIO().DisplaySize.x;
        const float propertyPanelWidth = std::min(350.0f, screenWidth * 0.25f);
        const float propertyPanelMinWidth = 300.0f;
        const float actualPanelWidth = std::max(propertyPanelMinWidth, propertyPanelWidth);
        
        float canvasX = 0.0f;
        float canvasY = ribbonHeight;
        float canvasWidth = screenWidth - actualPanelWidth;
        float canvasHeight = ImGui::GetIO().DisplaySize.y - ribbonHeight - statusBarHeight;
        
        // Get mouse position
        ImVec2 mousePos = ImGui::GetMousePos();
        
        // Only apply panning if the mouse is over the canvas
        if (mousePos.x >= canvasX && mousePos.x <= canvasX + canvasWidth &&
            mousePos.y >= canvasY && mousePos.y <= canvasY + canvasHeight) {
          UIState::panOffset.x += event.motion.xrel / UIState::zoomLevel;
          UIState::panOffset.y += event.motion.yrel / UIState::zoomLevel;
          UIState::consoleMessage = "Pan View";
        }
      }
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Handle keyboard shortcuts
    HandleKeyboardShortcuts(canvas);

    // Render the AutoCAD-like UI elements
    RenderTopRibbon(canvas);
    RenderCanvas(canvas);
    RenderPropertyPanel(canvas);
    RenderStatusBar(canvas);
    
    // Render 3D view window if active (will now create separate windows)
    Render3DViewWindow(canvas); // Use original function name

    ImGui::Render();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(UIColors::BACKGROUND.x, UIColors::BACKGROUND.y,
                 UIColors::BACKGROUND.z, UIColors::BACKGROUND.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_GL_DeleteContext(gl_context);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}

// Helper function to get the static 3D viewer instance
BellowsViewer3D& GetBellowsViewer() {
  static BellowsViewer3D viewer;
  static bool initialized = false;
  
  if (!initialized) {
    viewer.initialize();
    initialized = true;
  }
  
  return viewer;
}

// Helper function to get the static ball bearing 3D viewer instance
BallBearingViewer3D& GetBallBearingViewer() {
  static BallBearingViewer3D viewer;
  static bool initialized = false;
  
  if (!initialized) {
    viewer.initialize();
    initialized = true;
  }
  
  return viewer;
}

// Modified function to render separate standard windows
void Render3DViewWindow(Drawing::Canvas &canvas) {
  // Exit immediately if 3D view is not enabled
  if (!UIState::show3DView) {
    return;
  }
  
  // Check what type of shape we're viewing
  const Drawing::Bellows* bellows = canvas.findOrCreateBellows();
  const Drawing::BallBearing* ballBearing = canvas.findOrCreateBallBearing();
  
  // Determine which viewer to use
  bool showBellows = (bellows != nullptr);
  bool showBallBearing = (ballBearing != nullptr);
  
  // If no supported shape exists, don't show the 3D view
  if (!showBellows && !showBallBearing) {
    UIState::show3DView = false;
    return;
  }
  
  // Prefer ball bearing if both exist (most recently selected)
  if (showBallBearing) {
    // Get a reference to the static ball bearing viewer instance
    BallBearingViewer3D& viewer = GetBallBearingViewer();
    
    // Initialize viewer once only
    if (!UIState::ballBearing3DViewInitialized) {
      viewer.initialize();
      UIState::ballBearing3DViewInitialized = true;
    }
    
    // Get the current window size
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    
    // Set the 3D viewport size
    ImGui::SetNextWindowSize(ImVec2(screenSize.x * 0.75f, screenSize.y * 0.75f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(screenSize.x * 0.5f, screenSize.y * 0.5f), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    
    // Add no padding to ensure viewport uses full window size
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    
    if (ImGui::Begin("3D Ball Bearing Viewport", &UIState::show3DView, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoMove)) {
      ImVec2 viewportSize = ImGui::GetContentRegionAvail();
      
      // Render the 3D model into the viewport window
      viewer.render(ballBearing, viewportSize); 
      
      // Handle Mouse Rotation Input
      ImGuiIO& io = ImGui::GetIO();
      if (ImGui::IsWindowHovered()) {
          ImGui::SetWindowFocus(); 

          if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
              ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
              ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left); 
              
              if (std::abs(delta.x) > 0.1f || std::abs(delta.y) > 0.1f) {
                   viewer.getCamera()->ProcessMouseMovement(delta.x, delta.y, true); 
              }
          }
      }
    }
    ImGui::End();
    ImGui::PopStyleVar(); // WindowPadding
  }
  else if (showBellows) {
    // Get a reference to the static bellows viewer instance
    BellowsViewer3D& viewer = GetBellowsViewer();
    
    // Initialize viewer once only
    if (!UIState::bellows3DViewInitialized) {
      viewer.initialize();
      UIState::bellows3DViewInitialized = true;
    }
    
    // Get the current window size
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    
    // Set the 3D viewport size
    ImGui::SetNextWindowSize(ImVec2(screenSize.x * 0.75f, screenSize.y * 0.75f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(screenSize.x * 0.5f, screenSize.y * 0.5f), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    
    // Add no padding to ensure viewport uses full window size
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    
    if (ImGui::Begin("3D Bellows Viewport", &UIState::show3DView, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoMove)) {
      ImVec2 viewportSize = ImGui::GetContentRegionAvail();
      
      // Render the 3D model into the viewport window
      viewer.render(bellows, viewportSize); 
      
      // Handle Mouse Rotation Input
      ImGuiIO& io = ImGui::GetIO();
      if (ImGui::IsWindowHovered()) {
          ImGui::SetWindowFocus(); 

          if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
              ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
              ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left); 
              
              if (std::abs(delta.x) > 0.1f || std::abs(delta.y) > 0.1f) {
                   viewer.getCamera()->ProcessMouseMovement(delta.x, delta.y, true); 
              }
          }
      }
    }
    ImGui::End();
    ImGui::PopStyleVar(); // WindowPadding
  }

  // If the main show3DView flag becomes false, ensure we reset initialization
  if (!UIState::show3DView) {
      UIState::bellows3DViewInitialized = false; // Reset initialization flag when view is closed
      UIState::ballBearing3DViewInitialized = false; // Reset ball bearing initialization flag when view is closed
  }
}

// Helper function to handle 3D viewer input
void Handle3DViewerInput(const SDL_Event& event) {
  if (!UIState::show3DView) return;
  
  // Check if ImGui wants to capture keyboard input (e.g., when typing in text fields)
  ImGuiIO& io = ImGui::GetIO();
  bool allowKeyboardInput = !io.WantCaptureKeyboard;
  
  // Determine which viewer is active
  // For now, we'll handle input for both viewers since we don't track which is currently shown
  BellowsViewer3D& bellowsViewer = GetBellowsViewer();
  BallBearingViewer3D& ballBearingViewer = GetBallBearingViewer();
  
  // Pass keyboard events only if ImGui doesn't want them, always pass mouse events
  if ((event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) && allowKeyboardInput) {
      bellowsViewer.handleInput(event);
      ballBearingViewer.handleInput(event);
  }
  // Always pass mouse wheel and mouse button events
  else if (event.type == SDL_MOUSEWHEEL || event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
      bellowsViewer.handleInput(event);
      ballBearingViewer.handleInput(event);
  }
}
