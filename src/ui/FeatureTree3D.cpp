#include "ui/FeatureTree3D.hpp"
#include "ui/UIColors.hpp"
#include "ui/UIHelpers.hpp"
#include "ApplicationContext.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <cstring>

namespace {
std::string kindToIcon(const std::string& kind) {
    if (kind == "Box")     return "tree_box";
    if (kind == "Cylinder")return "tree_cylinder";
    if (kind == "Sphere")  return "tree_sphere";
    if (kind == "Cone")    return "tree_cone";
    if (kind == "Torus")   return "tree_torus";
    if (kind == "Extrude") return "tree_extrude";
    if (kind == "Revolve") return "tree_extrude";
    if (kind == "Mirror")  return "tree_mirror";
    if (kind == "Pattern") return "tree_pattern";
    if (kind == "Boolean") return "tree_boolean";
    if (kind == "Import")  return "tree_import";
    return "tree_box";
}
void renderKindIcon(const std::string& iconName) {
    unsigned int tex = GetIconTexture(iconName);
    if (tex) {
        ImGui::Image((ImTextureID)(intptr_t)tex, ImVec2(18, 18));
        ImGui::SameLine();
    } else {
        ImGui::TextUnformatted("*");
        ImGui::SameLine();
    }
}
bool renderEyeToggle(bool visible) {
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 22);
    std::string eyeIcon = visible ? "tree_eye_open" : "tree_eye_closed";
    unsigned int tex = GetIconTexture(eyeIcon);
    if (tex) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.28f, 0.35f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.33f, 0.42f, 0.8f));
        bool pressed = ImGui::ImageButton("##eye", (ImTextureID)(intptr_t)tex, ImVec2(16, 16));
        ImGui::PopStyleColor(3);
        return pressed;
    }
    return ImGui::SmallButton(visible ? "O" : "--");
}
} // anonymous namespace

namespace UI {

void FeatureTree3D::Render(Modeling3D::Viewport3D& viewport, Core::ApplicationContext& appContext) {
    const float ribbonHeight = 48.0f;
    const float statusBarHeight = 28.0f;
    float panelWidth = 220.0f;
    float panelHeight = ImGui::GetIO().DisplaySize.y - ribbonHeight - statusBarHeight;

    ImGui::SetNextWindowPos(ImVec2(0, ribbonHeight));
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight));
    ImGui::PushStyleColor(ImGuiCol_WindowBg,      ImVec4(0.13f, 0.14f, 0.16f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.20f, 0.22f, 0.27f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.25f, 0.28f, 0.35f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 6));
    ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 14.0f);

    ImGui::Begin("##FeatureTree", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar);

    // Title
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.70f, 0.90f, 1.0f));
    ImGui::TextUnformatted("FEATURE TREE");
    ImGui::PopStyleColor();
    ImGui::Separator();

    auto* scene = viewport.getScene();

    // ── Bodies ──
    bool bodiesOpen = ImGui::TreeNodeEx("Bodies",
        ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth);
    if (bodiesOpen) {
        // Inline-rename state (one rename at a time)
        static uint64_t renameId = 0;
        static char renameBuf[128] = "";

        auto& bodies = scene->getBodies();
        for (size_t i = 0; i < bodies.size(); ++i) {
            auto* body = bodies[i].get();
            bool isSelected = body->isSelected();
            ImGui::PushID((int)i);

            // Kind icon
            renderKindIcon(kindToIcon(body->getKind()));

            // Name (or inline rename editor)
            if (renameId == body->getId()) {
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 30);
                ImGui::InputText("##rename", renameBuf, sizeof(renameBuf));
                bool commit = ImGui::IsKeyPressed(ImGuiKey_Enter) ||
                              ImGui::IsKeyPressed(ImGuiKey_KeypadEnter);
                bool closed = ImGui::IsItemDeactivatedAfterEdit();
                ImGui::PopItemWidth();
                if (commit || closed) {
                    if (renameBuf[0] != 0) body->setName(renameBuf);
                    renameId = 0;
                }
            } else {
                ImGuiTreeNodeFlags nodeFlags =
                    ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                if (isSelected) nodeFlags |= ImGuiTreeNodeFlags_Selected;
                if (isSelected)
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.75f, 1.0f, 1.0f));
                ImGui::TreeNodeEx((void*)(intptr_t)i, nodeFlags, "%s",
                                  body->getName().c_str());
                if (isSelected) ImGui::PopStyleColor();

                // Click to select / click again to deselect
                if (ImGui::IsItemClicked()) {
                    if (isSelected) scene->clearSelection();
                    else scene->selectBody(body);
                }

                // Double-click to rename
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    renameId = body->getId();
                    snprintf(renameBuf, sizeof(renameBuf), "%s", body->getName().c_str());
                }
            }

            // Right-click context menu (must follow the row item directly —
            // it attaches to the LAST item, which the eye toggle would steal)
            if (ImGui::BeginPopupContextItem("##bodyctx")) {
                if (ImGui::MenuItem("Rename")) {
                    renameId = body->getId();
                    snprintf(renameBuf, sizeof(renameBuf), "%s", body->getName().c_str());
                }
                if (ImGui::MenuItem(body->isVisible() ? "Hide" : "Show")) {
                    body->setVisible(!body->isVisible());
                }
                if (ImGui::MenuItem("Delete")) {
                    auto cmd = std::make_unique<Modeling3D::DeleteBodyCommand>(scene, body);
                    cmd->execute();  // detaches the body
                    scene->getCommandManager().execute(std::move(cmd));
                    appContext.consoleMessage = "Body deleted (Ctrl+Z to undo)";
                    ImGui::EndPopup();
                    ImGui::PopID();
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

            // Visibility eye toggle (after the context menu declaration)
            if (renderEyeToggle(body->isVisible()))
                body->setVisible(!body->isVisible());

            ImGui::PopID();
        }
        ImGui::TreePop();
    }

    ImGui::Spacing();

    // ── Sketches ──
    bool sketchesOpen = ImGui::TreeNodeEx("Sketches",
        ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth);
    if (sketchesOpen) {
        auto& sketches = scene->getSketches();
        if (sketches.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.48f, 0.55f, 1.0f));
            ImGui::Text("  No sketches yet");
            ImGui::PopStyleColor();
        }
        for (size_t i = 0; i < sketches.size(); ++i) {
            auto* sk = sketches[i].get();
            bool isActive = scene->isSketchActive() && sk == scene->getActiveSketch();
            bool isSel = (viewport.getSelectedSketch() == sk);

            std::string label = sk->getName();
            if (isActive) label += "  [active]";

            ImGui::PushID((int)i);
            renderKindIcon("tree_sketch");
            if (isActive) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.8f, 1.0f, 1.0f));
            else if (!sk->isVisible()) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.42f, 0.48f, 1.0f));
            ImGui::TreeNodeEx((void*)(intptr_t)(1000 + i),
                ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen,
                "%s (%zu shapes)", label.c_str(), sk->getShapes().size());
            if (isActive || !sk->isVisible()) ImGui::PopStyleColor();

            // Click to select / toggle-off; right-click context menu (explicit ID —
            // BulletText has no ID of its own)
            if (ImGui::IsItemClicked() && !isActive) {
                // Toggle: clicking the already-selected sketch deselects it
                viewport.setSelectedSketch(isSel ? nullptr : sk);
            }
            if (ImGui::BeginPopupContextItem("##SketchCtx")) {
                if (ImGui::MenuItem(sk->isVisible() ? "Hide" : "Show"))
                    sk->setVisible(!sk->isVisible());
                if (ImGui::MenuItem("Delete")) {
                    scene->removeSketch(sk);
                    if (viewport.getSelectedSketch() == sk) viewport.setSelectedSketch(nullptr);
                    ImGui::EndPopup();
                    ImGui::PopID();
                    break; // Iterator invalidated
                }
                ImGui::EndPopup();
            }

            // Visibility eye toggle (after the context menu declaration)
            if (renderEyeToggle(sk->isVisible()))
                sk->setVisible(!sk->isVisible());

            ImGui::PopID();
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
