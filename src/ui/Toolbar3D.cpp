#include "ui/Toolbar3D.hpp"
#include "ui/UIColors.hpp"
#include "ui/UIHelpers.hpp"
#include "ApplicationContext.hpp"
#include <imgui.h>
#include <imgui_internal.h>

namespace UI {

// ── Separator ─────────────────────────────────────────────────────────────────
static void TSep() {
    ImGui::SameLine(0, 8);
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddLine(
        ImVec2(p.x, p.y + 3), ImVec2(p.x, p.y + 26),
        IM_COL32(55, 58, 68, 180), 1.0f);
    ImGui::Dummy(ImVec2(1, 30));
    ImGui::SameLine(0, 8);
}

// ── Toolbar icon button (wraps shared IconButton, matches 2D style) ───────────
//    sz = button size (width=0 means auto)
static bool TIconBtn(const char* iconName, const char* fallback, const char* tip,
                     bool active, ImVec2 sz = ImVec2(30, 30)) {
    return IconButton(iconName, fallback, tip, sz, active);
}

// ── Small view button ─────────────────────────────────────────────────────────
static bool ViewBtn(const char* label, const char* tip) {
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.40f, 0.41f, 0.48f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.48f, 0.49f, 0.56f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text,          UIColors::TEXT_BRIGHT);
    bool r = ImGui::Button(label, ImVec2(0, 28));
    ImGui::PopStyleColor(3);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tip);
    return r;
}

// ─────────────────────────────────────────────────────────────────────────────
void Toolbar3D::Render(Modeling3D::Viewport3D& viewport, Core::ApplicationContext& appContext) {
    auto  tool     = viewport.getActiveTool();
    bool  sketching = viewport.getScene()->isSketchActive();
    auto  dt       = viewport.getSketchDrawTool();

    const float toolbarHeight = 48.0f;
    const float displayWidth  = ImGui::GetIO().DisplaySize.x;
    const ImVec2 btnSz(30, 30);

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(displayWidth, toolbarHeight));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 9));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.11f, 0.12f, 0.14f, 1.0f));  // dark 3D theme

    ImGui::Begin("##Toolbar3D", nullptr,
        ImGuiWindowFlags_NoResize    | ImGuiWindowFlags_NoMove    |
        ImGuiWindowFlags_NoCollapse  | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // ── Brand ─────────────────────────────────────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_Text, UIColors::ACCENT);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);
    ImGui::Text("NAVIX");
    ImGui::PopStyleColor();
    ImGui::SameLine(0, 6);

    // ── Back to 2D ────────────────────────────────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.18f, 0.42f, 0.55f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.52f, 0.65f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text,          UIColors::TEXT_BRIGHT);
    if (ImGui::Button("<2D", ImVec2(0, 30))) {
        appContext.currentWorkspaceMode = Core::WorkspaceMode::Mode2D;
        appContext.consoleMessage = "Switched to 2D Mode";
    }
    ImGui::PopStyleColor(3);
    ImGui::SameLine(0, 4);
    TSep();

    // ── Select ────────────────────────────────────────────────────────────────
    if (TIconBtn("select", "→", "Select (Esc)", tool == Modeling3D::Tool3DType::Select, btnSz))
        viewport.setActiveTool(Modeling3D::Tool3DType::Select);
    ImGui::SameLine(0, 3);
    TSep();

    // ── Sketch Draw Tools ─────────────────────────────────────────────────────
    //   Clicking any draw tool auto-starts a sketch on XY if none is active.
    //   No mandatory plane-selection dialog — just start drawing immediately.
    {
        auto startSketch = [&](Modeling3D::SketchDrawTool newTool) {
            if (!sketching) {
                viewport.getScene()->beginSketch(Modeling3D::WorkPlane3D::XY());
                viewport.setActiveTool(Modeling3D::Tool3DType::Sketch);
            }
            viewport.setSketchDrawTool(newTool);
        };

        bool lineSel   = sketching && dt == Modeling3D::SketchDrawTool::Line;
        bool rectSel   = sketching && dt == Modeling3D::SketchDrawTool::Rectangle;
        bool circleSel = sketching && dt == Modeling3D::SketchDrawTool::Circle;
        bool splineSel = sketching && dt == Modeling3D::SketchDrawTool::Spline;

        if (TIconBtn("line", "-", "Line — click start, click end\n(Chain: continues from last point)", lineSel, btnSz))
            startSketch(Modeling3D::SketchDrawTool::Line);
        ImGui::SameLine(0, 3);

        if (TIconBtn("rectangle", "[]", "Rectangle — click corner to corner", rectSel, btnSz))
            startSketch(Modeling3D::SketchDrawTool::Rectangle);
        ImGui::SameLine(0, 3);

        if (TIconBtn("circle", "O", "Circle — click center, drag to radius", circleSel, btnSz))
            startSketch(Modeling3D::SketchDrawTool::Circle);
        ImGui::SameLine(0, 3);

        if (TIconBtn("spline3d", "~", "Spline / Polyline — click points, RMB to stop chain", splineSel, btnSz))
            startSketch(Modeling3D::SketchDrawTool::Spline);
        ImGui::SameLine(0, 4);

        // Chain mode toggle (only relevant for Line / Spline)
        if (sketching && (dt == Modeling3D::SketchDrawTool::Line ||
                          dt == Modeling3D::SketchDrawTool::Spline)) {
            bool chain = viewport.isSketchChainMode();
            if (TIconBtn("spline", "⛓", chain ? "Chain ON — continue from last point"
                                               : "Chain OFF — each segment is independent",
                         chain, ImVec2(22, 30)))
                viewport.setSketchChainMode(!chain);
            ImGui::SameLine(0, 4);
        }

        // Plane switcher — only shown while sketching, does NOT restart the sketch
        if (sketching) {
            if (TIconBtn("plane3d", "⊡", "Change sketch plane (XY / XZ / YZ)", false, ImVec2(30, 30)))
                ImGui::OpenPopup("##PlanePicker3D");
            if (ImGui::BeginPopup("##PlanePicker3D")) {
                ImGui::TextDisabled("Sketch plane:");
                ImGui::Separator();
                if (ImGui::MenuItem("XY  — top (default)")) {
                    // Switch plane without losing existing sketch:
                    // finish current, immediately begin new on target plane
                    viewport.getScene()->finishSketch();
                    viewport.getScene()->beginSketch(Modeling3D::WorkPlane3D::XY());
                }
                if (ImGui::MenuItem("XZ  — front / back")) {
                    viewport.getScene()->finishSketch();
                    viewport.getScene()->beginSketch(Modeling3D::WorkPlane3D::XZ());
                }
                if (ImGui::MenuItem("YZ  — left / right")) {
                    viewport.getScene()->finishSketch();
                    viewport.getScene()->beginSketch(Modeling3D::WorkPlane3D::YZ());
                }
                ImGui::EndPopup();
            }
            ImGui::SameLine(0, 4);

            // Done — also finalizes any in-progress spline
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.15f, 0.42f, 0.22f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.52f, 0.28f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text,          UIColors::TEXT_BRIGHT);
            if (ImGui::Button("Done", ImVec2(0, 30))) {
                viewport.commitSpline();  // finalize spline if any (no-op otherwise)
                viewport.setSketchDrawTool(Modeling3D::SketchDrawTool::None);
                viewport.getScene()->finishSketch();
                viewport.setActiveTool(Modeling3D::Tool3DType::Select);
                viewport.setSelectedSketch(nullptr);
                appContext.consoleMessage = "Sketch finished";
            }
            ImGui::PopStyleColor(3);
            ImGui::SameLine(0, 2);

            // Cancel
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.40f, 0.13f, 0.13f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.52f, 0.18f, 0.18f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text,          UIColors::TEXT_BRIGHT);
            if (ImGui::Button("✕##CancelSk", ImVec2(26, 30))) {
                viewport.setSketchDrawTool(Modeling3D::SketchDrawTool::None);
                viewport.getScene()->cancelSketch();
                viewport.setActiveTool(Modeling3D::Tool3DType::Select);
                appContext.consoleMessage = "Sketch cancelled";
            }
            ImGui::PopStyleColor(3);
            ImGui::SameLine(0, 3);
        }
    }
    TSep();

    // ── Sketch Operations ─────────────────────────────────────────────────────
    if (TIconBtn("extrude", "↑", "Extrude sketch", tool == Modeling3D::Tool3DType::Extrude, btnSz))
        viewport.setActiveTool(Modeling3D::Tool3DType::Extrude);
    ImGui::SameLine(0, 3);
    if (TIconBtn("revolve", "↻", "Revolve sketch", tool == Modeling3D::Tool3DType::Revolve, btnSz))
        viewport.setActiveTool(Modeling3D::Tool3DType::Revolve);
    ImGui::SameLine(0, 3);
    if (TIconBtn("fillet3d", "⌒", "Fillet / Chamfer edges", tool == Modeling3D::Tool3DType::Fillet, btnSz))
        viewport.setActiveTool(Modeling3D::Tool3DType::Fillet);
    ImGui::SameLine(0, 3);
    TSep();

    // ── Primitives flyout ─────────────────────────────────────────────────────
    {
        static float boxW=10,boxH=10,boxD=10, cylR=5,cylH=15, sphR=8;
        static float coneR1=6,coneR2=2,coneH=12, torMaj=10,torMin=3;

        if (TIconBtn("primitives", "□", "Insert primitive solid", false, btnSz))
            ImGui::OpenPopup("##PrimPopup3D");

        if (ImGui::BeginPopup("##PrimPopup3D")) {
            ImGui::PushStyleColor(ImGuiCol_Text, UIColors::ACCENT);
            ImGui::TextUnformatted("Create Primitive:");
            ImGui::PopStyleColor();
            ImGui::Separator();

            ImGui::TextDisabled("Click 'Place' then click on canvas to position");
            ImGui::Spacing();

            // Box
            ImGui::SetNextItemWidth(120);
            ImGui::DragFloat3("Box (W/H/D)##pb", &boxW, 0.5f, 0.5f, 500.f, "%.1f");
            ImGui::SameLine();
            if (ImGui::SmallButton("Place##Box")) {
                viewport.startPlaceBox(boxW, boxH, boxD);
                appContext.consoleMessage = "Click on canvas to place Box";
                ImGui::CloseCurrentPopup();
            }
            // Cylinder
            ImGui::SetNextItemWidth(80);
            ImGui::DragFloat("Cyl R##pcr", &cylR, 0.5f, 0.5f, 200.f, "R:%.1f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80);
            ImGui::DragFloat("H##pch", &cylH, 0.5f, 0.5f, 500.f, "H:%.1f");
            ImGui::SameLine();
            if (ImGui::SmallButton("Place##Cyl")) {
                viewport.startPlaceCylinder(cylR, cylH);
                appContext.consoleMessage = "Click on canvas to place Cylinder";
                ImGui::CloseCurrentPopup();
            }
            // Sphere
            ImGui::SetNextItemWidth(80);
            ImGui::DragFloat("Sphere R##psr", &sphR, 0.5f, 0.5f, 200.f, "R:%.1f");
            ImGui::SameLine();
            if (ImGui::SmallButton("Place##Sph")) {
                viewport.startPlaceSphere(sphR);
                appContext.consoleMessage = "Click on canvas to place Sphere";
                ImGui::CloseCurrentPopup();
            }
            // Cone
            ImGui::SetNextItemWidth(70);
            ImGui::DragFloat("R1##pcn1", &coneR1, 0.5f, 0.5f, 200.f, "R1:%.1f"); ImGui::SameLine();
            ImGui::SetNextItemWidth(70);
            ImGui::DragFloat("R2##pcn2", &coneR2, 0.5f, 0.0f, 200.f, "R2:%.1f"); ImGui::SameLine();
            ImGui::SetNextItemWidth(70);
            ImGui::DragFloat("H##pcnh", &coneH, 0.5f, 0.5f, 500.f, "H:%.1f"); ImGui::SameLine();
            if (ImGui::SmallButton("Place##Cone")) {
                viewport.startPlaceCone(coneR1, coneR2, coneH);
                appContext.consoleMessage = "Click on canvas to place Cone";
                ImGui::CloseCurrentPopup();
            }
            // Torus
            ImGui::SetNextItemWidth(80);
            ImGui::DragFloat("Maj##ptr", &torMaj, 0.5f, 1.f, 200.f, "Maj:%.1f"); ImGui::SameLine();
            ImGui::SetNextItemWidth(80);
            ImGui::DragFloat("Min##ptm", &torMin, 0.5f, 0.5f, 100.f, "Min:%.1f"); ImGui::SameLine();
            if (ImGui::SmallButton("Place##Tor")) {
                viewport.startPlaceTorus(torMaj, torMin);
                appContext.consoleMessage = "Click on canvas to place Torus";
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
    ImGui::SameLine(0, 3);
    TSep();

    // ── Transform ─────────────────────────────────────────────────────────────
    if (TIconBtn("move3d",   "✦", "Move gizmo (W)",   tool == Modeling3D::Tool3DType::Move,   btnSz))
        viewport.setActiveTool(Modeling3D::Tool3DType::Move);
    ImGui::SameLine(0, 3);
    if (TIconBtn("rotate3d", "↻", "Rotate gizmo (E)", tool == Modeling3D::Tool3DType::Rotate, btnSz))
        viewport.setActiveTool(Modeling3D::Tool3DType::Rotate);
    ImGui::SameLine(0, 3);
    if (TIconBtn("scale3d",  "⤢", "Scale gizmo (R)",  tool == Modeling3D::Tool3DType::Scale,  btnSz))
        viewport.setActiveTool(Modeling3D::Tool3DType::Scale);
    ImGui::SameLine(0, 3);
    TSep();

    // ── Undo / Redo ───────────────────────────────────────────────────────────
    {
        auto& cmd = viewport.getScene()->getCommandManager();
        if (!cmd.canUndo()) ImGui::BeginDisabled();
        if (TIconBtn("undo", "↶", "Undo (Ctrl+Z)", false, btnSz)) viewport.getScene()->undo();
        if (!cmd.canUndo()) ImGui::EndDisabled();
        ImGui::SameLine(0, 3);
        if (!cmd.canRedo()) ImGui::BeginDisabled();
        if (TIconBtn("redo", "↷", "Redo (Ctrl+Y)", false, btnSz)) viewport.getScene()->redo();
        if (!cmd.canRedo()) ImGui::EndDisabled();
    }
    ImGui::SameLine(0, 3);
    TSep();

    // ── Clear All ─────────────────────────────────────────────────────────────
    {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.45f, 0.12f, 0.12f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.65f, 0.18f, 0.18f, 1.0f));
        if (ImGui::Button("Clear All", ImVec2(0, 30))) {
            viewport.getScene()->clearAll();
            viewport.setSelectedSketch(nullptr);
            viewport.setActiveTool(Modeling3D::Tool3DType::Select);
            appContext.consoleMessage = "Scene cleared";
        }
        ImGui::PopStyleColor(2);
    }
    ImGui::SameLine(0, 3);
    TSep();

    // ── View shortcuts ────────────────────────────────────────────────────────
    if (ViewBtn("Fit",  "Zoom to fit all"))       viewport.zoomToFit();
    ImGui::SameLine(0, 2);
    if (ViewBtn("Top",  "Top view (XY)"))         viewport.setStandardView(Modeling3D::StandardView::Top);
    ImGui::SameLine(0, 2);
    if (ViewBtn("Frt",  "Front view (XZ)"))       viewport.setStandardView(Modeling3D::StandardView::Front);
    ImGui::SameLine(0, 2);
    if (ViewBtn("Rht",  "Right view (YZ)"))       viewport.setStandardView(Modeling3D::StandardView::Right);
    ImGui::SameLine(0, 2);
    if (ViewBtn("Iso",  "Isometric view"))         viewport.setStandardView(Modeling3D::StandardView::Isometric);

    // ── Render mode (pushed right) ────────────────────────────────────────────
    {
        float rightX = displayWidth - 140.0f;
        ImGui::SameLine(rightX);
        auto rm = viewport.getRenderMode();
        const char* labels[] = {"Solid", "Edges", "Wire"};
        int idx = static_cast<int>(rm);
        ImGui::PushItemWidth(90);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, UIColors::DARK_PANEL);
        if (ImGui::Combo("##RM3D", &idx, labels, 3))
            viewport.setRenderMode(static_cast<Modeling3D::RenderMode3D>(idx));
        ImGui::PopStyleColor();
        ImGui::PopItemWidth();
    }

    // Bottom hairline (same as 2D toolbar)
    {
        ImVec2 wPos = ImGui::GetWindowPos();
        ImVec2 wSz  = ImGui::GetWindowSize();
        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(wPos.x,          wPos.y + wSz.y - 1),
            ImVec2(wPos.x + wSz.x, wPos.y + wSz.y - 1),
            IM_COL32(35, 38, 48, 255), 1.5f);
    }

    ImGui::End();
    ImGui::PopStyleColor();  // WindowBg
    ImGui::PopStyleVar();    // WindowPadding
}

} // namespace UI
