#include <glm/glm.hpp>
#pragma once

#include <imgui.h>
#include <vector>
#include <array>
#include "Constants.hpp"

struct ImDrawList;

namespace Drawing {
class Canvas;
}

namespace Core {
class SceneModel;
class Renderer2D;
class ApplicationContext;

/**
 * @brief The Controller in the MVC architecture for the Canvas.
 *
 * Processes all mouse and keyboard input, manages drawing tool state,
 * handles shape selection and curve manipulation. Delegates shape storage
 * to SceneModel and visual previews to Renderer2D.
 */
class InputController {
public:
    InputController(Drawing::Canvas* canvas, SceneModel* model, Renderer2D* renderer, ApplicationContext* context);
    ~InputController() = default;

    // Main input entry points
    void handleInput();
    void handleKeyboardShortcuts();

    // Specific input handlers
    void handleSelection(const glm::dvec2& mousePos);
    void handleDrawing(const glm::dvec2& mousePos);
    void handleCurveManipulation(const glm::dvec2& mousePos);

    // Drawing mode management
    void setDrawingMode(Drawing::DrawingMode mode);
    void resetDrawingState();

    // Drawing state accessors (Canvas forwards these for Renderer2D)
    bool getIsDrawing() const { return isDrawing; }
    int getClickCount() const { return clickCount; }
    glm::dvec2 getStartPoint() const { return startPoint; }
    const std::array<glm::dvec2, 3>& getTrianglePoints() const { return trianglePoints; }
    const std::vector<glm::dvec2>& getCurrentSplinePoints() const { return currentSplinePoints; }
    const std::vector<glm::dvec2>& getCurrentCurvePoints() const { return currentCurvePoints; }

private:
    Drawing::Canvas* canvas;
    SceneModel* model;
    Renderer2D* renderer;
    ApplicationContext* context;

    // Drawing state
    bool isDrawing{false};
    bool isFirstClick{true};
    int clickCount{0};
    bool isDraggingCanvas{false};

    // Temporary points during drawing
    glm::dvec2 startPoint{0.0, 0.0};
    glm::dvec2 endPoint{0.0, 0.0};
    std::array<glm::dvec2, 3> trianglePoints{};
    std::vector<glm::dvec2> currentSplinePoints;
    std::vector<glm::dvec2> currentCurvePoints;

    // Helper
    static glm::dvec2 normalizeVec(const glm::dvec2& vec);

    // Drawing handlers (one per shape type)
    void handlePointDrawing(ImDrawList* drawList, const glm::dvec2& currentPos);
    void handleLineDrawing(ImDrawList* drawList, const glm::dvec2& currentPos);
    void handleCircleDrawing(ImDrawList* drawList, const glm::dvec2& currentPos);
    void handleTriangleDrawing(ImDrawList* drawList, const glm::dvec2& currentPos);
    void handleSquareDrawing(ImDrawList* drawList, const glm::dvec2& currentPos);
    void handleRectangleDrawing(ImDrawList* drawList, const glm::dvec2& currentPos);
    void handleSplineDrawing(ImDrawList* drawList, const glm::dvec2& currentPos);
    void handleBezierDrawing(ImDrawList* drawList, const glm::dvec2& currentPos);
    void handleBellowsDrawing(ImDrawList* drawList, const glm::dvec2& currentPos);
    void handleBallBearingDrawing(ImDrawList* drawList, const glm::dvec2& currentPos);
    void handleSpring2DDrawing(ImDrawList* drawList, const glm::dvec2& currentPos);
};

} // namespace Core
