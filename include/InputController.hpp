#include <glm/glm.hpp>
#pragma once

#include <imgui.h>
#include "SceneModel.hpp"
class ApplicationContext;

namespace Drawing {
    class Canvas; // Forward declaration
}

namespace Core {

/**
 * @brief The Controller in the MVC architecture for the Canvas.
 * 
 * InputController is responsible for processing mouse and keyboard input,
 * and updating the SceneModel accordingly. It handles drawing logic,
 * selection logic, and curve manipulation.
 */
class InputController {
public:
    InputController(Drawing::Canvas* canvas, SceneModel* model, ApplicationContext* context);
    ~InputController() = default;

    // Main input entry point
    void handleInput();

    // Specific input handlers
    void handleSelection(const glm::dvec2& mousePos);
    void handleDrawing(const glm::dvec2& mousePos);
    void handleCurveManipulation(const glm::dvec2& mousePos);

    // Shape manipulation
    void deleteSelectedShape();
    void duplicateSelectedShape();
    void moveSelectedShape(const glm::dvec2& delta);
    void rotateSelectedShape(float angle);
    void scaleSelectedShape(float factor);

private:
    Drawing::Canvas* canvas;
    SceneModel* model;
    ApplicationContext* context;

    // Drawing handlers
    void handlePointDrawing(const glm::dvec2& currentPos);
    void handleLineDrawing(const glm::dvec2& currentPos);
    void handleCircleDrawing(const glm::dvec2& currentPos);
    void handleTriangleDrawing(const glm::dvec2& currentPos);
    void handleSquareDrawing(const glm::dvec2& currentPos);
    void handleRectangleDrawing(const glm::dvec2& currentPos);
    void handleSplineDrawing(const glm::dvec2& currentPos);
    void handleBezierDrawing(const glm::dvec2& currentPos);
    void handleBellowsDrawing(const glm::dvec2& currentPos);
    void handleBallBearingDrawing(const glm::dvec2& currentPos);
    void handleSpring2DDrawing(const glm::dvec2& currentPos);
};

} // namespace Core
