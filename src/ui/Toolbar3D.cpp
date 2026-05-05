#include "ui/Toolbar3D.hpp"
#include "ui/UIColors.hpp"
#include "ApplicationContext.hpp"
#include <imgui.h>
#include <imgui_internal.h>

namespace UI {

static void TSep() {
    ImGui::SameLine(0, 6);
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddLine(
        ImVec2(p.x, p.y + 3), ImVec2(p.x, p.y + 29),
        IM_COL32(55, 58, 68, 200), 1.0f);
    ImGui::Dummy(ImVec2(1, 32));
    ImGui::SameLine(0, 6);
}

static bool TBtn(const char* label, const char* tip, bool active,
                 ImVec2 sz = ImVec2(0, 32)) {
    if (active) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.42f, 0.62f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.50f, 0.72f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.17f, 0.21f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.24f, 0.30f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.72f, 0.75f, 0.82f, 1.0f));
    }
    bool clicked = ImGui::Button(label, sz);
    ImGui::PopStyleColor(3);
    if (ImGui::IsItemHovered() && tip[0]) ImGui::SetTooltip("%s", tip);
    return clicked;
}

// Small view button (for Fit/Top/Front/Right/Iso)
static bool ViewBtn(const char* label, const char* tip) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.15f, 0.19f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.25f, 0.35f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.62f, 0.75f, 1.0f));
    bool r = ImGui::Button(label, ImVec2(0, 28));
    ImGui::PopStyleColor(3);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tip);
    return r;
}

void Toolbar3D::Render(Modeling3D::Viewport3D& viewport, Core::ApplicationContext& appContext) {
    auto tool = viewport.getActiveTool();
    bool sketching = viewport.getScene()->isSketchActive();

    const float toolbarHeight = 48.0f;
    const float displayWidth = ImGui::GetIO().DisplaySize.x;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(displayWidth, toolbarHeight));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 7));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.11f, 0.12f, 0.14f, 1.0f));

    ImGui::Begin("##Toolbar3D", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // ── Brand + Mode Switch ──
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.65f, 0.95f, 1.0f));
    ImGui::Text("NAVIX");
    ImGui::PopStyleColor();
    ImGui::SameLine(0, 6);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.45f, 0.18f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.25f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.7f, 0.6f, 1.0f));
    if (ImGui::Button("<2D", ImVec2(0, 32))) {
        appContext.currentWorkspaceMode = Core::WorkspaceMode::Mode2D;
    }
    ImGui::PopStyleColor(3);
    ImGui::SameLine(0, 4);
    TSep();

    // ── Select ──
    if (TBtn("Select", "Select (Esc)", tool == Modeling3D::Tool3DType::Select))
        viewport.setActiveTool(Modeling3D::Tool3DType::Select);
    ImGui::SameLine(0, 3);
    TSep();

    // ── Draw Tools (always visible — auto-start XY sketch if none active) ──
    {
        auto dt = viewport.getSketchDrawTool();

        auto startSketchIfNeeded = [&](Modeling3D::SketchDrawTool newTool) {
            if (!sketching) {
                viewport.getScene()->beginSketch(Modeling3D::WorkPlane3D::XY());
                viewport.setActiveTool(Modeling3D::Tool3DType::Sketch);
            }
            viewport.setSketchDrawTool(newTool);
        };

        // ── Draw tool buttons ──
        if (TBtn("Line",   "Line — click to set start, click to end\n[Chain: auto-continues from last point]",
                 dt == Modeling3D::SketchDrawTool::Line, ImVec2(0, 32)))
            startSketchIfNeeded(Modeling3D::SketchDrawTool::Line);
        ImGui::SameLine(0, 2);
        if (TBtn("Rect",   "Rectangle — click corner to corner",
                 dt == Modeling3D::SketchDrawTool::Rectangle, ImVec2(0, 32)))
            startSketchIfNeeded(Modeling3D::SketchDrawTool::Rectangle);
        ImGui::SameLine(0, 2);
        if (TBtn("Circle", "Circle — click center then drag to radius",
                 dt == Modeling3D::SketchDrawTool::Circle, ImVec2(0, 32)))
            startSketchIfNeeded(Modeling3D::SketchDrawTool::Circle);
        ImGui::SameLine(0, 2);
        if (TBtn("Spline", "Spline / Polyline — click points, RMB to finish segment",
                 dt == Modeling3D::SketchDrawTool::Spline, ImVec2(0, 32)))
            startSketchIfNeeded(Modeling3D::SketchDrawTool::Spline);

        if (sketching) {
            // Chain mode toggle (only meaningful for Line/Spline)
            if (dt == Modeling3D::SketchDrawTool::Line ||
                dt == Modeling3D::SketchDrawTool::Spline) {
                ImGui::SameLine(0, 4);
                bool chain = viewport.isSketchChainMode();
                if (TBtn("~", "Chain mode — continue from last endpoint", chain, ImVec2(24, 32)))
                    viewport.setSketchChainMode(!chain);
            }

            ImGui::SameLine(0, 6);
            // Plane picker
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.22f, 0.30f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.28f, 0.38f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.70f, 0.90f, 1.0f));
            if (ImGui::Button("Plane", ImVec2(0, 32)))
                ImGui::OpenPopup("##PlanePicker");
            ImGui::PopStyleColor(3);
            if (ImGui::BeginPopup("##PlanePicker")) {
                ImGui::TextDisabled("Change sketch plane:");
                ImGui::Separator();
                if (ImGui::MenuItem("XY  (top — horizontal)")) {
                    viewport.getScene()->finishSketch();
                    viewport.getScene()->beginSketch(Modeling3D::WorkPlane3D::XY());
                }
                if (ImGui::MenuItem("XZ  (front / back)")) {
                    viewport.getScene()->finishSketch();
                    viewport.getScene()->beginSketch(Modeling3D::WorkPlane3D::XZ());
                }
                if (ImGui::MenuItem("YZ  (left / right)")) {
                    viewport.getScene()->finishSketch();
                    viewport.getScene()->beginSketch(Modeling3D::WorkPlane3D::YZ());
                }
                ImGui::EndPopup();
            }

            ImGui::SameLine(0, 4);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.45f, 0.22f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.55f, 0.28f, 1.0f));
            if (ImGui::Button("Done", ImVec2(0, 32))) {
                viewport.setSketchDrawTool(Modeling3D::SketchDrawTool::None);
                viewport.getScene()->finishSketch();
                viewport.setActiveTool(Modeling3D::Tool3DType::Select);
            }
            ImGui::PopStyleColor(2);
            ImGui::SameLine(0, 2);

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.40f, 0.13f, 0.13f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.52f, 0.18f, 0.18f, 1.0f));
            if (ImGui::Button("X", ImVec2(28, 32))) {
                viewport.setSketchDrawTool(Modeling3D::SketchDrawTool::None);
                viewport.getScene()->cancelSketch();
                viewport.setActiveTool(Modeling3D::Tool3DType::Select);
            }
            ImGui::PopStyleColor(2);
        }
    }
    ImGui::SameLine(0, 3);
    TSep();

    // ── Operations ──
    if (TBtn("Ext", "Extrude sketch", tool == Modeling3D::Tool3DType::Extrude))
        viewport.setActiveTool(Modeling3D::Tool3DType::Extrude);
    ImGui::SameLine(0, 2);
    if (TBtn("Rev", "Revolve sketch", tool == Modeling3D::Tool3DType::Revolve))
        viewport.setActiveTool(Modeling3D::Tool3DType::Revolve);
    ImGui::SameLine(0, 2);
    if (TBtn("Fil", "Fillet edges", tool == Modeling3D::Tool3DType::Fillet))
        viewport.setActiveTool(Modeling3D::Tool3DType::Fillet);
    ImGui::SameLine(0, 3);
    TSep();

    // ── Primitives (flyout popup) ──
    {
        static float boxW=10,boxH=10,boxD=10, cylR=5,cylH=15, sphR=8;
        static float coneR1=6,coneR2=2,coneH=12, torMaj=10,torMin=3;

        if (TBtn("Prims", "Insert primitive solid", false)) {
            ImGui::OpenPopup("##PrimPopup");
        }
        if (ImGui::BeginPopup("##PrimPopup")) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.65f, 0.72f, 0.85f, 1.0f));
            ImGui::Text("Create Primitive:");
            ImGui::PopStyleColor();
            ImGui::Separator();

            // Box
            ImGui::DragFloat3("Box##d", &boxW, 0.5f, 0.5f, 200.f, "%.1f");
            ImGui::SameLine();
            if (ImGui::SmallButton("Add##Box")) {
                viewport.createBox(boxW, boxH, boxD);
                appContext.consoleMessage = "Created Box";
            }
            // Cylinder
            ImGui::DragFloat("CylR", &cylR, 0.5f, 0.5f, 100.f, "R:%.1f");
            ImGui::SameLine();
            ImGui::DragFloat("CylH", &cylH, 0.5f, 0.5f, 200.f, "H:%.1f");
            ImGui::SameLine();
            if (ImGui::SmallButton("Add##Cyl")) {
                viewport.createCylinder(cylR, cylH);
                appContext.consoleMessage = "Created Cylinder";
            }
            // Sphere
            ImGui::DragFloat("SphR", &sphR, 0.5f, 0.5f, 100.f, "R:%.1f");
            ImGui::SameLine();
            if (ImGui::SmallButton("Add##Sph")) {
                viewport.createSphere(sphR);
                appContext.consoleMessage = "Created Sphere";
            }
            // Cone
            ImGui::DragFloat("R1##Cn", &coneR1, 0.5f, 0.5f, 100.f, "R1:%.1f");
            ImGui::SameLine();
            ImGui::DragFloat("R2##Cn", &coneR2, 0.5f, 0.0f, 100.f, "R2:%.1f");
            ImGui::SameLine();
            ImGui::DragFloat("H##Cn", &coneH, 0.5f, 0.5f, 200.f, "H:%.1f");
            ImGui::SameLine();
            if (ImGui::SmallButton("Add##Cone")) {
                viewport.createCone(coneR1, coneR2, coneH);
                appContext.consoleMessage = "Created Cone";
            }
            // Torus
            ImGui::DragFloat("Maj##Tr", &torMaj, 0.5f, 1.f, 100.f, "Maj:%.1f");
            ImGui::SameLine();
            ImGui::DragFloat("Min##Tr", &torMin, 0.5f, 0.5f, 50.f, "Min:%.1f");
            ImGui::SameLine();
            if (ImGui::SmallButton("Add##Tor")) {
                viewport.createTorus(torMaj, torMin);
                appContext.consoleMessage = "Created Torus";
            }

            ImGui::EndPopup();
        }
    }
    ImGui::SameLine(0, 3);
    TSep();

    // ── Transform ──
    if (TBtn("Mov", "Move (W)", tool == Modeling3D::Tool3DType::Move))
        viewport.setActiveTool(Modeling3D::Tool3DType::Move);
    ImGui::SameLine(0, 2);
    if (TBtn("Rot", "Rotate (E)", tool == Modeling3D::Tool3DType::Rotate))
        viewport.setActiveTool(Modeling3D::Tool3DType::Rotate);
    ImGui::SameLine(0, 2);
    if (TBtn("Scl", "Scale (R)", tool == Modeling3D::Tool3DType::Scale))
        viewport.setActiveTool(Modeling3D::Tool3DType::Scale);
    ImGui::SameLine(0, 3);
    TSep();

    // ── Undo/Redo ──
    {
        auto& cmd = viewport.getScene()->getCommandManager();
        if (!cmd.canUndo()) ImGui::BeginDisabled();
        if (TBtn("Un", "Undo (Ctrl+Z)", false, ImVec2(32, 32)))
            viewport.getScene()->undo();
        if (!cmd.canUndo()) ImGui::EndDisabled();
        ImGui::SameLine(0, 2);
        if (!cmd.canRedo()) ImGui::BeginDisabled();
        if (TBtn("Re", "Redo (Ctrl+Y)", false, ImVec2(32, 32)))
            viewport.getScene()->redo();
        if (!cmd.canRedo()) ImGui::EndDisabled();
    }
    ImGui::SameLine(0, 3);
    TSep();

    // ── Views ──
    if (ViewBtn("Fit", "Zoom to fit all")) viewport.zoomToFit();
    ImGui::SameLine(0, 2);
    if (ViewBtn("Top", "Top view")) viewport.setStandardView(Modeling3D::StandardView::Top);
    ImGui::SameLine(0, 2);
    if (ViewBtn("Frt", "Front view")) viewport.setStandardView(Modeling3D::StandardView::Front);
    ImGui::SameLine(0, 2);
    if (ViewBtn("Rht", "Right view")) viewport.setStandardView(Modeling3D::StandardView::Right);
    ImGui::SameLine(0, 2);
    if (ViewBtn("Iso", "Isometric view")) viewport.setStandardView(Modeling3D::StandardView::Isometric);

    // ── Render mode (far right) ──
    {
        float rightX = displayWidth - 140.0f;
        ImGui::SameLine(rightX);
        auto rm = viewport.getRenderMode();
        const char* labels[] = {"Solid", "Edges", "Wire"};
        int idx = static_cast<int>(rm);
        ImGui::PushItemWidth(90);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.14f, 0.15f, 0.19f, 1.0f));
        if (ImGui::Combo("##RM", &idx, labels, 3))
            viewport.setRenderMode(static_cast<Modeling3D::RenderMode3D>(idx));
        ImGui::PopStyleColor();
        ImGui::PopItemWidth();
    }

    // Bottom edge
    ImVec2 wPos = ImGui::GetWindowPos();
    ImVec2 wSz = ImGui::GetWindowSize();
    ImGui::GetWindowDrawList()->AddLine(
        ImVec2(wPos.x, wPos.y + wSz.y - 1),
        ImVec2(wPos.x + wSz.x, wPos.y + wSz.y - 1),
        IM_COL32(35, 38, 48, 255), 1.0f);

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

} // namespace UI
