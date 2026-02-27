#pragma once

#include <imgui.h>
#include <vector>
#include <array>
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
 * It takes the SceneModel and the current view state (from Canvas) to draw
 * the grid, shapes, control points, and drawing previews.
 */
class Renderer2D {
public:
    Renderer2D(Drawing::Canvas* canvas);
    ~Renderer2D() = default;

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
};

} // namespace Core
