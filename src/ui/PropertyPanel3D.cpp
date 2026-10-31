#include "ui/PropertyPanel3D.hpp"
#include "ui/UIColors.hpp"
#include "ui/ExportUI.hpp"
#include "ApplicationContext.hpp"
#include "modeling3d/Exporter3D.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

using Modeling3D::Body3D;

namespace UI {

void PropertyPanel3D::Render(Modeling3D::Viewport3D& viewport, Core::ApplicationContext& appContext) {
    const float ribbonHeight = 48.0f;
    const float statusBarHeight = 28.0f;
    const float featureTreeWidth = 220.0f;
    float panelWidth = appContext.userPropertyPanelWidth;
    float panelX = ImGui::GetIO().DisplaySize.x - panelWidth;
    float panelHeight = ImGui::GetIO().DisplaySize.y - ribbonHeight - statusBarHeight;

    ImGui::SetNextWindowPos(ImVec2(panelX, ribbonHeight));
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.13f, 0.14f, 0.16f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));

    ImGui::Begin("##Properties3D", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

    // Title
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.70f, 0.90f, 1.0f));
    ImGui::TextUnformatted("PROPERTIES");
    ImGui::PopStyleColor();
    ImGui::Separator();

    auto tool = viewport.getActiveTool();
    Modeling3D::Body3D* selected = viewport.getScene()->getSelectedBody();

    // ── Sketch Cursor + Snap (when sketching) ──
    if (viewport.getScene()->isSketchActive()) {
        glm::dvec2 cursor = viewport.getSketchCursor();

        // Snap indicator
        bool snapOn = viewport.isSnapEnabled();
        if (snapOn && viewport.isSnapActive()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.25f, 0.90f, 0.45f, 1.0f));
            ImGui::Text("⬤ SNAP  (%.1f, %.1f)", cursor.x, cursor.y);
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.85f, 1.0f, 1.0f));
            ImGui::Text("Cursor: (%.1f, %.1f)", cursor.x, cursor.y);
            ImGui::PopStyleColor();
        }

        if (viewport.isSketchDrawing()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.2f, 1.0f));
            ImGui::Text("Drawing — click to place");
            ImGui::PopStyleColor();
        }

        auto* sketch = viewport.getScene()->getActiveSketch();
        if (sketch)
            ImGui::Text("Shapes: %zu", sketch->getShapes().size());

        // Snap controls
        ImGui::Spacing();
        if (ImGui::Checkbox("Snap to Grid", &snapOn))
            viewport.setSnapEnabled(snapOn);
        if (snapOn) {
            float gs = viewport.getSnapGridSize();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##SnapSz", &gs, 0.25f, 0.25f, 50.f, "Grid: %.2f"))
                viewport.setSnapGridSize(gs);
            ImGui::PopItemWidth();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f,0.50f,0.58f,1.f));
            ImGui::TextUnformatted("Hold Ctrl to disable snap");
            ImGui::PopStyleColor();
        }
        ImGui::Separator();
    }

    // ── Selected Sketch Controls ──────────────────────────────────────────────
    {
        auto* selSk = viewport.getSelectedSketch();
        if (selSk && !viewport.getScene()->isSketchActive()) {
            if (ImGui::CollapsingHeader("Selected Sketch", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.88f, 0.15f, 1.0f));
                ImGui::Text("Shapes: %zu", selSk->getShapes().size());
                ImGui::PopStyleColor();

                // Move sketch (translate all 2D points)
                ImGui::TextDisabled("Move (dX, dZ, dY):");
                static float skMoveX{0}, skMoveY{0}, skMoveZ{0};
                ImGui::PushItemWidth(-1);
                ImGui::DragFloat3("##SkMove", &skMoveX, 0.5f, -999.f, 999.f, "%.1f");
                ImGui::PopItemWidth();
                if (ImGui::Button("Apply Move##sk", ImVec2(-1, 24))) {
                    // Translate all shapes in the sketch by (skMoveX, skMoveY) in sketch-plane coords
                    glm::dvec2 delta(skMoveX, skMoveZ);
                    for (auto& shape : selSk->getShapesMutable()) {
                        if (shape->type == Drawing::ShapeType::LINE) {
                            auto* ln = static_cast<Drawing::Line*>(shape.get());
                            ln->start += delta; ln->end += delta;
                        } else if (shape->type == Drawing::ShapeType::CIRCLE) {
                            auto* c = static_cast<Drawing::Circle*>(shape.get());
                            c->center += delta;
                        } else if (shape->type == Drawing::ShapeType::RECTANGLE) {
                            auto* r = static_cast<Drawing::Rectangle*>(shape.get());
                            r->topLeft+=delta; r->topRight+=delta;
                            r->bottomLeft+=delta; r->bottomRight+=delta;
                        }
                    }
                    viewport.markSketchDirty();
                    skMoveX = skMoveY = skMoveZ = 0;
                }

                ImGui::Spacing();
                // Delete selected sketch
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.45f, 0.12f, 0.12f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.58f, 0.18f, 0.18f, 1.0f));
                if (ImGui::Button("Delete Sketch", ImVec2(-1, 26))) {
                    viewport.getScene()->removeSketch(selSk);
                    viewport.setSelectedSketch(nullptr);
                }
                ImGui::PopStyleColor(2);
                ImGui::SameLine();
                if (ImGui::Button("Deselect##sk", ImVec2(-1, 26)))
                    viewport.setSelectedSketch(nullptr);
                ImGui::Separator();
            }
        }
    }

    // ── Selected Body Properties ──
    if (selected) {
        if (ImGui::CollapsingHeader("Selected Body", ImGuiTreeNodeFlags_DefaultOpen)) {
            char nameBuf[128];
            snprintf(nameBuf, sizeof(nameBuf), "%s", selected->getName().c_str());
            ImGui::PushItemWidth(-1);
            if (ImGui::InputText("##Name", nameBuf, sizeof(nameBuf)))
                selected->setName(nameBuf);
            ImGui::PopItemWidth();

            glm::vec3 col = selected->getColor();
            if (ImGui::ColorEdit3("##Color", glm::value_ptr(col), ImGuiColorEditFlags_NoInputs))
                selected->setColor(col);

            bool vis = selected->isVisible();
            if (ImGui::Checkbox("Visible", &vis))
                selected->setVisible(vis);

            glm::vec3 bmin = selected->getBoundsMin();
            glm::vec3 bmax = selected->getBoundsMax();
            glm::vec3 sz = bmax - bmin;
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.55f, 0.65f, 1.0f));
            ImGui::Text("%.1f x %.1f x %.1f", sz.x, sz.y, sz.z);
            ImGui::Text("%d faces", (int)selected->getTriangles().size());
            ImGui::PopStyleColor();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.45f, 0.12f, 0.12f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.58f, 0.18f, 0.18f, 1.0f));
            if (ImGui::Button("Delete", ImVec2(-1, 26))) {
                auto cmd = std::make_unique<Modeling3D::DeleteBodyCommand>(
                    viewport.getScene(), selected);
                cmd->execute();  // detaches the body
                viewport.getScene()->getCommandManager().execute(std::move(cmd));
                appContext.consoleMessage = "Body deleted (Ctrl+Z to undo)";
                selected = nullptr;
            }
            ImGui::PopStyleColor(2);
            ImGui::Separator();
        }
    }

    // ── Extrude (context-sensitive) ──
    if (tool == Modeling3D::Tool3DType::Extrude) {
        if (ImGui::CollapsingHeader("Extrude", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Sketch profile status
            auto* selSk = viewport.getSelectedSketch();
            auto* actSk = viewport.getScene()->getActiveSketch();
            auto* profileSk = selSk ? selSk : actSk;
            if (profileSk) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.88f, 0.15f, 1.0f));
                ImGui::Text("✓ Profile: %s", profileSk->getName().c_str());
                ImGui::PopStyleColor();
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                ImGui::TextWrapped("Click on sketch lines to select profile");
                ImGui::PopStyleColor();
            }
            ImGui::Spacing();

            float dist = viewport.getExtrudeDistance();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##ExtDist", &dist, 0.5f, 0.1f, 500.0f, "Distance: %.1f"))
                viewport.setExtrudeDistance(dist);
            ImGui::PopItemWidth();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.45f, 0.22f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.55f, 0.28f, 1.0f));
            if (ImGui::Button("Apply Extrude", ImVec2(-1, 30))) {
                if (viewport.executeExtrude()) {
                    appContext.consoleMessage = "Extrude successful";
                    viewport.setActiveTool(Modeling3D::Tool3DType::Select);
                } else {
                    appContext.consoleMessage = "Extrude failed — select a closed sketch profile first";
                }
            }
            ImGui::PopStyleColor(2);
            ImGui::Separator();
        }
    }

    // ── Revolve (context-sensitive) ──
    if (tool == Modeling3D::Tool3DType::Revolve) {
        if (ImGui::CollapsingHeader("Revolve", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Sketch profile status
            auto* selSk = viewport.getSelectedSketch();
            auto* actSk = viewport.getScene()->getActiveSketch();
            auto* profileSk = selSk ? selSk : actSk;
            if (profileSk) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.88f, 0.15f, 1.0f));
                ImGui::Text("✓ Profile: %s", profileSk->getName().c_str());
                ImGui::PopStyleColor();
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                ImGui::TextWrapped("Click on sketch lines to select profile");
                ImGui::PopStyleColor();
            }
            ImGui::Spacing();

            // Axis selector
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.65f, 0.80f, 1.0f));
            ImGui::TextUnformatted("Revolve Axis:");
            ImGui::PopStyleColor();

            auto curAxis = viewport.getRevolveAxisMode();
            struct { const char* label; Modeling3D::RevolveAxis val; } axes[] = {
                { "Sketch X",  Modeling3D::RevolveAxis::SketchX  },
                { "Sketch Y",  Modeling3D::RevolveAxis::SketchY  },
                { "Global X",  Modeling3D::RevolveAxis::GlobalX  },
                { "Global Y",  Modeling3D::RevolveAxis::GlobalY  },
                { "Global Z",  Modeling3D::RevolveAxis::GlobalZ  },
            };
            for (auto& a : axes) {
                bool sel = (curAxis == a.val);
                if (ImGui::RadioButton(a.label, sel))
                    viewport.setRevolveAxisMode(a.val);
                ImGui::SameLine();
            }
            ImGui::NewLine();

            // Angle
            float angle = viewport.getRevolveAngle();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##RevAngle", &angle, 1.0f, 1.0f, 360.0f, "Angle: %.0f°"))
                viewport.setRevolveAngle(angle);
            ImGui::PopItemWidth();

            // Tip
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.50f, 0.58f, 1.0f));
            ImGui::TextWrapped("Tip: Profile must be on ONE side of the revolve axis.");
            ImGui::PopStyleColor();

            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.30f, 0.50f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.38f, 0.60f, 1.0f));
            if (ImGui::Button("Apply Revolve", ImVec2(-1, 30))) {
                if (viewport.executeRevolve()) {
                    appContext.consoleMessage = "Revolve successful";
                    viewport.setActiveTool(Modeling3D::Tool3DType::Select);
                } else {
                    appContext.consoleMessage = "Revolve failed — check profile & axis";
                }
            }
            ImGui::PopStyleColor(2);
            ImGui::Separator();
        }
    }

    // ── Fillet / Chamfer (context-sensitive — with edge selection) ──
    if (tool == Modeling3D::Tool3DType::Fillet) {
        if (ImGui::CollapsingHeader("Fillet / Chamfer", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (!selected) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.75f, 0.2f, 1.0f));
                ImGui::TextWrapped("Click a body in the viewport first,\nthen pick its edges to fillet.");
                ImGui::PopStyleColor();
                ImGui::Separator();
            } else {
            // Edge selection status
            int nSel = (int)viewport.filletSelectedEdges.size();
            if (nSel == 0) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.75f, 0.2f, 1.0f));
                ImGui::TextWrapped("Click edges on the body to select them.\n"
                                   "Leave empty to fillet ALL edges.");
                ImGui::PopStyleColor();
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.9f, 0.5f, 1.0f));
                ImGui::Text("%d edge(s) selected", nSel);
                ImGui::PopStyleColor();
                ImGui::SameLine();
                if (ImGui::SmallButton("Clear##FE")) {
                    viewport.filletSelectedEdges.clear();
                    viewport.filletHoveredEdge = -1;
                    viewport.rebuildToolPreview();
                }
            }
            ImGui::Spacing();

            float filletR = viewport.getFilletRadius();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##FilR", &filletR, 0.1f, 0.05f, 50.0f, "Fillet R: %.2f"))
                viewport.setFilletRadius(filletR);
            ImGui::PopItemWidth();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.40f, 0.30f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.50f, 0.35f, 1.0f));
            if (ImGui::Button(nSel > 0 ? "Apply Fillet (Selected)" : "Apply Fillet (All)", ImVec2(-1, 26))) {
#ifdef USE_OCCT
                bool ok = false;
                TopoDS_Shape oldShape = selected->getOCCTShape();
                glm::mat4 oldTransform = selected->getTransform();
                if (nSel > 0)
                    ok = selected->filletEdgesByIndex(viewport.filletSelectedEdges, filletR);
                else
                    ok = selected->filletAllEdges(filletR);
                if (ok) {
                    selected->uploadToGPU();
                    viewport.getScene()->getCommandManager().execute(
                        std::make_unique<Modeling3D::ModifyShapeCommand>(
                            selected, oldShape, oldTransform,
                            selected->getOCCTShape(), selected->getTransform(),
                            "Fillet edges"));
                    viewport.filletSelectedEdges.clear();
                    viewport.filletHoveredEdge = -1;
                    viewport.rebuildToolPreview();
                    appContext.consoleMessage = "Fillet applied (Ctrl+Z to undo)";
                } else {
                    appContext.consoleMessage = "Fillet failed — needs closed manifold mesh";
                }
#else
                appContext.consoleMessage = "Fillet requires OpenCASCADE";
#endif
            }
            ImGui::PopStyleColor(2);

            ImGui::Spacing();
            float chamferD = viewport.getChamferDistance();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##ChamD", &chamferD, 0.1f, 0.05f, 50.0f, "Chamfer D: %.2f"))
                viewport.setChamferDistance(chamferD);
            ImGui::PopItemWidth();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.30f, 0.30f, 0.40f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.35f, 0.50f, 1.0f));
            if (ImGui::Button("Apply Chamfer (All)", ImVec2(-1, 26))) {
#ifdef USE_OCCT
                TopoDS_Shape oldShape = selected->getOCCTShape();
                glm::mat4 oldTransform = selected->getTransform();
                if (selected->chamferAllEdges(chamferD)) {
                    selected->uploadToGPU();
                    viewport.getScene()->getCommandManager().execute(
                        std::make_unique<Modeling3D::ModifyShapeCommand>(
                            selected, oldShape, oldTransform,
                            selected->getOCCTShape(), selected->getTransform(),
                            "Chamfer edges"));
                    appContext.consoleMessage = "Chamfer applied (Ctrl+Z to undo)";
                } else {
                    appContext.consoleMessage = "Chamfer failed";
                }
#else
                appContext.consoleMessage = "Chamfer requires OpenCASCADE";
#endif
            }
            ImGui::PopStyleColor(2);
            ImGui::Separator();
        }
        }  // end: selected body required
    }

    // ── Move Body (position a body) ──
    if (tool == Modeling3D::Tool3DType::MoveBody) {
        if (ImGui::CollapsingHeader("Move Body", ImGuiTreeNodeFlags_DefaultOpen)) {
            Body3D* mb = viewport.getMoveBody();
            if (mb) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.9f, 0.5f, 1.0f));
                ImGui::Text("Body: %s", mb->getName().c_str());
                ImGui::PopStyleColor();
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                ImGui::TextWrapped("Click the body you want to move");
                ImGui::PopStyleColor();
            }
            ImGui::Spacing();

            static const char* modeNames[] = { "Translate (XYZ)", "Mate to face (coincident)", "Rotate (axis + angle)" };
            int m = (int)viewport.getMoveMode();
            ImGui::PushItemWidth(-1);
            if (ImGui::Combo("##MoveMode", &m, modeNames, 3))
                viewport.setMoveMode((Modeling3D::Viewport3D::MoveMode)m);
            ImGui::PopItemWidth();

            bool ready = false;
            if (viewport.getMoveMode() == Modeling3D::Viewport3D::MoveMode::Translate) {
                glm::dvec3 d = viewport.getMoveDelta();
                float df[3] = { (float)d.x, (float)d.y, (float)d.z };
                ImGui::PushItemWidth(-1);
                if (ImGui::DragFloat3("##MoveDelta", df, 0.25f, -500.0f, 500.0f, "%.1f"))
                    viewport.setMoveDelta(glm::dvec3(df[0], df[1], df[2]));
                ImGui::PopItemWidth();
                ready = mb && (std::abs(df[0]) + std::abs(df[1]) + std::abs(df[2]) > 1e-4);
            } else if (viewport.getMoveMode() == Modeling3D::Viewport3D::MoveMode::CoincidentFace) {
                if (!mb) { ImGui::TextDisabled("Select a body first"); }
                else if (!viewport.isMoveFacePicked()) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.75f, 0.2f, 1.0f));
                    ImGui::TextWrapped("Click the face ON the moving body that will mate");
                    ImGui::PopStyleColor();
                } else if (!viewport.isMoveTargetPicked()) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.75f, 0.2f, 1.0f));
                    ImGui::TextWrapped("Click the target face on the other body");
                    ImGui::PopStyleColor();
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.9f, 0.5f, 1.0f));
                    ImGui::Text("Mate with: %s", viewport.getMoveTargetBody()->getName().c_str());
                    ImGui::PopStyleColor();
                    ready = true;
                }
                if (viewport.isMoveFacePicked() || viewport.isMoveTargetPicked()) {
                    if (ImGui::SmallButton("Re-pick faces##mv"))
                        viewport.clearMoveFaces();
                }
            } else {  // Rotate
                static const char* axNames[] = { "X axis", "Y axis", "Z axis" };
                int ax = viewport.getRotateAxis();
                ImGui::PushItemWidth(-1);
                if (ImGui::Combo("##RotAxis", &ax, axNames, 3))
                    viewport.setRotateAxis(ax);
                float ang = viewport.getRotateAngle();
                if (ImGui::DragFloat("##RotAng", &ang, 1.0f, -360.0f, 360.0f, "Angle: %.1f deg"))
                    viewport.setRotateAngle(ang);
                ImGui::PopItemWidth();
                ready = mb && std::abs(ang) > 1e-4;
            }

            ImGui::Spacing();
            if (!ready) ImGui::BeginDisabled();
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.18f, 0.30f, 0.50f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.38f, 0.60f, 1.0f));
            if (ImGui::Button("Apply Move", ImVec2(-1, 30))) {
                if (viewport.executeMoveBody())
                    appContext.consoleMessage = "Body moved (Ctrl+Z to undo)";
                else
                    appContext.consoleMessage = "Nothing to move";
            }
            ImGui::PopStyleColor(2);
            if (!ready) ImGui::EndDisabled();
            ImGui::Separator();
        }
    }

    // ── Combine (SolidWorks-style N-body operation) ──
    if (tool == Modeling3D::Tool3DType::Combine) {
        if (ImGui::CollapsingHeader("Combine", ImGuiTreeNodeFlags_DefaultOpen)) {
            static int combineOp = 0;  // 0=Add 1=Subtract 2=Common
            static const char* opNames[] = { "Add (fuse)", "Subtract (cut all)", "Common (intersect all)" };
            ImGui::PushItemWidth(-1);
            ImGui::Combo("##CombOp", &combineOp, opNames, 3);
            ImGui::PopItemWidth();
            Body3D* base = viewport.getMergeBase();
            const auto& tools = viewport.getMergeTools();

            if (base) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.9f, 0.5f, 1.0f));
                ImGui::Text("Base: %s", base->getName().c_str());
                ImGui::PopStyleColor();
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                ImGui::TextWrapped("Click the BASE body in the viewport");
                ImGui::PopStyleColor();
            }
            if (viewport.isMergeFacePicked()) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.9f, 0.5f, 1.0f));
                ImGui::TextUnformatted("Face picked");
                ImGui::PopStyleColor();
                static const char* alignNames[] = { "Flat on face", "Centered on point" };
                int al = viewport.getMergeAlign();
                ImGui::PushItemWidth(-1);
                if (ImGui::Combo("##MergeAlign", &al, alignNames, 2))
                    viewport.setMergeAlign(al);
                float off = viewport.getMergeOffset();
                if (ImGui::DragFloat("##MergeOff", &off, 0.25f, -200.0f, 200.0f, "Offset: %.1f"))
                    viewport.setMergeOffset(off);
                ImGui::PopItemWidth();
                if (ImGui::SmallButton("Re-pick face##merge"))
                    viewport.clearMergeFace();
            } else if (base) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.75f, 0.2f, 1.0f));
                ImGui::TextWrapped("Click a face of the base to set where tools merge onto");
                ImGui::PopStyleColor();
            }
            ImGui::PushStyleColor(ImGuiCol_Text,
                                  tools.empty() ? ImVec4(1.0f, 0.75f, 0.2f, 1.0f)
                                                : ImVec4(0.3f, 0.9f, 0.5f, 1.0f));
            ImGui::Text("Tools: %d", (int)tools.size());
            ImGui::PopStyleColor();
            if (!tools.empty()) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.50f, 0.58f, 1.0f));
                for (auto* t : tools)
                    ImGui::BulletText("%s", t->getName().c_str());
                ImGui::PopStyleColor();
            }
            ImGui::Spacing();

            if (ImGui::SmallButton("Select all visible##merge")) {
                viewport.mergeSelectAll();
                appContext.consoleMessage = "Merge: all visible bodies selected";
            }
            if (base || !tools.empty()) {
                ImGui::SameLine();
                if (ImGui::SmallButton("Clear##merge"))
                    viewport.clearMergeSelection();
            }
            ImGui::Spacing();

            bool ready = base && !tools.empty();
            if (!ready) ImGui::BeginDisabled();
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.15f, 0.45f, 0.22f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.55f, 0.28f, 1.0f));
            if (ImGui::Button(viewport.isMergeFacePicked() && combineOp == 0
                                  ? "Move onto Face + Combine" : "Apply Combine",
                              ImVec2(-1, 30))) {
                Modeling3D::BoolOp op = (combineOp == 0) ? Modeling3D::BoolOp::Union
                                      : (combineOp == 1) ? Modeling3D::BoolOp::Subtract
                                                         : Modeling3D::BoolOp::Intersect;
                if (viewport.executeCombine(op))
                    appContext.consoleMessage = "Combine applied (Ctrl+Z to undo)";
                else
                    appContext.consoleMessage = "Combine failed — bodies must have B-Rep data";
            }
            ImGui::PopStyleColor(2);
            if (!ready) ImGui::EndDisabled();
            ImGui::Separator();
        }
    }

    // ── Boolean (context-sensitive — base/tool body picking) ──
    if (tool == Modeling3D::Tool3DType::Boolean) {
        if (ImGui::CollapsingHeader("Boolean", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto base = viewport.getBoolBaseBody();
            auto toolB = viewport.getBoolToolBody();

            // Operation selector
            auto op = viewport.getBoolOpMode();
            struct { const char* label; Modeling3D::BoolOp val; } ops[] = {
                { "Union",     Modeling3D::BoolOp::Union     },
                { "Subtract",  Modeling3D::BoolOp::Subtract  },
                { "Intersect", Modeling3D::BoolOp::Intersect },
            };
            for (auto& o : ops) {
                if (ImGui::RadioButton(o.label, op == o.val))
                    viewport.setBoolOpMode(o.val);
                ImGui::SameLine();
            }
            ImGui::NewLine();
            ImGui::Spacing();

            // Step-by-step picking status
            if (!base) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                ImGui::TextWrapped("1. Click the BASE body in the viewport");
                ImGui::PopStyleColor();
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.9f, 0.5f, 1.0f));
                ImGui::Text("✓ Base: %s", base->getName().c_str());
                ImGui::PopStyleColor();
            }
            if (base && !toolB) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.75f, 0.2f, 1.0f));
                ImGui::TextWrapped("2. Click the TOOL body in the viewport");
                ImGui::PopStyleColor();
            } else if (toolB) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.9f, 0.5f, 1.0f));
                ImGui::Text("✓ Tool: %s", toolB->getName().c_str());
                ImGui::PopStyleColor();
            }
            if (base && toolB) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.50f, 0.58f, 1.0f));
                const char* opDesc = (op == Modeling3D::BoolOp::Union)   ? "Base + Tool merged into Base"
                                   : (op == Modeling3D::BoolOp::Subtract)? "Tool cut away from Base"
                                                                         : "Only the overlap is kept";
                ImGui::TextWrapped("%s", opDesc);
                if (!viewport.toolPreviewBody) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.2f, 1.0f));
                    ImGui::TextWrapped("⚠ No preview — bodies may not overlap");
                    ImGui::PopStyleColor();
                }
                ImGui::PopStyleColor();
            }

            ImGui::Spacing();
            if (base || toolB) {
                if (ImGui::SmallButton("Clear selection##Bool"))
                    viewport.clearBooleanSelection();
                ImGui::Spacing();
            }

            bool ready = base && toolB && base != toolB;
            if (!ready) ImGui::BeginDisabled();
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.15f, 0.45f, 0.22f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.55f, 0.28f, 1.0f));
            const char* applyLbl = (op == Modeling3D::BoolOp::Union)   ? "Apply Union"
                                 : (op == Modeling3D::BoolOp::Subtract)? "Apply Subtract"
                                                                       : "Apply Intersect";
            if (ImGui::Button(applyLbl, ImVec2(-1, 30))) {
                if (viewport.executeBoolean()) {
                    appContext.consoleMessage = "Boolean applied (Ctrl+Z to undo)";
                } else {
                    appContext.consoleMessage = "Boolean failed — bodies may not overlap or lack B-Rep data";
                }
            }
            ImGui::PopStyleColor(2);
            if (!ready) {
                ImGui::EndDisabled();
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                    ImGui::SetTooltip("Select a base body and a tool body first");
            }
            ImGui::Separator();
        }
    }

    // ── Cut-Extrude / Pocket ──
    if (tool == Modeling3D::Tool3DType::CutExtrude) {
        if (ImGui::CollapsingHeader("Cut-Extrude", ImGuiTreeNodeFlags_DefaultOpen)) {
            int step = viewport.getCutStep();
            Body3D* target = viewport.getCutTarget();
            auto* actSk = viewport.getScene()->getActiveSketch();

            // Step-by-step status
            ImGui::PushStyleColor(ImGuiCol_Text, step >= 1 ? ImVec4(0.3f, 0.9f, 0.5f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::Text("1. %s", step >= 1 ? "✓ Target body selected" : "Click the target body");
            ImGui::PopStyleColor();
            ImGui::PushStyleColor(ImGuiCol_Text, step >= 2 ? ImVec4(0.3f, 0.9f, 0.5f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::Text("2. %s", step >= 2 ? "✓ Face selected — draw on the grid"
                                           : "Click a face of the body");
            ImGui::PopStyleColor();
            ImGui::PushStyleColor(ImGuiCol_Text, step >= 2 ? ImVec4(1.0f, 0.88f, 0.15f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            if (step >= 2)
                ImGui::Text("3. %s", (actSk && actSk->getShapes().size() > 0)
                                     ? "✓ Profile drawn — press Apply"
                                     : "Draw the profile (Line tool active)");
            else
                ImGui::TextUnformatted("3. Draw the 2D profile on the face");
            ImGui::PopStyleColor();
            ImGui::Spacing();

            if (step >= 2) {
                static const char* dirNames[] = { "Blind (into body)", "Through All" };
                int dirIdx = (viewport.getCutDirMode() == Modeling3D::CutDirMode::ThroughAll) ? 1 : 0;
                ImGui::PushItemWidth(-1);
                if (ImGui::Combo("##CutDir", &dirIdx, dirNames, 2))
                    viewport.setCutDirMode(dirIdx == 1 ? Modeling3D::CutDirMode::ThroughAll
                                                       : Modeling3D::CutDirMode::Blind);
                if (viewport.getCutDirMode() == Modeling3D::CutDirMode::Blind) {
                    float d = viewport.getCutDepth();
                    if (ImGui::DragFloat("##CutDepth", &d, 0.25f, 0.1f, 500.0f, "Depth: %.1f"))
                        viewport.setCutDepth(d);
                }
                ImGui::PopItemWidth();

                bool ready = (actSk && actSk->getShapes().size() > 0);
                if (!ready) ImGui::BeginDisabled();
                ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.50f, 0.15f, 0.15f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.62f, 0.20f, 0.20f, 1.0f));
                if (ImGui::Button("Apply Cut", ImVec2(-1, 30))) {
                    if (viewport.executeCutExtrude())
                        appContext.consoleMessage = "Cut applied (Ctrl+Z to undo)";
                    else
                        appContext.consoleMessage = "Cut failed — profile must overlap the body";
                }
                ImGui::PopStyleColor(2);
                if (!ready) ImGui::EndDisabled();
            }
            ImGui::Separator();
        }
    }

    // ── Hole (lite wizard) ──
    if (tool == Modeling3D::Tool3DType::Hole) {
        if (ImGui::CollapsingHeader("Hole", ImGuiTreeNodeFlags_DefaultOpen)) {
            Body3D* target = viewport.getScene()->getSelectedBody();
            if (target) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.9f, 0.5f, 1.0f));
                ImGui::Text("✓ Target: %s", target->getName().c_str());
                ImGui::PopStyleColor();
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                ImGui::TextWrapped("Select a body, then click its face to place the hole");
                ImGui::PopStyleColor();
            }
            if (viewport.isHolePicked()) {
                glm::dvec3 c = viewport.getHoleCenter();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.9f, 0.5f, 1.0f));
                ImGui::Text("✓ Hole at (%.1f, %.1f, %.1f)", c.x, c.y, c.z);
                ImGui::PopStyleColor();
                ImGui::SameLine();
                if (ImGui::SmallButton("Clear##hole")) viewport.clearHolePick();
            }
            ImGui::Spacing();

            static const char* holeTypeNames[] = { "Through", "Blind", "Counterbore" };
            int ht = (int)viewport.getHoleType();
            ImGui::PushItemWidth(-1);
            if (ImGui::Combo("##HoleType", &ht, holeTypeNames, 3))
                viewport.setHoleType((Modeling3D::HoleType)ht);
            float d = viewport.getHoleDiameter();
            if (ImGui::DragFloat("##HoleDia", &d, 0.1f, 0.2f, 200.0f, "Diameter: %.2f"))
                viewport.setHoleDiameter(d);
            if (viewport.getHoleType() == Modeling3D::HoleType::Blind) {
                float hd = viewport.getHoleDepth();
                if (ImGui::DragFloat("##HoleDepth", &hd, 0.25f, 0.1f, 500.0f, "Depth: %.1f"))
                    viewport.setHoleDepth(hd);
            }
            if (viewport.getHoleType() == Modeling3D::HoleType::Counterbore) {
                float cd = viewport.getCBoreDiameter();
                if (ImGui::DragFloat("##CBDia", &cd, 0.1f, 0.2f, 200.0f, "Cbore ∅: %.2f"))
                    viewport.setCBoreDiameter(cd);
                float cdep = viewport.getCBoreDepthValue();
                if (ImGui::DragFloat("##CBDepth", &cdep, 0.25f, 0.1f, 200.0f, "Cbore depth: %.1f"))
                    viewport.setCBoreDepth(cdep);
            }
            ImGui::PopItemWidth();

            bool ready = target && viewport.isHolePicked();
            if (!ready) ImGui::BeginDisabled();
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.50f, 0.15f, 0.15f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.62f, 0.20f, 0.20f, 1.0f));
            if (ImGui::Button("Apply Hole", ImVec2(-1, 30))) {
                if (viewport.executeHole())
                    appContext.consoleMessage = "Hole applied (Ctrl+Z to undo)";
                else
                    appContext.consoleMessage = "Hole failed";
            }
            ImGui::PopStyleColor(2);
            if (!ready) ImGui::EndDisabled();
            ImGui::Separator();
        }
    }

    // ── Pattern (linear / circular) ──
    if (tool == Modeling3D::Tool3DType::Pattern) {
        if (ImGui::CollapsingHeader("Pattern", ImGuiTreeNodeFlags_DefaultOpen)) {
            Body3D* target = viewport.getScene()->getSelectedBody();
            if (target) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.9f, 0.5f, 1.0f));
                ImGui::Text("✓ Source: %s", target->getName().c_str());
                ImGui::PopStyleColor();
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                ImGui::TextWrapped("Select a body to pattern");
                ImGui::PopStyleColor();
            }
            ImGui::Spacing();

            static const char* kindNames[] = { "Linear", "Circular" };
            int kind = (int)viewport.getPatternKind();
            ImGui::PushItemWidth(-1);
            if (ImGui::Combo("##PatKind", &kind, kindNames, 2))
                viewport.setPatternKind((Modeling3D::PatternKind)kind);

            if (viewport.getPatternKind() == Modeling3D::PatternKind::Linear) {
                static const char* axNames[] = { "X axis", "Y axis", "Z axis" };
                int ax = viewport.getLinearAxis();
                if (ImGui::Combo("##LinAxis", &ax, axNames, 3))
                    viewport.setLinearAxis(ax);
                int n = viewport.getLinearCount();
                if (ImGui::DragInt("##LinCount", &n, 0.1f, 2, 64, "Count: %d"))
                    viewport.setLinearCount(n);
                float s = viewport.getLinearSpacing();
                if (ImGui::DragFloat("##LinSpace", &s, 0.25f, 0.1f, 1000.0f, "Spacing: %.1f"))
                    viewport.setLinearSpacing(s);
            } else {
                static const char* axNames[] = { "X axis", "Y axis", "Z axis" };
                int ax = viewport.getCircAxis();
                if (ImGui::Combo("##CircAxis", &ax, axNames, 3))
                    viewport.setCircAxis(ax);
                static const char* cNames[] = { "World origin", "Source body center", "Pick center body..." };
                int cc = viewport.getCircCenter();
                if (ImGui::Combo("##CircCenter", &cc, cNames, 3))
                    viewport.setCircCenter(cc);
                if (cc == 2) {
                    Body3D* cb = viewport.getCircCenterBody();
                    if (cb) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.9f, 0.5f, 1.0f));
                        ImGui::Text("Center: %s", cb->getName().c_str());
                        ImGui::PopStyleColor();
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Re-pick##cc"))
                            viewport.clearCircCenterBody();
                    } else {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.75f, 0.2f, 1.0f));
                        ImGui::TextWrapped("Click the body whose center is the rotation center");
                        ImGui::PopStyleColor();
                    }
                }
                int n = viewport.getCircCount();
                if (ImGui::DragInt("##CircCount", &n, 0.1f, 2, 64, "Count: %d"))
                    viewport.setCircCount(n);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.50f, 0.58f, 1.0f));
                ImGui::TextWrapped("Full circle about the chosen axis");
                ImGui::PopStyleColor();
            }
            ImGui::PopItemWidth();

            bool ready = target != nullptr;
            if (!ready) ImGui::BeginDisabled();
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.15f, 0.45f, 0.22f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.55f, 0.28f, 1.0f));
            if (ImGui::Button("Apply Pattern", ImVec2(-1, 30))) {
                if (viewport.executePattern())
                    appContext.consoleMessage = "Pattern applied (Ctrl+Z to undo)";
                else
                    appContext.consoleMessage = "Pattern failed";
            }
            ImGui::PopStyleColor(2);
            if (!ready) ImGui::EndDisabled();
            ImGui::Separator();
        }
    }

    // ── Mirror ──
    if (tool == Modeling3D::Tool3DType::Mirror) {
        if (ImGui::CollapsingHeader("Mirror", ImGuiTreeNodeFlags_DefaultOpen)) {
            Body3D* target = viewport.getScene()->getSelectedBody();
            if (target) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.9f, 0.5f, 1.0f));
                ImGui::Text("✓ Source: %s", target->getName().c_str());
                ImGui::PopStyleColor();
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                ImGui::TextWrapped("Select a body to mirror");
                ImGui::PopStyleColor();
            }
            ImGui::Spacing();

            static const char* planeNames[] = { "YZ plane (x = offset)", "XZ plane (y = offset)", "XY plane (z = offset)" };
            ImGui::PushItemWidth(-1);
            int pl = viewport.getMirrorPlane();
            if (ImGui::Combo("##MirPlane", &pl, planeNames, 3))
                viewport.setMirrorPlane(pl);
            float off = viewport.getMirrorOffset();
            if (ImGui::DragFloat("##MirOff", &off, 0.25f, -500.0f, 500.0f, "Offset: %.1f"))
                viewport.setMirrorOffset(off);
            bool fuse = viewport.getMirrorFuse();
            if (ImGui::Checkbox("Fuse with original (symmetric body)", &fuse))
                viewport.setMirrorFuse(fuse);
            ImGui::PopItemWidth();

            bool ready = target != nullptr;
            if (!ready) ImGui::BeginDisabled();
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.15f, 0.45f, 0.22f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.55f, 0.28f, 1.0f));
            if (ImGui::Button("Apply Mirror", ImVec2(-1, 30))) {
                if (viewport.executeMirror())
                    appContext.consoleMessage = viewport.getMirrorFuse()
                        ? "Mirror fused (Ctrl+Z to undo)"
                        : "Mirror body created (Ctrl+Z to undo)";
                else
                    appContext.consoleMessage = "Mirror failed";
            }
            ImGui::PopStyleColor(2);
            if (!ready) ImGui::EndDisabled();
            ImGui::Separator();
        }
    }

    // ── Section View (clip bodies) ──
    if (viewport.getScene()->getBodyCount() > 0) {
        if (ImGui::CollapsingHeader("Section View")) {
            bool en = viewport.isSectionEnabled();
            if (ImGui::Checkbox("Enable section", &en))
                viewport.setSectionEnabled(en);
            if (en) {
                static const char* axNames[] = { "X", "Y", "Z" };
                int ax = viewport.getSectionAxis();
                ImGui::PushItemWidth(-1);
                if (ImGui::Combo("##SectAxis", &ax, axNames, 3))
                    viewport.setSectionAxis(ax);

                // World-bounds slider range across all bodies
                float lo = -1.0f, hi = 1.0f;
                {
                    bool first = true;
                    for (const auto& b : viewport.getScene()->getBodies()) {
                        glm::mat4 M = b->getTransform();
                        glm::vec3 c[2] = { b->getBoundsMin(), b->getBoundsMax() };
                        for (int i = 0; i < 8; ++i) {
                            glm::vec3 corner((i & 1) ? c[1].x : c[0].x,
                                             (i & 2) ? c[1].y : c[0].y,
                                             (i & 4) ? c[1].z : c[0].z);
                            glm::vec3 w = M * glm::vec4(corner, 1.0f);
                            float v = (ax == 0) ? w.x : (ax == 1) ? w.y : w.z;
                            if (first) { lo = hi = v; first = false; }
                            else { lo = std::min(lo, v); hi = std::max(hi, v); }
                        }
                    }
                }
                float p = viewport.getSectionPos();
                if (ImGui::SliderFloat("##SectPos", &p, lo - 0.5f, hi + 0.5f, "Position: %.1f"))
                    viewport.setSectionPos(p);
                ImGui::PopItemWidth();
                bool fl = viewport.isSectionFlipped();
                if (ImGui::Checkbox("Flip side", &fl))
                    viewport.setSectionFlipped(fl);
                bool dbg = viewport.isCapDebugNoStencil();
                if (ImGui::Checkbox("Debug: cap ignores stencil", &dbg))
                    viewport.setCapDebugNoStencil(dbg);
            }
            ImGui::Separator();
        }
    }

    // ── Placement Mode hint ──
    if (tool == Modeling3D::Tool3DType::PlacePrimitive) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.1f, 1.0f));
        ImGui::TextWrapped("PLACEMENT MODE\nClick anywhere on canvas to place.\nRMB or Esc to cancel.");
        ImGui::PopStyleColor();
        if (ImGui::Button("Cancel Placement", ImVec2(-1, 26)))
            viewport.cancelPlacement();
        ImGui::Separator();
    }


    // ── Export (only when bodies exist) ──
    if (viewport.getScene()->getBodyCount() > 0) {
        static std::string exportMsg;
        auto& bodies = viewport.getScene()->getBodies();
        std::vector<Modeling3D::Body3D*> ptrs;
        ptrs.reserve(bodies.size());
        for (auto& b : bodies) ptrs.push_back(b.get());

        UI::renderExportSection(
            "mode3d", "navix_model",
            /*STL */ [&](const std::string& p) {
                return Modeling3D::Exporter3D::exportSTL(ptrs, p, true);
            },
            /*OBJ */ [&](const std::string& p) {
                return Modeling3D::Exporter3D::exportOBJ(ptrs, p);
            },
#ifdef USE_OCCT
            /*STEP*/ [&](const std::string& p) {
                return Modeling3D::Exporter3D::exportSTEP(ptrs, p);
            },
            /*IGES*/ [&](const std::string& p) {
                return Modeling3D::Exporter3D::exportIGES(ptrs, p);
            },
#else
            nullptr, nullptr,
#endif
            /*INP */ nullptr,
            exportMsg);
    }

    // ── Navigation help (collapsed) ──
    if (ImGui::CollapsingHeader("Controls")) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.50f, 0.58f, 1.0f));
        ImGui::TextWrapped(
            "MMB: Orbit\n"
            "Shift+MMB: Pan\n"
            "Ctrl+MMB: Zoom\n"
            "Right-drag: Pan\n"
            "Scroll: Zoom\n"
            "ViewCube: Click to snap view"
        );
        ImGui::PopStyleColor();
    }

    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

} // namespace UI
