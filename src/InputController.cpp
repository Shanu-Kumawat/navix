#include "InputController.hpp"
#include "Canvas.hpp"

namespace Core {

InputController::InputController(Drawing::Canvas* canvas, SceneModel* model, ApplicationContext* context)
    : canvas(canvas), model(model), context(context) {}

void InputController::handleInput() {
    // Delegate to Canvas for now
    canvas->handleInput();
}

void InputController::handleSelection(const ImVec2& mousePos) {
    canvas->handleSelection(mousePos);
}

void InputController::handleDrawing(const ImVec2& mousePos) {
    canvas->handleDrawing(mousePos);
}

void InputController::handleCurveManipulation(const ImVec2& mousePos) {
    canvas->handleCurveManipulation(mousePos);
}

void InputController::deleteSelectedShape() {
    canvas->deleteSelectedShape();
}

void InputController::duplicateSelectedShape() {
    canvas->duplicateSelectedShape();
}

void InputController::moveSelectedShape(const ImVec2& delta) {
    canvas->moveSelectedShape(delta);
}

void InputController::rotateSelectedShape(float angle) {
    canvas->rotateSelectedShape(angle);
}

void InputController::scaleSelectedShape(float factor) {
    canvas->scaleSelectedShape(factor);
}

void InputController::handlePointDrawing(const ImVec2& currentPos) {}
void InputController::handleLineDrawing(const ImVec2& currentPos) {}
void InputController::handleCircleDrawing(const ImVec2& currentPos) {}
void InputController::handleTriangleDrawing(const ImVec2& currentPos) {}
void InputController::handleSquareDrawing(const ImVec2& currentPos) {}
void InputController::handleRectangleDrawing(const ImVec2& currentPos) {}
void InputController::handleSplineDrawing(const ImVec2& currentPos) {}
void InputController::handleBezierDrawing(const ImVec2& currentPos) {}
void InputController::handleBellowsDrawing(const ImVec2& currentPos) {}
void InputController::handleBallBearingDrawing(const ImVec2& currentPos) {}
void InputController::handleSpring2DDrawing(const ImVec2& currentPos) {}

} // namespace Core
