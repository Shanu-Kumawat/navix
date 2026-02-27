#include "core/ImGuiRenderer.hpp"
#include "topology/Node.hpp"
#include "topology/Edge.hpp"
#include "shapes/BasicShapes.hpp"

namespace Core {
namespace Graphics {

ImGuiRenderer::ImGuiRenderer() 
    : currentDrawList(nullptr), currentZoom(1.0f) {}

void ImGuiRenderer::beginFrame(ImDrawList* drawList) {
    currentDrawList = drawList;
}

void ImGuiRenderer::setTransform(const glm::dvec2& pan, float zoom, const glm::dvec2& windowPos) {
    currentPan = pan;
    currentZoom = zoom;
    currentWindowPos = windowPos;
}

ImVec2 ImGuiRenderer::toScreen(const glm::dvec2& worldPt) const {
    return ImVec2(
        static_cast<float>(currentWindowPos.x + currentPan.x + worldPt.x * currentZoom),
        static_cast<float>(currentWindowPos.y + currentPan.y + worldPt.y * currentZoom)
    );
}

void ImGuiRenderer::drawLine(const glm::dvec2& start, const glm::dvec2& end, uint32_t color, float thickness) {
    if (!currentDrawList) return;
    currentDrawList->AddLine(toScreen(start), toScreen(end), color, thickness);
}

void ImGuiRenderer::drawDashedLine(const glm::dvec2& start, const glm::dvec2& end, uint32_t color, float thickness, float dashLength) {
    if (!currentDrawList) return;
    
    // Very basic ad-hoc dash rendering for now.
    // Needs proper direction calculation. Using standard AddLine as fallback for now.
    currentDrawList->AddLine(toScreen(start), toScreen(end), color, thickness);
}

void ImGuiRenderer::drawCircle(const glm::dvec2& center, float radius, uint32_t color, float thickness, bool fill) {
    if (!currentDrawList) return;
    float screenRadius = radius * currentZoom;
    if (fill) {
        currentDrawList->AddCircleFilled(toScreen(center), screenRadius, color);
    } else {
        currentDrawList->AddCircle(toScreen(center), screenRadius, color, 0, thickness);
    }
}

void ImGuiRenderer::drawPolygon(const std::vector<glm::dvec2>& points, uint32_t color, float thickness, bool fill) {
    if (!currentDrawList || points.empty()) return;
    
    std::vector<ImVec2> screenPoints;
    screenPoints.reserve(points.size());
    for (const auto& pt : points) {
        screenPoints.push_back(toScreen(pt));
    }
    
    if (fill) {
        currentDrawList->AddConvexPolyFilled(screenPoints.data(), screenPoints.size(), color);
    } else {
        currentDrawList->AddPolyline(screenPoints.data(), screenPoints.size(), color, ImDrawFlags_Closed, thickness);
    }
}

void ImGuiRenderer::drawText(const glm::dvec2& pos, const char* text, uint32_t color) {
    if (!currentDrawList) return;
    currentDrawList->AddText(toScreen(pos), color, text);
}

void ImGuiRenderer::drawShape(const Drawing::Shape* shape, bool isSelected) {
    // Basic implementation bridging the gap
    // In a fully decoupled model, shapes wouldn't know how to render themselves
    // Right now shapes just hold attributes
}

void ImGuiRenderer::drawNode(const Topology::Node* node, bool isSelected) {
    if (!node) return;
    uint32_t color = isSelected ? IM_COL32(255, 100, 100, 255) : IM_COL32(0, 255, 0, 255);
    drawCircle(node->getPosition(), 4.0f / currentZoom, color, 1.0f, true);
}

void ImGuiRenderer::drawEdge(const Topology::Edge* edge, const Topology::Node* n1, const Topology::Node* n2, bool isSelected) {
    if (!edge || !n1 || !n2) return;
    uint32_t color = isSelected ? IM_COL32(255, 255, 0, 255) : IM_COL32(0, 255, 255, 255);
    drawLine(glm::dvec2(n1->getPosition().x, n1->getPosition().y),
             glm::dvec2(n2->getPosition().x, n2->getPosition().y), 
             color, 2.0f);
}

} // namespace Graphics
} // namespace Core
