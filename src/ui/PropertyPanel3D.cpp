#include "ui/PropertyPanel3D.hpp"
#include "ui/UIColors.hpp"
#include "ApplicationContext.hpp"
#include "modeling3d/Exporter3D.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>

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
                viewport.getScene()->removeBody(selected);
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
            if (selSk || actSk) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.88f, 0.15f, 1.0f));
                ImGui::TextUnformatted("✓ Profile: sketch selected");
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
            if (selSk || actSk) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.88f, 0.15f, 1.0f));
                ImGui::TextUnformatted("✓ Profile: sketch selected");
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
    if (tool == Modeling3D::Tool3DType::Fillet && selected) {
        if (ImGui::CollapsingHeader("Fillet / Chamfer", ImGuiTreeNodeFlags_DefaultOpen)) {
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
                bool ok = false;
                if (nSel > 0)
                    ok = selected->filletEdgesByIndex(viewport.filletSelectedEdges, filletR);
                else
                    ok = selected->filletAllEdges(filletR);
                if (ok) {
                    selected->uploadToGPU();
                    viewport.filletSelectedEdges.clear();
                    viewport.filletHoveredEdge = -1;
                    viewport.rebuildToolPreview();
                    appContext.consoleMessage = "Fillet applied";
                } else {
                    appContext.consoleMessage = "Fillet failed — needs closed manifold mesh";
                }
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
                if (selected->chamferAllEdges(chamferD)) {
                    selected->uploadToGPU();
                    appContext.consoleMessage = "Chamfer applied";
                } else {
                    appContext.consoleMessage = "Chamfer failed";
                }
            }
            ImGui::PopStyleColor(2);
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
        if (ImGui::CollapsingHeader("Export")) {
            static char path[256] = "export";
            ImGui::PushItemWidth(-1);
            ImGui::InputText("##ExPath", path, sizeof(path));
            ImGui::PopItemWidth();

            auto& bodies = viewport.getScene()->getBodies();
            std::vector<Modeling3D::Body3D*> ptrs;
            ptrs.reserve(bodies.size());
            for (auto& b : bodies) ptrs.push_back(b.get());

            if (ImGui::Button("STL", ImVec2(-1, 24))) {
                std::string p = std::string(path) + ".stl";
                Modeling3D::Exporter3D::exportSTL(ptrs, p, true);
                appContext.consoleMessage = "Exported: " + p;
            }
            if (ImGui::Button("OBJ", ImVec2(-1, 24))) {
                std::string p = std::string(path) + ".obj";
                Modeling3D::Exporter3D::exportOBJ(ptrs, p);
                appContext.consoleMessage = "Exported: " + p;
            }
#ifdef USE_OCCT
            if (ImGui::Button("STEP", ImVec2(-1, 24))) {
                std::string p = std::string(path) + ".step";
                Modeling3D::Exporter3D::exportSTEP(ptrs, p);
                appContext.consoleMessage = "Exported: " + p;
            }
            if (ImGui::Button("IGES", ImVec2(-1, 24))) {
                std::string p = std::string(path) + ".igs";
                Modeling3D::Exporter3D::exportIGES(ptrs, p);
                appContext.consoleMessage = "Exported: " + p;
            }
#endif
            ImGui::Separator();
        }
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
