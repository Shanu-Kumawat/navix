#include "Canvas.hpp"
#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>
#include <iostream>
#include "BellowsViewer3D.hpp"

// Replace icon definitions with text labels
#define ICON_LINE "Line"
#define ICON_CIRCLE "Circle"
#define ICON_RECTANGLE "Rect"
#define ICON_POINT "Point"
#define ICON_TRIANGLE "Triangle"
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

// UI Colors - Enhanced AutoCAD-inspired color scheme
namespace UIColors {
// AutoCAD-like modern theme (2023 version)
const ImVec4 BACKGROUND = ImVec4(0.15f, 0.16f, 0.17f, 1.0f);                // Original dark gray background
const ImVec4 PANEL = ImVec4(0.94f, 0.94f, 0.94f, 1.0f);                  // Very light gray panels
const ImVec4 DARK_PANEL = ImVec4(0.87f, 0.87f, 0.87f, 1.0f);             // Subtle darker gray for headers
const ImVec4 HEADER = ImVec4(0.0f, 0.47f, 0.84f, 1.0f);                  // AutoCAD 2023 blue
const ImVec4 BORDER = ImVec4(0.75f, 0.75f, 0.75f, 1.0f);                 // Subtle gray borders
const ImVec4 TEXT = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);                      // Pure black text for clarity
const ImVec4 TEXT_DIM = ImVec4(0.45f, 0.45f, 0.45f, 1.0f);               // Medium gray dimmed text
const ImVec4 BUTTON = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);                    // Very light gray buttons
const ImVec4 BUTTON_HOVERED = ImVec4(0.78f, 0.78f, 0.78f, 1.0f);         // Darker when hovered
const ImVec4 BUTTON_ACTIVE = ImVec4(0.0f, 0.47f, 0.84f, 1.0f);           // Blue when active
const ImVec4 BUTTON_TEXT = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);               // Dark button text
const ImVec4 ACCENT = ImVec4(0.0f, 0.47f, 0.84f, 1.0f);                  // Accent color - AutoCAD blue
const ImVec4 TAB_ACTIVE = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);                // White active tab background
const ImVec4 GRID_BACKGROUND = ImVec4(0.15f, 0.15f, 0.18f, 1.0f);        // Dark background for grid area
const ImVec4 COMMAND_BG = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);             // Dark command line background
const ImVec4 COMMAND_TEXT = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);              // White command line text
const ImVec4 SUCCESS = ImVec4(0.0f, 0.7f, 0.2f, 1.0f);                   // Green for success/confirmation
const ImVec4 WARNING = ImVec4(0.9f, 0.6f, 0.0f, 1.0f);                   // Orange for warnings
const ImVec4 ERROR = ImVec4(0.8f, 0.0f, 0.0f, 1.0f);                     // Red for errors
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
void ProcessCommand(const std::string &command, Drawing::Canvas &canvas);
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

  // Colors - AutoCAD-like
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

  // Styles - AutoCAD-like
  style.WindowPadding = ImVec2(8, 8);
  style.FramePadding = ImVec2(6, 4);
  style.CellPadding = ImVec2(4, 2);
  style.ItemSpacing = ImVec2(8, 6);
  style.ItemInnerSpacing = ImVec2(4, 4);
  style.TouchExtraPadding = ImVec2(0, 0);
  style.IndentSpacing = 21;
  style.ScrollbarSize = 12;
  style.GrabMinSize = 12;

  style.WindowBorderSize = 1;
  style.ChildBorderSize = 1;
  style.PopupBorderSize = 1;
  style.FrameBorderSize = 1;
  style.TabBorderSize = 0;

  style.WindowRounding = 0;
  style.ChildRounding = 0;
  style.FrameRounding = 2;
  style.PopupRounding = 0;
  style.ScrollbarRounding = 2;
  style.GrabRounding = 2;
  style.TabRounding = 2;
}

void RenderTopRibbon(Drawing::Canvas &canvas) {
  // AutoCAD-style ribbon at the top
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, 110));

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2, 2));
  ImGui::Begin("Ribbon", nullptr,
               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

  // Brand/filename section on the left
  ImGui::BeginGroup();
  ImGui::PushStyleColor(ImGuiCol_Text, UIColors::ACCENT);
  ImGui::Text("CAD");
  ImGui::PopStyleColor();
  ImGui::SameLine();
  ImGui::Text("Drawing");
  ImGui::EndGroup();
  
  ImGui::SameLine();
  ImGui::Dummy(ImVec2(20, 0)); // Reduced spacing from 40 to 20
  
  // Consolidated ribbon with all tools in a single row
  // No tabs, just direct panel display for a cleaner interface
  const float panelHeight = 85.0f; // Reduced height for a more compact UI
  const float buttonSize = 56.0f;  // Slightly smaller buttons
  const float smallBtnSize = 38.0f;
  const float buttonSpacing = 6.0f; // Tighter spacing
  const float panelSpacing = 8.0f;  // Reduced panel spacing
  ImVec4 groupColor = ImVec4(0.97f, 0.97f, 0.97f, 1.0f); // Lighter background for better contrast
  
  // Draw Panel
  ImGui::BeginGroup();
  ImGui::PushStyleColor(ImGuiCol_ChildBg, groupColor);
  ImGui::BeginChild("DrawPanel", ImVec2(280, panelHeight), true);
  
  ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
  ImGui::PushStyleColor(ImGuiCol_Text, UIColors::HEADER);
  ImGui::Text("Draw");
  ImGui::PopStyleColor();
  ImGui::PopFont();
  ImGui::Separator();
  
  // Drawing tools in a single row
  ImGui::PushStyleColor(ImGuiCol_Button, UIState::activeMode == Drawing::DrawingMode::Line ? UIColors::BUTTON_ACTIVE : UIColors::BUTTON);
  if (ImGui::Button(ICON_LINE, ImVec2(buttonSize, buttonSize)))
    SelectTool(Drawing::DrawingMode::Line, canvas, "Line Tool: Click to set start point, second click sets direction");
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Line (L)");
  ImGui::PopStyleColor();
  
  ImGui::SameLine(0, buttonSpacing);
  
  ImGui::PushStyleColor(ImGuiCol_Button, UIState::activeMode == Drawing::DrawingMode::Circle ? UIColors::BUTTON_ACTIVE : UIColors::BUTTON);
  if (ImGui::Button(ICON_CIRCLE, ImVec2(buttonSize, buttonSize)))
    SelectTool(Drawing::DrawingMode::Circle, canvas, "Circle Tool: Click to set center and direction (radius in properties)");
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Circle (C)");
  ImGui::PopStyleColor();
  
  ImGui::SameLine(0, buttonSpacing);
  
  ImGui::PushStyleColor(ImGuiCol_Button, UIState::activeMode == Drawing::DrawingMode::Rectangle ? UIColors::BUTTON_ACTIVE : UIColors::BUTTON);
  if (ImGui::Button(ICON_RECTANGLE, ImVec2(buttonSize, buttonSize)))
    SelectTool(Drawing::DrawingMode::Rectangle, canvas, "Rectangle Tool: Click to set corner and direction (dimensions in properties)");
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Rectangle (R)");
  ImGui::PopStyleColor();
  
  ImGui::SameLine(0, buttonSpacing);
  
  ImGui::PushStyleColor(ImGuiCol_Button, UIState::activeMode == Drawing::DrawingMode::Point ? UIColors::BUTTON_ACTIVE : UIColors::BUTTON);
  if (ImGui::Button(ICON_POINT, ImVec2(buttonSize, buttonSize)))
    SelectTool(Drawing::DrawingMode::Point, canvas, "Point Tool: Click to place points");
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Point (P)");
  ImGui::PopStyleColor();
  
  ImGui::EndChild();
  ImGui::PopStyleColor(); // Child bg
  ImGui::EndGroup();
  
  ImGui::SameLine(0, panelSpacing);
  
  // More Shapes Panel
  ImGui::BeginGroup();
  ImGui::PushStyleColor(ImGuiCol_ChildBg, groupColor);
  ImGui::BeginChild("MoreShapesPanel", ImVec2(155, panelHeight), true);
  
  ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
  ImGui::PushStyleColor(ImGuiCol_Text, UIColors::HEADER);
  ImGui::Text("More Shapes");
  ImGui::PopStyleColor();
  ImGui::PopFont();
  ImGui::Separator();
  
  ImGui::PushStyleColor(ImGuiCol_Button, UIState::activeMode == Drawing::DrawingMode::Triangle ? UIColors::BUTTON_ACTIVE : UIColors::BUTTON);
  if (ImGui::Button(ICON_TRIANGLE, ImVec2(buttonSize, buttonSize)))
    SelectTool(Drawing::DrawingMode::Triangle, canvas, "Triangle Tool: Click to set point and direction (dimensions in properties)");
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Triangle (T)");
  ImGui::PopStyleColor();
  
  ImGui::SameLine(0, buttonSpacing);
  
  ImGui::PushStyleColor(ImGuiCol_Button, UIState::activeMode == Drawing::DrawingMode::Square ? UIColors::BUTTON_ACTIVE : UIColors::BUTTON);
  if (ImGui::Button(ICON_SQUARE, ImVec2(buttonSize, buttonSize)))
    SelectTool(Drawing::DrawingMode::Square, canvas, "Square Tool: Click to set corner and direction (side length in properties)");
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Square (S)");
  ImGui::PopStyleColor();
  
  ImGui::EndChild();
  ImGui::PopStyleColor(); // Child bg
  ImGui::EndGroup();
  
  ImGui::SameLine(0, panelSpacing);
  
  // Curves Panel
  ImGui::BeginGroup();
  ImGui::PushStyleColor(ImGuiCol_ChildBg, groupColor);
  ImGui::BeginChild("CurvesPanel", ImVec2(225, panelHeight), true);  // Increased width to fit 3 buttons
  
  ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
  ImGui::PushStyleColor(ImGuiCol_Text, UIColors::HEADER);
  ImGui::Text("Curves");
  ImGui::PopStyleColor();
  ImGui::PopFont();
  ImGui::Separator();
  
  ImGui::PushStyleColor(ImGuiCol_Button, UIState::activeMode == Drawing::DrawingMode::Spline ? UIColors::BUTTON_ACTIVE : UIColors::BUTTON);
  if (ImGui::Button(ICON_SPLINE, ImVec2(buttonSize, buttonSize)))
    SelectTool(Drawing::DrawingMode::Spline, canvas, "Spline Tool: Click to add control points, right-click to finish");
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Spline");
  ImGui::PopStyleColor();
  
  ImGui::SameLine(0, buttonSpacing);
  
  ImGui::PushStyleColor(ImGuiCol_Button, UIState::activeMode == Drawing::DrawingMode::BezierCurve ? UIColors::BUTTON_ACTIVE : UIColors::BUTTON);
  if (ImGui::Button(ICON_BEZIER, ImVec2(buttonSize, buttonSize)))
    SelectTool(Drawing::DrawingMode::BezierCurve, canvas, "Bezier Curve Tool: Click to place 4 control points");
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Bezier");
  ImGui::PopStyleColor();
  
  ImGui::SameLine(0, buttonSpacing);
  
  ImGui::PushStyleColor(ImGuiCol_Button, UIState::activeMode == Drawing::DrawingMode::Bellows ? UIColors::BUTTON_ACTIVE : UIColors::BUTTON);
  if (ImGui::Button("Bellows", ImVec2(buttonSize, buttonSize)))
    SelectTool(Drawing::DrawingMode::Bellows, canvas, "Bellows Tool: Click to set start, second click sets length");
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Parametric Bellows");
  ImGui::PopStyleColor();
  
  ImGui::EndChild();
  ImGui::PopStyleColor(); // Child bg
  ImGui::EndGroup();
  
  ImGui::SameLine(0, panelSpacing);
  
  // Modify Panel
  ImGui::BeginGroup();
  ImGui::PushStyleColor(ImGuiCol_ChildBg, groupColor);
  ImGui::BeginChild("ModifyPanel", ImVec2(220, panelHeight), true);
  
  ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
  ImGui::PushStyleColor(ImGuiCol_Text, UIColors::HEADER);
  ImGui::Text("Modify");
  ImGui::PopStyleColor();
  ImGui::PopFont();
  ImGui::Separator();
  
  // Selection tools with improved layout
  ImGui::PushStyleColor(ImGuiCol_Button, UIState::activeMode == Drawing::DrawingMode::Select ? UIColors::BUTTON_ACTIVE : UIColors::BUTTON);
  if (ImGui::Button(ICON_SELECT, ImVec2(buttonSize, buttonSize)))
    SelectTool(Drawing::DrawingMode::Select, canvas, "Select Tool: Click to select objects");
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select");
  ImGui::PopStyleColor();
  
  ImGui::SameLine(0, buttonSpacing);
  
  if (ImGui::Button("Clear", ImVec2(buttonSize, buttonSize))) {
    canvas.clearAll();
    UIState::consoleMessage = "All shapes cleared";
  }
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Clear All");
  
  ImGui::EndChild();
  ImGui::PopStyleColor(); // Child bg
  ImGui::EndGroup();
  
  ImGui::End();
  ImGui::PopStyleVar();
}

void RenderStatusBar(Drawing::Canvas &canvas) {
  // AutoCAD-style status bar with command line at the bottom
  const float commandLineHeight = 25.0f;
  const float statusBarHeight = 28.0f;
  const float totalHeight = commandLineHeight + statusBarHeight;
  
  // Command line (dark background with light text)
  ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetIO().DisplaySize.y - totalHeight));
  ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, commandLineHeight));
  ImGui::PushStyleColor(ImGuiCol_WindowBg, UIColors::COMMAND_BG);
  ImGui::PushStyleColor(ImGuiCol_Text, UIColors::COMMAND_TEXT);
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
  ImGui::Begin("CommandLine", nullptr,
               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

  // Command prompt
  ImGui::Text("Command: ");
  ImGui::SameLine();
  
  ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 50.0f);
  if (UIState::focusCommandLine) {
    ImGui::SetKeyboardFocusHere();
    UIState::focusCommandLine = false;
  }
  
  if (ImGui::InputText("##commandInput", UIState::commandBuffer, IM_ARRAYSIZE(UIState::commandBuffer), 
                      ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
    // Process command when Enter is pressed
    std::string command = UIState::commandBuffer;
    if (!command.empty()) {
      // Process the command
      ProcessCommand(command, canvas);
      
      // Clear the command buffer
      UIState::commandBuffer[0] = '\0';
    }
  }
  ImGui::PopItemWidth();
  
  // Command history navigation using up/down keys
  if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) && !UIState::commandHistory.empty()) {
    if (UIState::commandHistoryPos < 0)
      UIState::commandHistoryPos = UIState::commandHistory.size() - 1;
    else if (UIState::commandHistoryPos > 0)
      UIState::commandHistoryPos--;
      
    if (UIState::commandHistoryPos >= 0 && UIState::commandHistoryPos < UIState::commandHistory.size()) {
      strncpy(UIState::commandBuffer, UIState::commandHistory[UIState::commandHistoryPos].c_str(), 
              IM_ARRAYSIZE(UIState::commandBuffer) - 1);
      UIState::focusCommandLine = true;
    }
  }
  
  if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) && !UIState::commandHistory.empty()) {
    if (UIState::commandHistoryPos < UIState::commandHistory.size() - 1) {
      UIState::commandHistoryPos++;
      strncpy(UIState::commandBuffer, UIState::commandHistory[UIState::commandHistoryPos].c_str(), 
              IM_ARRAYSIZE(UIState::commandBuffer) - 1);
    } else {
      UIState::commandHistoryPos = -1;
      UIState::commandBuffer[0] = '\0';
    }
    UIState::focusCommandLine = true;
  }
  
  ImGui::End();
  ImGui::PopStyleColor(3);
  
  // Status bar (below command line) - full width
  ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetIO().DisplaySize.y - statusBarHeight));
  ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, statusBarHeight));
  ImGui::PushStyleColor(ImGuiCol_WindowBg, UIColors::DARK_PANEL);
  ImGui::Begin("StatusBar", nullptr,
               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

  // Status sections with separators - evenly spaced across the full width
  ImVec2 mousePos = ImGui::GetMousePos();
  float windowWidth = ImGui::GetWindowWidth();
  
  // Calculate section widths for even distribution
  float section1Width = windowWidth * 0.22f; // Coordinates
  float section2Width = windowWidth * 0.35f; // Status message
  float section3Width = windowWidth * 0.22f; // Zoom level
  float section4Width = windowWidth * 0.21f; // Grid size
  
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
  ImGui::Text("X: %.2f%s  Y: %.2f%s", scaledX, unitSuffix.c_str(), scaledY, unitSuffix.c_str());
  
  ImGui::SameLine(section1Width);
  ImGui::Text("|");
  ImGui::SameLine(section1Width + 10);
  
  // Section 2: Status message
  ImGui::Text("%s", UIState::consoleMessage.c_str());
  
  ImGui::SameLine(section1Width + section2Width);
  ImGui::Text("|");
  ImGui::SameLine(section1Width + section2Width + 10);
  
  // Section 3: Zoom level
  ImGui::Text("Zoom: %.0f%%", UIState::zoomLevel * 100.0f);
  
  ImGui::SameLine(section1Width + section2Width + section3Width);
  ImGui::Text("|");
  ImGui::SameLine(section1Width + section2Width + section3Width + 10);
  
  // Section 4: Grid size indicator
  ImGui::Text("Grid: %d%s", (int)UIState::gridSize, unitSuffix.c_str());

  ImGui::End();
  ImGui::PopStyleColor();
}

void RenderPropertyPanel(Drawing::Canvas &canvas) {
  // AutoCAD-style properties panel on the right
  ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 250, 95));
  ImGui::SetNextWindowSize(ImVec2(250, ImGui::GetIO().DisplaySize.y - 150));

  ImGui::Begin("Properties", nullptr,
               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoCollapse);

  // Get the selected shape
  const Drawing::Shape* selectedShape = canvas.getSelectedShape();

  ImGui::PushStyleColor(ImGuiCol_Header, UIColors::HEADER);
  
  if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Indent(10);
    ImGui::AlignTextToFramePadding();
    
    ImGui::Text("Workspace:");
    ImGui::SameLine(120);
    ImGui::Text("%s", UIState::currentWorkspace.c_str());
    
    ImGui::Text("Units:");
    ImGui::SameLine(120);
    const char* unitNames[] = { "Pixels", "Millimeters", "Centimeters", "Inches" };
    ImGui::Text("%s", unitNames[static_cast<int>(UIState::units)]);
    
    ImGui::Text("Grid Size:");
    ImGui::SameLine(120);
    ImGui::Text("%.0f", UIState::gridSize);
    
    ImGui::Text("Snap to Grid:");
    ImGui::SameLine(120);
    bool snapValue = UIState::snapEnabled;
    if (ImGui::Checkbox("##SnapToGrid", &snapValue)) {
        UIState::snapEnabled = snapValue;
        canvas.setSnapToGrid(UIState::snapEnabled);
        UIState::consoleMessage = UIState::snapEnabled ? "Snap to Grid: ON" : "Snap to Grid: OFF";
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("When enabled, points will snap to the\ngrid and existing geometry points");
    }
    
    ImGui::Unindent(10);
  }
  
  // Tool-specific properties
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
        ImGui::Text("Line Length:");
        ImGui::SameLine(120);
        if (ImGui::InputFloat("##LineLength", &lineLength, 1.0f, 10.0f, "%.1f")) {
          // Ensure length is positive
          if (lineLength < 0.1f) lineLength = 0.1f;
          // Update canvas with new line length
          canvas.setLineLength(lineLength);
        }
      } else {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Click and drag to draw line");
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Release to complete");
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
        ImGui::SameLine(120);
        if (ImGui::InputFloat("##CircleRadius", &circleRadius, 1.0f, 10.0f, "%.1f")) {
          // Ensure radius is positive
          if (circleRadius < 0.1f) circleRadius = 0.1f;
          // Update canvas with new radius
          canvas.setCircleRadius(circleRadius);
        }
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
        ImGui::SameLine(120);
        if (ImGui::InputFloat("##SquareSize", &squareSize, 1.0f, 10.0f, "%.1f")) {
          // Ensure size is positive
          if (squareSize < 0.1f) squareSize = 0.1f;
          // Update canvas with new size
          canvas.setSquareSize(squareSize);
        }
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
        ImGui::SameLine(120);
        if (ImGui::InputFloat("##RectWidth", &rectangleWidth, 1.0f, 10.0f, "%.1f")) {
          // Ensure width is positive
          if (rectangleWidth < 0.1f) rectangleWidth = 0.1f;
          // Update canvas with new width
          canvas.setRectangleWidth(rectangleWidth);
        }
        
        // Input field for rectangle height
        ImGui::Text("Height:");
        ImGui::SameLine(120);
        if (ImGui::InputFloat("##RectHeight", &rectangleHeight, 1.0f, 10.0f, "%.1f")) {
          // Ensure height is positive
          if (rectangleHeight < 0.1f) rectangleHeight = 0.1f;
          // Update canvas with new height
          canvas.setRectangleHeight(rectangleHeight);
        }
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
        ImGui::SameLine(120);
        if (ImGui::InputFloat("##TriangleSide", &triangleSide, 1.0f, 10.0f, "%.1f")) {
          // Ensure side length is positive
          if (triangleSide < 0.1f) triangleSide = 0.1f;
          // Update canvas with new side length
          canvas.setTriangleSide(triangleSide);
        }
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
  
  // Selected object properties
  if (ImGui::CollapsingHeader("Object Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Indent(10);
    
    // Get selected shape
    Drawing::Shape *selectedShape = canvas.getSelectedShape();

    if (selectedShape) {
      // ... existing shape type handling ...
      
      // Add Bellows shape properties
      if (selectedShape->type == Drawing::ShapeType::BELLOWS) {
        Drawing::Bellows* bellows = static_cast<Drawing::Bellows*>(selectedShape);
        
        // Ensure the bellows is properly selected
        bellows->isSelected = true;
        
        ImGui::SeparatorText("Bellows Design Module");
        
        // Overall dimensions with validation
        float overallLength = bellows->calculateOverallLength();
        if (ImGui::DragFloat("Overall Length (mm)", &overallLength, 1.0f, 20.0f, 1000.0f, "%.1f")) {
          bellows->updateFromOverallLength(overallLength);
        }
        
        ImGui::Separator();
        
        // Cuff parameters section with validation feedback
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.2f, 0.3f, 0.7f));
        if (ImGui::CollapsingHeader("Cuff Dimensions", ImGuiTreeNodeFlags_DefaultOpen)) {
          ImGui::PushItemWidth(150.0f);
          
          bool validCuffDiameters = bellows->cuffAInnerDiameter <= bellows->baseConvolutionDiameter &&
                                   bellows->cuffBInnerDiameter <= bellows->baseConvolutionDiameter;
          
          if (!validCuffDiameters) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
            ImGui::TextWrapped("Warning: Cuff diameters should be less than or equal to Base Diameter");
            ImGui::PopStyleColor();
          }
          
          // Ensure min/max limits are enforced
          if (ImGui::DragFloat("Cuff A Inner Diameter (mm)", &bellows->cuffAInnerDiameter, 0.5f, 5.0f, 200.0f, "%.1f")) {
            // Validate input - ensure it's positive
            if (bellows->cuffAInnerDiameter < 5.0f) bellows->cuffAInnerDiameter = 5.0f;
          }
          
          if (ImGui::DragFloat("Cuff B Inner Diameter (mm)", &bellows->cuffBInnerDiameter, 0.5f, 5.0f, 200.0f, "%.1f")) {
            // Validate input - ensure it's positive
            if (bellows->cuffBInnerDiameter < 5.0f) bellows->cuffBInnerDiameter = 5.0f;
          }
          
          if (ImGui::DragFloat("Cuff A Length (mm)", &bellows->cuffALength, 0.5f, 5.0f, 100.0f, "%.1f")) {
            // Validate input - ensure it's positive
            if (bellows->cuffALength < 5.0f) bellows->cuffALength = 5.0f;
            
            // Update overall length
            overallLength = bellows->calculateOverallLength();
          }
          
          if (ImGui::DragFloat("Cuff B Length (mm)", &bellows->cuffBLength, 0.5f, 5.0f, 100.0f, "%.1f")) {
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
          ImGui::PushItemWidth(150.0f);
          
          bool validDiameters = bellows->baseConvolutionDiameter < bellows->peakConvolutionDiameter;
          if (!validDiameters) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
            ImGui::TextWrapped("Warning: Peak Diameter must be greater than Base Diameter");
            ImGui::PopStyleColor();
          }
          
          // Calculate minimum base diameter based on cuff diameters
          float minBaseDiameter = std::max(bellows->cuffAInnerDiameter, bellows->cuffBInnerDiameter);
          if (minBaseDiameter < 10.0f) minBaseDiameter = 10.0f;
          
          if (ImGui::DragFloat("Base Diameter (mm)", &bellows->baseConvolutionDiameter, 0.5f, 
                           minBaseDiameter, 300.0f, "%.1f")) {
            // Ensure peak diameter is always greater than base
            if (bellows->peakConvolutionDiameter <= bellows->baseConvolutionDiameter) {
              bellows->peakConvolutionDiameter = bellows->baseConvolutionDiameter + 5.0f;
            }
          }
          
          if (ImGui::DragFloat("Peak Diameter (mm)", &bellows->peakConvolutionDiameter, 0.5f, 
                           bellows->baseConvolutionDiameter + 0.1f, 350.0f, "%.1f")) {
            // Enforce the constraint directly in the UI
            if (bellows->peakConvolutionDiameter <= bellows->baseConvolutionDiameter) {
              bellows->peakConvolutionDiameter = bellows->baseConvolutionDiameter + 0.1f;
            }
          }
          
          // Prevent negative or zero convolution section length
          if (ImGui::DragFloat("Convoluted Section Length (mm)", &bellows->convolutedSectionLength, 1.0f, 10.0f, 500.0f, "%.1f")) {
            if (bellows->convolutedSectionLength < 10.0f) {
              bellows->convolutedSectionLength = 10.0f;
            }
            
            // Update overall length
            overallLength = bellows->calculateOverallLength();
          }
          
          // Enforce reasonable limits for number of convolutions
          if (ImGui::DragInt("Number of Convolutions", &bellows->numConvolutions, 1, 1, 30)) {
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
          ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Convolution Spacing: %.1f mm", convolutionSpacing);
          
          ImGui::PopItemWidth();
        }
        ImGui::PopStyleColor();
        
        // Wall thickness with validation
        ImGui::PushItemWidth(150.0f);
        
        // Calculate reasonable limits for wall thickness
        float maxWallThickness = std::min(bellows->baseConvolutionDiameter / 4.0f, 
                                       (bellows->peakConvolutionDiameter - bellows->baseConvolutionDiameter) / 2.0f);
        if (maxWallThickness < 0.5f) maxWallThickness = 0.5f;
        
        if (ImGui::DragFloat("Wall Thickness (mm)", &bellows->wallThickness, 0.1f, 0.5f, maxWallThickness, "%.1f")) {
          // Ensure wall thickness stays within reasonable limits
          if (bellows->wallThickness < 0.5f) bellows->wallThickness = 0.5f;
          if (bellows->wallThickness > maxWallThickness) bellows->wallThickness = maxWallThickness;
        }
        
        bool validWallThickness = bellows->wallThickness <= bellows->baseConvolutionDiameter / 4.0f &&
                                 bellows->wallThickness <= (bellows->peakConvolutionDiameter - bellows->baseConvolutionDiameter) / 2.0f;
        
        if (!validWallThickness) {
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
          ImGui::TextWrapped("Warning: Wall thickness too large for current dimensions");
          ImGui::PopStyleColor();
        }
        
        ImGui::PopItemWidth();
        
        // Display options
        ImGui::Checkbox("Show Dimensions", &bellows->showDimensions);
        
        ImGui::Separator();
        
        // Action buttons
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.6f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.5f, 0.7f, 1.0f));
        
        // Generate profile button
        if (ImGui::Button("Generate Profile", ImVec2(150, 30))) {
          // Validate parameters
          if (!bellows->validateParameters()) {
            ImGui::OpenPopup("Invalid Parameters");
          } else {
            // Force a refresh of the profile
            bellows->isSelected = true;
          }
        }
        
        ImGui::SameLine();
        
        // Reset parameters button
        if (ImGui::Button("Reset Parameters", ImVec2(150, 30))) {
          bellows->resetParameters();
        }
        
        // Fit to view button
        if (ImGui::Button("Fit to View", ImVec2(150, 30))) {
          canvas.fitBellowsToView();
        }
        
        ImGui::PopStyleColor(2);
        
        // Validation popup
        if (ImGui::BeginPopupModal("Invalid Parameters", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
          ImGui::Text("Please check the following parameters:");
          
          if (bellows->cuffAInnerDiameter <= 0.0f || bellows->cuffBInnerDiameter <= 0.0f)
            ImGui::BulletText("Cuff inner diameters must be positive");
            
          if (bellows->cuffALength <= 0.0f || bellows->cuffBLength <= 0.0f)
            ImGui::BulletText("Cuff lengths must be positive");
            
          if (bellows->baseConvolutionDiameter <= 0.0f || bellows->peakConvolutionDiameter <= 0.0f)
            ImGui::BulletText("Convolution diameters must be positive");
            
          if (bellows->wallThickness <= 0.0f)
            ImGui::BulletText("Wall thickness must be positive");
            
          if (bellows->cuffAInnerDiameter > bellows->baseConvolutionDiameter ||
              bellows->cuffBInnerDiameter > bellows->baseConvolutionDiameter)
            ImGui::BulletText("Cuff diameters must be ≤ Base Diameter");
            
          if (bellows->baseConvolutionDiameter >= bellows->peakConvolutionDiameter)
            ImGui::BulletText("Base Diameter must be < Peak Diameter");
            
          if (bellows->wallThickness > bellows->baseConvolutionDiameter / 4.0f ||
              bellows->wallThickness > (bellows->peakConvolutionDiameter - bellows->baseConvolutionDiameter) / 2.0f)
            ImGui::BulletText("Wall thickness is too large for current dimensions");
          
          if (ImGui::Button("Fix Parameters Automatically", ImVec2(220, 0))) {
            // Auto-fix the parameters
            
            // Fix diameters
            if (bellows->baseConvolutionDiameter <= 0.0f) bellows->baseConvolutionDiameter = 60.0f;
            if (bellows->peakConvolutionDiameter <= bellows->baseConvolutionDiameter)
              bellows->peakConvolutionDiameter = bellows->baseConvolutionDiameter + 20.0f;
              
            // Fix cuff diameters
            if (bellows->cuffAInnerDiameter <= 0.0f) bellows->cuffAInnerDiameter = 50.0f;
            if (bellows->cuffBInnerDiameter <= 0.0f) bellows->cuffBInnerDiameter = 50.0f;
            
            if (bellows->cuffAInnerDiameter > bellows->baseConvolutionDiameter)
              bellows->cuffAInnerDiameter = bellows->baseConvolutionDiameter;
            if (bellows->cuffBInnerDiameter > bellows->baseConvolutionDiameter)
              bellows->cuffBInnerDiameter = bellows->baseConvolutionDiameter;
              
            // Fix lengths
            if (bellows->cuffALength <= 0.0f) bellows->cuffALength = 20.0f;
            if (bellows->cuffBLength <= 0.0f) bellows->cuffBLength = 20.0f;
            if (bellows->convolutedSectionLength <= 0.0f) bellows->convolutedSectionLength = 100.0f;
            
            // Fix wall thickness
            float maxWallThickness = std::min(bellows->baseConvolutionDiameter / 4.0f, 
                                           (bellows->peakConvolutionDiameter - bellows->baseConvolutionDiameter) / 2.0f);
            if (bellows->wallThickness <= 0.0f || bellows->wallThickness > maxWallThickness)
              bellows->wallThickness = std::min(2.0f, maxWallThickness);
              
            // Fix number of convolutions
            if (bellows->numConvolutions <= 0) bellows->numConvolutions = 6;
              
            ImGui::CloseCurrentPopup();
          }
          
          ImGui::SameLine();
          
          if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
          }
          ImGui::EndPopup();
        }
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
      ImGui::Indent(10);
      
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
        
        // Draw 3D View button
        ImGui::Separator();
        if (ImGui::Button("3D View", ImVec2(-1, 30))) {
          UIState::show3DView = true;
          UIState::bellows3DViewInitialized = false;
        }
        
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
  const float commandAndStatusHeight = 53.0f; // Command line (25) + status bar (28)
  const float propertiesPanelWidth = 250.0f;
  
  float canvasX = 0.0f;
  float canvasY = ribbonHeight;
  float canvasWidth = ImGui::GetIO().DisplaySize.x - propertiesPanelWidth;
  float canvasHeight = ImGui::GetIO().DisplaySize.y - ribbonHeight - commandAndStatusHeight;

  ImGui::SetNextWindowPos(ImVec2(canvasX, canvasY));
  ImGui::SetNextWindowSize(ImVec2(canvasWidth, canvasHeight));

  // Set canvas background to a slightly off-white color for better grid visibility
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

// Process command line input
void ProcessCommand(const std::string &command, Drawing::Canvas &canvas) {
  if (command.empty()) return;

  // Add to command history
  UIState::commandHistory.push_back(command);
  UIState::commandHistoryPos = UIState::commandHistory.size();

  // Process command
  if (command == "line" || command == "l") {
    UIState::consoleMessage = "Line command activated";
    // Would call SelectTool here
  } else if (command == "circle" || command == "c") {
    UIState::consoleMessage = "Circle command activated";
    // Would call SelectTool here
  } else if (command == "rect" || command == "rectangle") {
    UIState::consoleMessage = "Rectangle command activated";
    // Would call SelectTool here
  } else if (command == "grid") {
    bool currentGridState = canvas.isGridVisible();
    canvas.setShowGrid(!currentGridState);
    UIState::consoleMessage = !currentGridState ? "Grid turned ON" : "Grid turned OFF";
  } else if (command == "help") {
    UIState::consoleMessage = "Available commands: line, circle, rect, grid, help";
  } else {
    UIState::consoleMessage = "Unknown command: " + command;
  }
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
    else if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
      UIState::focusCommandLine = true;
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
      "CAD Drawing Application", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
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
      
      // Handle middle mouse button panning (AutoCAD-like)
      if (event.type == SDL_MOUSEMOTION && (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_MIDDLE))) {
        UIState::panOffset.x += event.motion.xrel / UIState::zoomLevel;
        UIState::panOffset.y += event.motion.yrel / UIState::zoomLevel;
        UIState::consoleMessage = "Pan View";
      }
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Handle keyboard shortcuts
    HandleKeyboardShortcuts(canvas);

    // Render the AutoCAD-like UI
    RenderTopRibbon(canvas);
    RenderCanvas(canvas);
    RenderPropertyPanel(canvas);
    RenderStatusBar(canvas);
    
    // Render 3D view window if active
    Render3DViewWindow(canvas);

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

// Add after the RenderPropertyPanel function

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

void Render3DViewWindow(Drawing::Canvas &canvas) {
  // Only show if enabled
  if (!UIState::show3DView) return;
  
  // Get selected bellows shape
  const Drawing::Shape* selectedShape = canvas.getSelectedShape();
  if (!selectedShape || selectedShape->type != Drawing::ShapeType::BELLOWS) {
    UIState::show3DView = false;
    return;
  }
  
  // First static_cast to const Drawing::Bellows*, then const_cast to remove const
  const Drawing::Bellows* const_bellows = static_cast<const Drawing::Bellows*>(selectedShape);
  Drawing::Bellows* bellows = const_cast<Drawing::Bellows*>(const_bellows);
  
  // Create a centered window that's 80% of the screen size
  ImVec2 screenSize = ImGui::GetIO().DisplaySize;
  ImVec2 windowSize(screenSize.x * 0.8f, screenSize.y * 0.8f);
  ImVec2 windowPos((screenSize.x - windowSize.x) * 0.5f, (screenSize.y - windowSize.y) * 0.5f);
  
  ImGui::SetNextWindowPos(windowPos, ImGuiCond_Appearing);
  ImGui::SetNextWindowSize(windowSize, ImGuiCond_Appearing);
  
  // Get the static viewer instance
  BellowsViewer3D& viewer = GetBellowsViewer();
  
  // Ensure the viewer is initialized
  if (!UIState::bellows3DViewInitialized) {
    UIState::bellows3DViewInitialized = true;
  }
  
  // Render the 3D view window
  if (ImGui::Begin("3D Bellows View", &UIState::show3DView, 
                  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
    
    // Split window into two parts: viewer and settings
    ImVec2 availSize = ImGui::GetContentRegionAvail();
    ImVec2 viewerSize(availSize.x * 0.75f, availSize.y);
    
    // Render the 3D model
    viewer.render(bellows, viewerSize);
    
    // Settings panel on the right
    ImGui::SameLine();
    viewer.showSettingsPanel();
    
    ImGui::End();
  }
}

// Helper function to handle 3D viewer input
void Handle3DViewerInput(const SDL_Event& event) {
  if (!UIState::show3DView) return;
  
  // Get the static viewer instance
  BellowsViewer3D& viewer = GetBellowsViewer();
  
  // Pass the event to the viewer
  viewer.handleInput(event);
}
