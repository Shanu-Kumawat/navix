#include "ui/TopRibbon.hpp"
#include "ui/UIHelpers.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include "shapes/BasicShapes.hpp"
#include "shapes/ComplexShapes.hpp"
#include "Constants.hpp"

namespace UI {

// Draw a subtle vertical divider between toolbar sections
static void ToolbarSep() {
  ImGui::SameLine(0, 8);
  ImVec2 p = ImGui::GetCursorScreenPos();
  ImGui::GetWindowDrawList()->AddLine(
    ImVec2(p.x, p.y + 2), ImVec2(p.x, p.y + 26),
    IM_COL32(55, 55, 65, 180), 1.0f);
  ImGui::Dummy(ImVec2(1, 28));
  ImGui::SameLine(0, 8);
}

void TopRibbon::Render(Drawing::Canvas &canvas, Core::ApplicationContext& appContext) {
  const float toolbarHeight = 80.0f;
  const float displayWidth = ImGui::GetIO().DisplaySize.x;
  const float btn = 30.0f;
  const float sp = 3.0f;

  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2(displayWidth, toolbarHeight));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 6));
  ImGui::PushStyleColor(ImGuiCol_WindowBg, UIColors::TOOLBAR);
  ImGui::Begin("##Toolbar", nullptr,
    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

  // ============================================================
  //  ROW 1 — Drawing Tools, Engineering Parts, Edit Actions
  // ============================================================

  // --- Brand ---
  ImGui::PushStyleColor(ImGuiCol_Text, UIColors::ACCENT);
  ImGui::SetCursorPosY(10);
  ImGui::Text("NAVIX");
  ImGui::PopStyleColor();
  ImGui::SameLine(0, 16);
  ToolbarSep();

  // --- Cursor / Select ---
  if (IconButton("select", FALLBACK_SELECT, "Select (Esc)", ImVec2(btn, btn),
      appContext.activeMode == Drawing::DrawingMode::Select)) {
    SelectTool(Drawing::DrawingMode::Select, canvas, appContext, "Select Tool: Click to select objects");
  }
  ImGui::SameLine(0, 8);
  ToolbarSep();

  // --- Basic Shapes ---
  if (IconButton("point", FALLBACK_POINT, "Point (P)", ImVec2(btn, btn),
      appContext.activeMode == Drawing::DrawingMode::Point))
    SelectTool(Drawing::DrawingMode::Point, canvas, appContext, "Point Tool: Click to place points");
  ImGui::SameLine(0, sp);

  if (IconButton("line", FALLBACK_LINE, "Line (L)", ImVec2(btn, btn),
      appContext.activeMode == Drawing::DrawingMode::Line))
    SelectTool(Drawing::DrawingMode::Line, canvas, appContext, "Line Tool: Click start & end points");
  ImGui::SameLine(0, sp);

  if (IconButton("circle", FALLBACK_CIRCLE, "Circle (C)", ImVec2(btn, btn),
      appContext.activeMode == Drawing::DrawingMode::Circle))
    SelectTool(Drawing::DrawingMode::Circle, canvas, appContext, "Circle Tool: Click center, drag radius");
  ImGui::SameLine(0, sp);

  if (IconButton("triangle", FALLBACK_TRIANGLE, "Triangle (T)", ImVec2(btn, btn),
      appContext.activeMode == Drawing::DrawingMode::Triangle))
    SelectTool(Drawing::DrawingMode::Triangle, canvas, appContext, "Triangle Tool: Click to place");
  ImGui::SameLine(0, sp);

  if (IconButton("square", FALLBACK_SQUARE, "Square (S)", ImVec2(btn, btn),
      appContext.activeMode == Drawing::DrawingMode::Square))
    SelectTool(Drawing::DrawingMode::Square, canvas, appContext, "Square Tool: Click to place");
  ImGui::SameLine(0, sp);

  if (IconButton("rectangle", FALLBACK_RECTANGLE, "Rectangle (R)", ImVec2(btn, btn),
      appContext.activeMode == Drawing::DrawingMode::Rectangle))
    SelectTool(Drawing::DrawingMode::Rectangle, canvas, appContext, "Rectangle Tool: Click to place");
  ImGui::SameLine(0, 8);
  ToolbarSep();

  // --- Curves ---
  if (IconButton("spline", FALLBACK_SPLINE, "Spline", ImVec2(btn, btn),
      appContext.activeMode == Drawing::DrawingMode::Spline))
    SelectTool(Drawing::DrawingMode::Spline, canvas, appContext, "Spline: Left-click points, right-click finish");
  ImGui::SameLine(0, sp);

  if (IconButton("bezier", FALLBACK_BEZIER, "Bezier Curve", ImVec2(btn, btn),
      appContext.activeMode == Drawing::DrawingMode::BezierCurve))
    SelectTool(Drawing::DrawingMode::BezierCurve, canvas, appContext, "Bezier: Left-click points, right-click finish");
  ImGui::SameLine(0, 8);
  ToolbarSep();

  // --- Engineering Components ---
  if (IconButton("bellows", FALLBACK_BELLOWS, "Bellows", ImVec2(btn, btn),
      appContext.activeMode == Drawing::DrawingMode::Bellows))
    SelectTool(Drawing::DrawingMode::Bellows, canvas, appContext, "Bellows Tool: Click to create");
  ImGui::SameLine(0, sp);

  if (IconButton("bearing", FALLBACK_BEARING, "Ball Bearing", ImVec2(btn, btn),
      appContext.activeMode == Drawing::DrawingMode::BallBearing))
    SelectTool(Drawing::DrawingMode::BallBearing, canvas, appContext, "Ball Bearing Tool: Click to create");
  ImGui::SameLine(0, sp);

  if (IconButton("suspension", FALLBACK_SUSPENSION, "Spring", ImVec2(btn, btn),
      appContext.activeMode == Drawing::DrawingMode::Spring2D))
    SelectTool(Drawing::DrawingMode::Spring2D, canvas, appContext, "Spring Tool: Set params in properties");
  ImGui::SameLine(0, 8);
  ToolbarSep();

  // --- Edit Actions ---
  if (IconButton("undo", FALLBACK_UNDO, "Undo (Ctrl+Z)", ImVec2(btn, btn))) {
    canvas.undo(); appContext.consoleMessage = "Undo";
  }
  ImGui::SameLine(0, sp);
  if (IconButton("redo", FALLBACK_REDO, "Redo (Ctrl+Y)", ImVec2(btn, btn))) {
    canvas.redo(); appContext.consoleMessage = "Redo";
  }
  ImGui::SameLine(0, sp);
  if (IconButton("clear", FALLBACK_CLEAR, "Clear All", ImVec2(btn, btn))) {
    canvas.clearAll(); appContext.consoleMessage = "All shapes cleared";
  }

  // --- Right-aligned 3D View Button ---
  {
    Drawing::Shape* sel = canvas.getSelectedShape();
    bool has3D = false;
    const char* label3D = "3D View";
    if (sel) {
      if (sel->type == Drawing::ShapeType::BELLOWS)      { label3D = "3D Bellows"; has3D = true; }
      else if (sel->type == Drawing::ShapeType::BALL_BEARING) { label3D = "3D Bearing"; has3D = true; }
      else if (sel->type == Drawing::ShapeType::SPRING2D)     { label3D = "3D Spring";  has3D = true; }
    }

    float rightX = displayWidth - 122;
    ImGui::SameLine(rightX);

    if (has3D) {
      ImGui::PushStyleColor(ImGuiCol_Button, UIColors::ACCENT);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, UIColors::ACCENT_HOVER);
      ImGui::PushStyleColor(ImGuiCol_Text, UIColors::TEXT_BRIGHT);
    } else {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.22f, 0.27f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.28f, 0.33f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_Text, UIColors::TEXT_DIM);
    }

    if (ImGui::Button(label3D, ImVec2(104, 28)) && has3D) {
      if (sel->type == Drawing::ShapeType::BELLOWS)           appContext.showBellows3DView = true;
      else if (sel->type == Drawing::ShapeType::BALL_BEARING) appContext.showBallBearing3DView = true;
      else if (sel->type == Drawing::ShapeType::SPRING2D)     appContext.showSpring3DView = true;
    }
    ImGui::PopStyleColor(3);

    if (!has3D && ImGui::IsItemHovered())
      ImGui::SetTooltip("Select a component (Bellows / Bearing / Spring) for 3D view");

    // Extra Shock Absorber button when Spring selected
    if (sel && sel->type == Drawing::ShapeType::SPRING2D) {
      ImGui::SameLine(0, 4);
      bool canSA = canvas.hasCompleteShockAbsorberAssembly();
      if (!canSA) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.22f, 0.27f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, UIColors::TEXT_DIM);
      } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.45f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, UIColors::TEXT_BRIGHT);
      }
      if (ImGui::Button("S.A.", ImVec2(38, 28)) && canSA)
        appContext.showShockAbsorber3DView = true;
      ImGui::PopStyleColor(2);
      if (!canSA && ImGui::IsItemHovered())
        ImGui::SetTooltip("Build full assembly (spring + top/bottom ends) first");
    }
  }

  // ============================================================
  //  ROW 2 — View Controls  |  Mesh  |  info
  // ============================================================
  ImGui::SetCursorPosY(46);

  // --- View Toggles ---
  ImGui::PushStyleColor(ImGuiCol_Text, UIColors::TEXT_DIM);
  ImGui::Text("View");
  ImGui::PopStyleColor();
  ImGui::SameLine(0, 8);

  bool showGrid = canvas.isGridVisible();
  if (ImGui::Checkbox("Grid", &showGrid)) {
    canvas.setShowGrid(showGrid);
    appContext.consoleMessage = showGrid ? "Grid: ON" : "Grid: OFF";
  }
  ImGui::SameLine(0, 10);

  bool snapToGrid = canvas.isSnapToGridEnabled();
  if (ImGui::Checkbox("Snap", &snapToGrid)) {
    canvas.setSnapToGrid(snapToGrid);
    appContext.snapEnabled = snapToGrid;
    appContext.consoleMessage = snapToGrid ? "Snap: ON" : "Snap: OFF";
  }
  ImGui::SameLine(0, 10);

  ImGui::PushItemWidth(72);
  float gridSize = appContext.gridSize;
  if (ImGui::SliderFloat("##GridSz", &gridSize, 5.0f, 50.0f, "%.0f")) {
    appContext.gridSize = gridSize;
    canvas.setGridSpacing(gridSize);
  }
  ImGui::PopItemWidth();

  ImGui::SameLine(0, 16);

  // --- Mesh Controls (right-aligned) ---
  {
    // Calculate right-aligned position for mesh controls
    // Mesh section width: "Mesh" label(30) + Generate(62) + Clear(44) + Show checkbox(50) + slider(64) + gaps ≈ 290
    float meshSectionWidth = 300.0f;
    float rightEdge = ImGui::GetWindowWidth() - 12.0f;
    float meshStartX = rightEdge - meshSectionWidth;
    if (meshStartX > ImGui::GetCursorPosX() + 20.0f) {
      ImGui::SetCursorPosX(meshStartX);
    } else {
      // Fallback: just add separator and continue inline
      ImVec2 p = ImGui::GetCursorScreenPos();
      ImGui::GetWindowDrawList()->AddLine(
        ImVec2(p.x, p.y + 1), ImVec2(p.x, p.y + 22),
        IM_COL32(55, 55, 65, 150), 1.0f);
      ImGui::Dummy(ImVec2(1, 24));
      ImGui::SameLine(0, 10);
    }
  }

  ImGui::PushStyleColor(ImGuiCol_Text, UIColors::TEXT_DIM);
  ImGui::Text("Mesh");
  ImGui::PopStyleColor();
  ImGui::SameLine(0, 6);

  {
    Drawing::Shape* sel = canvas.getSelectedShape();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.34f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.44f, 0.26f, 1.0f));
    if (ImGui::Button("Generate##mesh", ImVec2(76, 22))) {
      bool ok;
      if (sel) {
        ok = (canvas.generateMeshIncludingShape(sel, canvas.getMeshElementSize()), canvas.hasMesh());
        appContext.consoleMessage = ok ? "Mesh generated for selected shape" : "Mesh failed";
      } else {
        ok = canvas.generateMesh(canvas.getMeshElementSize());
        appContext.consoleMessage = ok ? "Mesh generated (all shapes)" : "Mesh failed — draw closed shapes";
      }
    }
    ImGui::PopStyleColor(2);

    ImGui::SameLine(0, 4);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.34f, 0.18f, 0.18f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.44f, 0.24f, 0.24f, 1.0f));
    if (ImGui::Button("Clear##mesh", ImVec2(52, 22))) {
      if (sel) {
        canvas.clearMeshForShape(sel);
        appContext.consoleMessage = "Mesh cleared for selected shape";
      } else {
        canvas.clearMesh();
        appContext.consoleMessage = "All meshes cleared";
      }
    }
    ImGui::PopStyleColor(2);
  }

  ImGui::SameLine(0, 8);
  bool meshVis = canvas.isMeshVisible();
  if (ImGui::Checkbox("Show##meshVis", &meshVis))
    canvas.setMeshVisible(meshVis);

  ImGui::SameLine(0, 8);
  ImGui::PushItemWidth(64);
  float elemSz = static_cast<float>(canvas.getMeshElementSize());
  if (ImGui::SliderFloat("##ElemSz", &elemSz, 1.0f, 50.0f, "E:%.0f"))
    canvas.setMeshElementSize(static_cast<double>(elemSz));
  ImGui::PopItemWidth();

  // --- Bottom edge line ---
  ImVec2 wPos = ImGui::GetWindowPos();
  ImVec2 wSz  = ImGui::GetWindowSize();
  ImGui::GetWindowDrawList()->AddLine(
    ImVec2(wPos.x, wPos.y + wSz.y - 1),
    ImVec2(wPos.x + wSz.x, wPos.y + wSz.y - 1),
    IM_COL32(30, 30, 40, 255), 1.0f);

  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar();
}

} // namespace UI
