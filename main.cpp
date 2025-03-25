#include "Canvas.hpp"
#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>
#include <iostream>

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
static enum class UnitSystem { Pixels, Millimeters, Centimeters, Inches } units = UnitSystem::Pixels;
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
  ImGui::BeginChild("CurvesPanel", ImVec2(155, panelHeight), true);
  
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
    case UIState::UnitSystem::Pixels:
      unitSuffix = "px";
      break;
    case UIState::UnitSystem::Millimeters:
      unitSuffix = "mm";
      break;
    case UIState::UnitSystem::Centimeters:
      unitSuffix = "cm";
      break;
    case UIState::UnitSystem::Inches:
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
  
  // Selected object properties
  if (ImGui::CollapsingHeader("Object Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Indent(10);
    
    // This would normally be filled with the selected object's properties
    // For now, just display a placeholder
    ImGui::Text("No object selected");
    ImGui::Text("Select an object to");
    ImGui::Text("view its properties");
    
    ImGui::Unindent(10);
  }
  
  ImGui::PopStyleColor(); // Header color
  
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
        case UIState::UnitSystem::Pixels:
          unitSuffix = "px";
          break;
        case UIState::UnitSystem::Millimeters:
          unitSuffix = "mm";
          break;
        case UIState::UnitSystem::Centimeters:
          unitSuffix = "cm";
          break;
        case UIState::UnitSystem::Inches:
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

int main(int argc, char *argv[]) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::cerr << "Error: " << SDL_GetError() << std::endl;
    return -1;
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

  // Create window with AutoCAD-like title
  SDL_Window *window = SDL_CreateWindow(
      "Drawing Application", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      WINDOW_WIDTH, WINDOW_HEIGHT,
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

  if (!window) {
    std::cerr << "Error: " << SDL_GetError() << std::endl;
    return -1;
  }

  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
  SDL_GL_MakeCurrent(window, gl_context);
  SDL_GL_SetSwapInterval(1);

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
