#pragma once

#include <imgui.h>
#include <vector>
#include <array>
#include "core/Renderer.hpp"
#include "SceneModel.hpp"
#include "shapes/BasicShapes.hpp"
#include "shapes/ComplexShapes.hpp"

namespace Drawing {
    class Canvas; // Forward declaration
}

namespace Core {

/**
 * @brief The View in the MVC architecture for the Canvas.
 * 
 * Renderer2D is responsible for all ImGui/ImDrawList rendering logic.
 * It inherits from Core::Renderer so it can be used polymorphically
 * wherever a Renderer is expected (e.g. drawLine, drawCircle primitives).
 * It also provides higher-level render methods for shapes, grids, and previews.
 */
class Renderer2D : public Renderer {
public:
    Renderer2D(Drawing::Canvas* canvas);
    ~Renderer2D() override = default;

    // ── Core::Renderer interface ────────────────────────────────────────
    void setTransform(const glm::dvec2& pan, float zoom, const glm::dvec2& windowPos) override;
    void drawLine(const glm::dvec2& start, const glm::dvec2& end, uint32_t color, float thickness) override;
    void drawDashedLine(const glm::dvec2& start, const glm::dvec2& end, uint32_t color, float thickness, float dashLength) override;
    void drawCircle(const glm::dvec2& center, float radius, uint32_t color, float thickness, bool fill = false) override;
    void drawPolygon(const std::vector<glm::dvec2>& points, uint32_t color, float thickness, bool fill = false) override;
    void drawText(const glm::dvec2& pos, const char* text, uint32_t color) override;
    void drawShape(const Drawing::Shape* shape, bool isSelected) override;
    void drawNode(const Topology::Node* node, bool isSelected = false) override;
    void drawEdge(const Topology::Edge* edge, const Topology::Node* n1, const Topology::Node* n2, bool isSelected = false) override;
    void drawMesh(const Meshing::Mesh& mesh, const glm::dvec3& color) override;

    // Set the ImDrawList for the current frame (call before issuing draw commands)
    void beginFrame(ImDrawList* drawList);

    // ── High-level render API (existing) ────────────────────────────────
    // Main render entry point
    void render(ImDrawList* drawList, const SceneModel& model);

    // Grid rendering
    void renderGrid(ImDrawList* drawList);

    // Shape rendering
    void renderShapes(ImDrawList* drawList, const std::vector<std::unique_ptr<Drawing::Shape>>& shapes);
    void renderShape(ImDrawList* drawList, const Drawing::Shape* shape, bool isSelected);

    // Specific shape renderers
    void renderBellows(ImDrawList* drawList, const Drawing::Bellows* bellows, bool isSelected);
    void renderSprings2D(ImDrawList* drawList, const Drawing::Spring2D* spring, bool isSelected);
    void renderBallBearings(ImDrawList* drawList, const Drawing::BallBearing* bearing, bool isSelected);

    // Preview rendering (during drawing)
    void renderPreview(ImDrawList* drawList, const glm::dvec2& currentPos, Drawing::DrawingMode mode);
    
    // Specific preview methods
    void previewPoint(ImDrawList* drawList, const glm::dvec2& pos);
    void previewLine(ImDrawList* drawList, const glm::dvec2& start, const glm::dvec2& end);
    void previewCircle(ImDrawList* drawList, const glm::dvec2& center, float radius);
    void previewTriangle(ImDrawList* drawList, const std::array<glm::dvec2, 3>& points, int count);
    void previewSquare(ImDrawList* drawList, const glm::dvec2& start, const glm::dvec2& end);
    void previewRectangle(ImDrawList* drawList, const glm::dvec2& start, const glm::dvec2& end);
    void previewSpline(ImDrawList* drawList, const std::vector<glm::dvec2>& points);
    void previewBezier(ImDrawList* drawList, const std::vector<glm::dvec2>& points);
    void previewBellows(ImDrawList* drawList, const glm::dvec2& start, const glm::dvec2& end);
    void previewBallBearing(ImDrawList* drawList, const glm::dvec2& center, float radius);
    void previewSpring2D(ImDrawList* drawList, const glm::dvec2& center);

    // Helpers
    void drawDashedLine(ImDrawList* drawList, const ImVec2& p1, const ImVec2& p2, 
                       ImU32 color, float thickness, float dash_length);
    void renderSnapIndicator(ImDrawList* drawList, const glm::dvec2& pos, const std::string& type);

private:
    Drawing::Canvas* canvas; // Reference to canvas for view state (zoom, pan, transforms)
    
    // Stored per-frame state for Core::Renderer interface
    ImDrawList* currentDrawList{nullptr};
    glm::dvec2 currentPan{0.0, 0.0};
    float currentZoom{1.0f};
    glm::dvec2 currentWindowPos{0.0, 0.0};
    
    ImVec2 toScreen(const glm::dvec2& worldPt) const;
};

} // namespace Core
