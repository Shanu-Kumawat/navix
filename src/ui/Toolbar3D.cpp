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

// Toggle helper: clicking an ACTIVE tool icon returns to Select
static void activateTool(Modeling3D::Viewport3D& vp, Modeling3D::Tool3DType t) {
    vp.setActiveTool(vp.getActiveTool() == t
        ? Modeling3D::Tool3DType::Select
        : t);
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
            // Clicking the ACTIVE draw tool again stops drawing (sketch stays
            // open with Done/Cancel buttons)
            if (sketching && dt == newTool) {
                viewport.setSketchDrawTool(Modeling3D::SketchDrawTool::None);
                appContext.consoleMessage = "Drawing stopped";
                return;
            }
            if (!sketching) {
                viewport.beginSketchOnPendingPlane();  // chosen via the Plane selector
                viewport.setActiveTool(Modeling3D::Tool3DType::Sketch);
            }
            viewport.setSketchDrawTool(newTool);
        };

        // ── Sketch plane selector (ALWAYS visible — choose before drawing) ──
        {
            int planeIdx = viewport.getSketchPlaneIndex();
            const char* planeName = (planeIdx == 0) ? "Ground (XZ)"
                                  : (planeIdx == 1) ? "Front (XY)"
                                                    : "Side (YZ)";
            if (TIconBtn("plane3d", "⊡", "Sketch plane — pick the plane BEFORE drawing",
                         sketching, ImVec2(22, 30)))
                ImGui::OpenPopup("##PlanePickAlways");
            ImGui::SameLine(0, 2);
            ImGui::PushStyleColor(ImGuiCol_Text, UIColors::ACCENT);
            ImGui::TextUnformatted(planeName);
            ImGui::PopStyleColor();
            ImGui::SameLine(0, 6);
            if (ImGui::BeginPopup("##PlanePickAlways")) {
                ImGui::PushStyleColor(ImGuiCol_Text, UIColors::ACCENT);
                ImGui::TextUnformatted("Sketch plane:");
                ImGui::PopStyleColor();
                ImGui::Separator();
                struct PlaneItem { const char* label; int idx; };
                const PlaneItem planes[] = {
                    { "Ground (XZ) — draw flat on the floor", 0 },
                    { "Front  (XY) — vertical wall",          1 },
                    { "Side   (YZ) — vertical wall",          2 },
                };
                for (const auto& pl : planes) {
                    if (ImGui::MenuItem(pl.label, nullptr, planeIdx == pl.idx)) {
                        viewport.setSketchPlaneIndex(pl.idx);
                        appContext.consoleMessage = sketching
                            ? "Sketch restarted on the new plane"
                            : "Plane selected — now pick a drawing tool";
                    }
                }
                ImGui::EndPopup();
            }
            ImGui::SameLine(0, 6);
            TSep();
        }

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
            if (TIconBtn("plane3d", "⊡", "Change sketch plane", false, ImVec2(30, 30)))
                ImGui::OpenPopup("##PlanePicker3D");
            if (ImGui::BeginPopup("##PlanePicker3D")) {
                ImGui::TextDisabled("Sketch plane:");
                ImGui::Separator();
                if (ImGui::MenuItem("Ground (XZ)")) viewport.setSketchPlaneIndex(0);
                if (ImGui::MenuItem("Front (XY)"))  viewport.setSketchPlaneIndex(1);
                if (ImGui::MenuItem("Side (YZ)"))   viewport.setSketchPlaneIndex(2);
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
        activateTool(viewport, Modeling3D::Tool3DType::Extrude);
    ImGui::SameLine(0, 3);
    if (TIconBtn("revolve", "↻", "Revolve sketch", tool == Modeling3D::Tool3DType::Revolve, btnSz))
        activateTool(viewport, Modeling3D::Tool3DType::Revolve);
    ImGui::SameLine(0, 3);
    if (TIconBtn("fillet3d", "⌒", "Fillet / Chamfer edges", tool == Modeling3D::Tool3DType::Fillet, btnSz))
        activateTool(viewport, Modeling3D::Tool3DType::Fillet);
    ImGui::SameLine(0, 3);

    // ── Boolean ops flyout ────────────────────────────────────────────────────
    if (TIconBtn("boolean3d", "∪∩", "Boolean — Union / Subtract / Intersect",
                 tool == Modeling3D::Tool3DType::Boolean, btnSz))
        ImGui::OpenPopup("##BoolPopup3D");
    if (ImGui::BeginPopup("##BoolPopup3D")) {
        ImGui::PushStyleColor(ImGuiCol_Text, UIColors::ACCENT);
        ImGui::TextUnformatted("Boolean Operation:");
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::TextDisabled("Base body is modified, tool body is removed");
        ImGui::Spacing();
        struct BoolOpItem { const char* label; Modeling3D::BoolOp op; const char* hint; };
        const BoolOpItem ops[] = {
            { "Union (A + B)",       Modeling3D::BoolOp::Union,     "Merge two bodies into one" },
            { "Subtract (A - B)",    Modeling3D::BoolOp::Subtract,  "Cut the tool body out of the base body" },
            { "Intersect (A & B)",   Modeling3D::BoolOp::Intersect, "Keep only the overlapping volume" },
        };
        for (const auto& o : ops) {
            bool cur = (tool == Modeling3D::Tool3DType::Boolean &&
                        viewport.getBoolOpMode() == o.op);
            if (ImGui::MenuItem(o.label, nullptr, cur)) {
                viewport.beginBoolean(o.op);
                ImGui::CloseCurrentPopup();  // don't swallow the next viewport click
                appContext.consoleMessage = "Boolean: click the tool body in the viewport (base is pre-selected)";
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", o.hint);
        }
        ImGui::EndPopup();
    }
    ImGui::SameLine(0, 3);

    if (TIconBtn("movebody3d", "⇥", "Move Body — translate / face mate / rotate",
                 tool == Modeling3D::Tool3DType::MoveBody, btnSz))
        viewport.beginMoveBody();
    ImGui::SameLine(0, 3);

    // ── Cut-Extrude / Hole / Pattern / Mirror ─────────────────────────────────
    if (TIconBtn("cutextrude3d", "⊘", "Cut-Extrude — cut a sketch profile into a body",
                 tool == Modeling3D::Tool3DType::CutExtrude, btnSz))
        activateTool(viewport, Modeling3D::Tool3DType::CutExtrude);
    ImGui::SameLine(0, 3);
    if (TIconBtn("pattern3d", "⁘", "Pattern — linear / circular copies",
                 tool == Modeling3D::Tool3DType::Pattern, btnSz))
        activateTool(viewport, Modeling3D::Tool3DType::Pattern);
    ImGui::SameLine(0, 3);
    if (TIconBtn("mirror3d", "⋙", "Mirror — copy about a plane",
                 tool == Modeling3D::Tool3DType::Mirror, btnSz))
        activateTool(viewport, Modeling3D::Tool3DType::Mirror);
    ImGui::SameLine(0, 3);
    TSep();

    // ── Primitives flyout ─────────────────────────────────────────────────────
    {
        static float boxW=10,boxH=10,boxD=10, cylR=5,cylH=15, sphR=8;
        static float coneR1=6,coneR2=2,coneH=12, torMaj=10,torMin=3;

        if (TIconBtn("primitives", "□", "Insert primitive solid",
                     viewport.isInPlacementMode(), btnSz))
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
            static const char* cylAxNames[] = { "X axis", "Y axis (up)", "Z axis" };
            int cylAx = viewport.getCylinderAxis();
            ImGui::SetNextItemWidth(120);
            if (ImGui::Combo("Cyl axis##ca", &cylAx, cylAxNames, 3))
                viewport.setCylinderAxis(cylAx);
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
        activateTool(viewport, Modeling3D::Tool3DType::Move);
    ImGui::SameLine(0, 3);
    if (TIconBtn("rotate3d", "↻", "Rotate gizmo (E)", tool == Modeling3D::Tool3DType::Rotate, btnSz))
        activateTool(viewport, Modeling3D::Tool3DType::Rotate);
    ImGui::SameLine(0, 3);
    if (TIconBtn("scale3d",  "⤢", "Scale gizmo (R)",  tool == Modeling3D::Tool3DType::Scale,  btnSz))
        activateTool(viewport, Modeling3D::Tool3DType::Scale);
    ImGui::SameLine(0, 3);
    if (TIconBtn("pan3d", "✥", "Pan — left-drag to move the workspace",
                 tool == Modeling3D::Tool3DType::Pan, btnSz))
        activateTool(viewport, Modeling3D::Tool3DType::Pan);
    ImGui::SameLine(0, 3);
    TSep();

    // ── Undo / Redo ───────────────────────────────────────────────────────────
    {
        auto& cmd = viewport.getScene()->getCommandManager();
        // Evaluate availability ONCE: undoing the last command empties the
        // history mid-button, and a re-evaluated canUndo() would run an
        // unpaired EndDisabled → ImGui assertion crash
        bool canU = cmd.canUndo();
        if (!canU) ImGui::BeginDisabled();
        if (TIconBtn("undo", "↶", "Undo (Ctrl+Z)", false, btnSz)) {
            viewport.getScene()->undo();
            viewport.onHistoryChanged();  // drop stale refs/previews
        }
        if (!canU) ImGui::EndDisabled();
        ImGui::SameLine(0, 3);
        bool canR = cmd.canRedo();
        if (!canR) ImGui::BeginDisabled();
        if (TIconBtn("redo", "↷", "Redo (Ctrl+Y)", false, btnSz)) {
            viewport.getScene()->redo();
            viewport.onHistoryChanged();
        }
        if (!canR) ImGui::EndDisabled();
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
