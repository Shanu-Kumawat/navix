#pragma once

#include "Renderer.hpp"
#include <imgui.h>

namespace Core {
namespace Graphics {

/**
 * @brief Concrete implementation of Renderer using ImGui/ImDrawList backend
 */
class ImGuiRenderer : public Renderer {
public:
    ImGuiRenderer();
    ~ImGuiRenderer() override = default;

    // Call this every frame before issuing draw commands
    void beginFrame(ImDrawList* drawList);

    void setTransform(const glm::dvec2& pan, float zoom, const glm::dvec2& windowPos) override;
    void drawLine(const glm::dvec2& start, const glm::dvec2& end, uint32_t color, float thickness) override;
    void drawDashedLine(const glm::dvec2& start, const glm::dvec2& end, uint32_t color, float thickness, float dashLength) override;
    void drawCircle(const glm::dvec2& center, float radius, uint32_t color, float thickness, bool fill = false) override;
    void drawPolygon(const std::vector<glm::dvec2>& points, uint32_t color, float thickness, bool fill = false) override;
    void drawText(const glm::dvec2& pos, const char* text, uint32_t color) override;
    void drawShape(const Drawing::Shape* shape, bool isSelected) override;
    
    void drawNode(const Topology::Node* node, bool isSelected = false) override;
    void drawEdge(const Topology::Edge* edge, const Topology::Node* n1, const Topology::Node* n2, bool isSelected = false) override;
    
    void drawMesh(const Core::Meshing::Mesh& mesh, const glm::dvec3& color) override;

private:
    ImVec2 toScreen(const glm::dvec2& worldPt) const;

    ImDrawList* currentDrawList;
    glm::dvec2 currentPan;
    float currentZoom;
    glm::dvec2 currentWindowPos;
};

} // namespace Graphics
} // namespace Core
