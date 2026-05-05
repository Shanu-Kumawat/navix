#include "ui/FeatureTree3D.hpp"
#include "ui/UIColors.hpp"
#include "ApplicationContext.hpp"
#include <imgui.h>
#include <imgui_internal.h>

namespace UI {

void FeatureTree3D::Render(Modeling3D::Viewport3D& viewport, Core::ApplicationContext& appContext) {
    const float ribbonHeight = 48.0f;
    const float statusBarHeight = 28.0f;
    float panelWidth = 220.0f;
    float panelHeight = ImGui::GetIO().DisplaySize.y - ribbonHeight - statusBarHeight;

    ImGui::SetNextWindowPos(ImVec2(0, ribbonHeight));
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.13f, 0.14f, 0.16f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.20f, 0.22f, 0.27f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.25f, 0.28f, 0.35f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 6));
    ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 14.0f);

    ImGui::Begin("##FeatureTree", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar);

    // Title
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.65f, 0.72f, 0.82f, 1.0f));
    ImGui::Text("FEATURE TREE");
    ImGui::PopStyleColor();
    ImGui::Separator();

    auto* scene = viewport.getScene();

    // ── Bodies ──
    bool bodiesOpen = ImGui::TreeNodeEx("Bodies",
        ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth);
    if (bodiesOpen) {
        auto& bodies = scene->getBodies();
        for (size_t i = 0; i < bodies.size(); ++i) {
            auto* body = bodies[i].get();
            bool isSelected = body->isSelected();

            // Highlight selected body
            if (isSelected) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.75f, 1.0f, 1.0f));
            }

            ImGuiTreeNodeFlags nodeFlags =
                ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen |
                ImGuiTreeNodeFlags_SpanAvailWidth;
            if (isSelected) nodeFlags |= ImGuiTreeNodeFlags_Selected;

            // Icon based on shape type
            std::string label = body->getName();
            if (label.empty()) label = "Body." + std::to_string(i + 1);

            // Visibility indicator
            std::string displayLabel = (body->isVisible() ? "  " : "  ") + label;

            ImGui::TreeNodeEx((void*)(intptr_t)i, nodeFlags, "%s", displayLabel.c_str());

            // Click to select
            if (ImGui::IsItemClicked()) {
                scene->selectBody(body);
            }

            // Right-click context menu
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem(body->isVisible() ? "Hide" : "Show")) {
                    body->setVisible(!body->isVisible());
                }
                if (ImGui::MenuItem("Delete")) {
                    scene->removeBody(body);
                    ImGui::EndPopup();
                    if (isSelected) ImGui::PopStyleColor();
                    break; // Iterator invalidated
                }
                ImGui::Separator();
                // Color picker
                glm::vec3 col = body->getColor();
                float c[3] = {col.r, col.g, col.b};
                if (ImGui::ColorEdit3("Color", c, ImGuiColorEditFlags_NoInputs)) {
                    body->setColor(glm::vec3(c[0], c[1], c[2]));
                }
                ImGui::EndPopup();
            }

            if (isSelected) {
                ImGui::PopStyleColor();
            }
        }
        ImGui::TreePop();
    }

    ImGui::Spacing();

    // ── Sketches ──
    bool sketchesOpen = ImGui::TreeNodeEx("Sketches",
        ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth);
    if (sketchesOpen) {
        if (scene->isSketchActive()) {
            auto* sketch = scene->getActiveSketch();
            if (sketch) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.8f, 1.0f, 1.0f));
                ImGui::BulletText("%s [active] (%zu shapes)",
                    sketch->getName().c_str(),
                    sketch->getShapes().size());
                ImGui::PopStyleColor();
            }
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.48f, 0.55f, 1.0f));
            ImGui::Text("  No active sketch");
            ImGui::PopStyleColor();
        }
        ImGui::TreePop();
    }

    ImGui::Spacing();
    ImGui::Separator();

    // ── Scene Stats ──
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.50f, 0.58f, 1.0f));
    ImGui::Text("Bodies: %d", scene->getBodyCount());
    ImGui::Text("Faces:  %d", scene->getTotalFaces());
    ImGui::Text("Edges:  %d", scene->getTotalEdges());
    ImGui::PopStyleColor();

    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);
}

} // namespace UI
