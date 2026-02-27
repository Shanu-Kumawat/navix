#include "ui/UIState.hpp"
#include "ui/UIHelpers.hpp"
#include "ui/TopRibbon.hpp"
#include <imgui.h>
#include "shapes/BasicShapes.hpp"
#include "shapes/ComplexShapes.hpp"
#include "Constants.hpp"

namespace UI {

void TopRibbon::Render(Drawing::Canvas &canvas, Core::ApplicationContext& appContext) {

  // Professional style ribbon at the top
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, 145)); // Increased to fit View panel with slider

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
  const float panelHeight = 125.0f; // Height to fit View panel contents
  
  // For icon buttons, calculate size based on icon size and ensure proper centering
  const float iconSize = 20.0f; // Size of the icon font
  const float iconButtonPadding = 12.0f; // More padding to center icons properly
  const float iconButtonSize = iconSize + iconButtonPadding; // Square buttons for icons (32x32)
  
  // Calculate how many icon buttons per row based on available width
  const int iconsPerRow = std::max(2, static_cast<int>((panelWidth - 16.0f) / (iconButtonSize + 6.0f)));
  // Button spacing between icon buttons
  const float iconButtonSpacing = 6.0f;
  
  // For text buttons (like "Bellows", "Ball Bearing"), use larger size
  const float textWidth = ImGui::CalcTextSize("Ball Bearing").x;
  const float textButtonWidth = textWidth + 16.0f;
  const float textButtonHeight = 30.0f;
  
  // Draw Panel
  ImGui::BeginGroup();
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.98f, 0.98f, 0.98f, 1.0f));
  // Enable vertical scrolling for the drawing tools panel when content overflows
  ImGui::BeginChild("DrawPanel", ImVec2(panelWidth, panelHeight), true, 
                    ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NavFlattened);
  
  ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
  ImGui::PushStyleColor(ImGuiCol_Text, UIColors::TEXT_DIM);
  ImGui::Text("Draw");
  ImGui::PopStyleColor();
  ImGui::PopFont();
  
  ImGui::Dummy(ImVec2(0, 2));
  
  // Row 1: Basic drawing tools - use compact icon buttons
  float y = ImGui::GetCursorPosY();
  bool selected;
  
  // Push style for icon buttons to ensure proper centering
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 6));
  
  selected = appContext.activeMode == Drawing::DrawingMode::Point;
  if (IconButton("point", FALLBACK_POINT, "Point Tool", ImVec2(iconButtonSize, iconButtonSize))) {
    SelectTool(Drawing::DrawingMode::Point, canvas, appContext, "Point Tool: Click to place points");
  }
  
  ImGui::SameLine(0, iconButtonSpacing);
  selected = appContext.activeMode == Drawing::DrawingMode::Line;
  if (IconButton("line", FALLBACK_LINE, "Line Tool", ImVec2(iconButtonSize, iconButtonSize))) {
    SelectTool(Drawing::DrawingMode::Line, canvas, appContext, "Line Tool: Click to set start and end points");
  }
  
  if (iconsPerRow > 2) {
    ImGui::SameLine(0, iconButtonSpacing);
  } else {
    ImGui::Dummy(ImVec2(0, 3));
  }
  
  selected = appContext.activeMode == Drawing::DrawingMode::Circle;
  if (IconButton("circle", FALLBACK_CIRCLE, "Circle Tool", ImVec2(iconButtonSize, iconButtonSize))) {
    SelectTool(Drawing::DrawingMode::Circle, canvas, appContext, "Circle Tool: Click to set center and radius");
  }

  if (iconsPerRow > 3) {
    ImGui::SameLine(0, iconButtonSpacing);
  } else {
    ImGui::Dummy(ImVec2(0, 3));
  }
  
  selected = appContext.activeMode == Drawing::DrawingMode::Triangle;
  if (IconButton("triangle", FALLBACK_TRIANGLE, "Triangle Tool", ImVec2(iconButtonSize, iconButtonSize))) {
    SelectTool(Drawing::DrawingMode::Triangle, canvas, appContext, "Triangle Tool: Click to set point and direction (dimensions in properties)");
  }

  if (iconsPerRow > 4) {
    ImGui::SameLine(0, iconButtonSpacing);
  } else {
    ImGui::Dummy(ImVec2(0, 3));
  }
  
  selected = appContext.activeMode == Drawing::DrawingMode::Square;
  if (IconButton("square", FALLBACK_SQUARE, "Square Tool", ImVec2(iconButtonSize, iconButtonSize))) {
    SelectTool(Drawing::DrawingMode::Square, canvas, appContext, "Square Tool: Click to set corner and direction (side length in properties)");
  }

  if (iconsPerRow > 5) {
    ImGui::SameLine(0, iconButtonSpacing);
  } else {
    ImGui::Dummy(ImVec2(0, 3));
  }
  
  selected = appContext.activeMode == Drawing::DrawingMode::Rectangle;
  if (IconButton("rectangle", FALLBACK_RECTANGLE, "Rectangle Tool", ImVec2(iconButtonSize, iconButtonSize))) {
    SelectTool(Drawing::DrawingMode::Rectangle, canvas, appContext, "Rectangle Tool: Click to set corner and direction (dimensions in properties)");
  }
  
  // Pop the icon button style
  ImGui::PopStyleVar();

  // Start new row for additional tools
  ImGui::Dummy(ImVec2(0, 2));
  
  // Row 2: Additional drawing tools
  // Push style for icon buttons to ensure proper centering
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 6));
  
  selected = appContext.activeMode == Drawing::DrawingMode::Spline;
  if (IconButton("spline", FALLBACK_SPLINE, "Spline Tool", ImVec2(iconButtonSize, iconButtonSize))) {
    SelectTool(Drawing::DrawingMode::Spline, canvas, appContext, "Spline Tool: Click to add control points, double-click to finish");
  }
  
  ImGui::SameLine(0, iconButtonSpacing);
  selected = appContext.activeMode == Drawing::DrawingMode::BezierCurve;
  if (IconButton("bezier", FALLBACK_BEZIER, "Bezier Curve Tool", ImVec2(iconButtonSize, iconButtonSize))) {
    SelectTool(Drawing::DrawingMode::BezierCurve, canvas, appContext, "Bezier Tool: Click to add control points, double-click to finish");
  }
  
  // Pop the icon button style
  ImGui::PopStyleVar();

  if (iconsPerRow > 2) {
    ImGui::SameLine(0, iconButtonSpacing);
  } else {
    ImGui::Dummy(ImVec2(0, 3));
  }
  
  selected = appContext.activeMode == Drawing::DrawingMode::Bellows;
  if (IconButton("bellows", "Bellows", "Bellows Tool: Click to create a parametric bellows", ImVec2(textButtonWidth, textButtonHeight))) {
    SelectTool(Drawing::DrawingMode::Bellows, canvas, appContext, "Bellows Tool: Click to create a parametric bellows");
  }
  
  if (iconsPerRow > 2) {
    ImGui::SameLine(0, iconButtonSpacing);
  } else {
    ImGui::Dummy(ImVec2(0, 3));
  }
  
  selected = appContext.activeMode == Drawing::DrawingMode::BallBearing;
  if (IconButton("bearing", "Ball Bearing", "Ball Bearing Tool: Click to create a parametric ball bearing", ImVec2(textButtonWidth, textButtonHeight))) {
    SelectTool(Drawing::DrawingMode::BallBearing, canvas, appContext, "Ball Bearing Tool: Click to create a parametric ball bearing");
  }
  
  ImGui::SameLine(0, iconButtonSpacing);
  selected = appContext.activeMode == Drawing::DrawingMode::Spring2D;
  if (IconButton("suspension", "Spring", "Spring Tool: Set parameters in properties panel", ImVec2(textButtonWidth, textButtonHeight))) {
    SelectTool(Drawing::DrawingMode::Spring2D, canvas, appContext, "Spring Tool: Set parameters in properties panel");
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
  
  // Edit tools - use compact icon buttons
  // Push style for icon buttons to ensure proper centering
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 6));
  
  selected = appContext.activeMode == Drawing::DrawingMode::Select;
  if (IconButton("select", FALLBACK_SELECT, "Select Tool", ImVec2(iconButtonSize, iconButtonSize))) {
    SelectTool(Drawing::DrawingMode::Select, canvas, appContext, "Select Tool: Click to select objects");
  }
  
  ImGui::SameLine(0, iconButtonSpacing);
  if (IconButton("undo", FALLBACK_UNDO, "Undo", ImVec2(iconButtonSize, iconButtonSize))) {
    canvas.undo();
    appContext.consoleMessage = "Undo";
  }
  
  // Pop the icon button style
  ImGui::PopStyleVar();
  
  if (iconsPerRow > 2) {
    ImGui::SameLine(0, iconButtonSpacing);
  } else {
    ImGui::Dummy(ImVec2(0, 3));
  }
  
  if (IconButton("redo", FALLBACK_REDO, "##redo_tool", ImVec2(iconButtonSize, iconButtonSize))) {
    canvas.redo();
    appContext.consoleMessage = "Redo";
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Redo");
  }
  
  if (iconsPerRow > 2) {
    ImGui::SameLine(0, iconButtonSpacing);
  } else {
    ImGui::Dummy(ImVec2(0, 3));
  }
  
  // Add Clear All button using text button size
  if (IconButton("clear", "Clear All", "Clear All: Remove all shapes from canvas", ImVec2(textButtonWidth, textButtonHeight))) {
    canvas.clearAll();
    appContext.consoleMessage = "All shapes cleared";
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
  ImGui::PushItemWidth(80.0f); // Fixed width for slider that fits in panel
  if (ImGui::Checkbox("Grid", &showGrid)) {
    canvas.setShowGrid(showGrid);
    appContext.consoleMessage = showGrid ? "Grid: ON" : "Grid: OFF";
  }
  
  bool snapToGrid = canvas.isSnapToGridEnabled();
  if (ImGui::Checkbox("Snap", &snapToGrid)) {
    canvas.setSnapToGrid(snapToGrid);
    appContext.snapEnabled = snapToGrid;
    appContext.consoleMessage = snapToGrid ? "Snap to Grid: ON" : "Snap to Grid: OFF";
  }
  
  // Grid size slider
  float gridSize = appContext.gridSize;
  if (ImGui::SliderFloat("Size", &gridSize, 5.0f, 50.0f, "%.0f")) {
    appContext.gridSize = gridSize;
    canvas.setGridSpacing(gridSize);
    appContext.consoleMessage = "Grid Size Updated";
  }
  ImGui::PopItemWidth();
  
  ImGui::EndChild();
  ImGui::PopStyleColor();
  ImGui::EndGroup();
  
  // Right-aligned section for 3D view button
  ImGui::SameLine();
  
  // Shape-specific 3D view buttons
  ImGui::PushStyleColor(ImGuiCol_Button, UIColors::ACCENT);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.73f, 0.73f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
  
  // Check if we have any complex shapes to show 3D views for
  Drawing::Shape* selectedShape = canvas.getSelectedShape();
  bool hasComplexShapes = false;
  
  if (selectedShape) {
    if (selectedShape->type == Drawing::ShapeType::BELLOWS) {
      if (ImGui::Button("3D Bellows", ImVec2(100, 30))) {
        UIState::showBellows3DView = true;
        appContext.consoleMessage = "Opening 3D Bellows View";
      }
      hasComplexShapes = true;
    }
    else if (selectedShape->type == Drawing::ShapeType::BALL_BEARING) {
      if (ImGui::Button("3D Ball Bearing", ImVec2(100, 30))) {
        UIState::showBallBearing3DView = true;
        appContext.consoleMessage = "Opening 3D Ball Bearing View";
      }
      hasComplexShapes = true;
    }
  }
  
  // If no complex shape selected, show a disabled button with tooltip
  if (!hasComplexShapes) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    ImGui::Button("3D View", ImVec2(100, 30));
    ImGui::PopStyleColor(1);
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Select a complex shape (Bellows, Ball Bearing, etc.) to view in 3D");
    }
  }
  
  ImGui::PopStyleColor(3);
  
  ImGui::End();
  ImGui::PopStyleVar();
}
} // namespace UI
