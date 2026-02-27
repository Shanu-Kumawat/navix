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
    void handleSelection(const ImVec2& mousePos);
    void handleDrawing(const ImVec2& mousePos);
    void handleCurveManipulation(const ImVec2& mousePos);

    // Shape manipulation
    void deleteSelectedShape();
    void duplicateSelectedShape();
    void moveSelectedShape(const ImVec2& delta);
    void rotateSelectedShape(float angle);
    void scaleSelectedShape(float factor);

private:
    Drawing::Canvas* canvas;
    SceneModel* model;
    ApplicationContext* context;

    // Drawing handlers
    void handlePointDrawing(const ImVec2& currentPos);
    void handleLineDrawing(const ImVec2& currentPos);
    void handleCircleDrawing(const ImVec2& currentPos);
    void handleTriangleDrawing(const ImVec2& currentPos);
    void handleSquareDrawing(const ImVec2& currentPos);
    void handleRectangleDrawing(const ImVec2& currentPos);
    void handleSplineDrawing(const ImVec2& currentPos);
    void handleBezierDrawing(const ImVec2& currentPos);
    void handleBellowsDrawing(const ImVec2& currentPos);
    void handleBallBearingDrawing(const ImVec2& currentPos);
    void handleSpring2DDrawing(const ImVec2& currentPos);
};

} // namespace Core
