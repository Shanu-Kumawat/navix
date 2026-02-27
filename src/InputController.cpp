#include <glm/glm.hpp>
#include "InputController.hpp"
#include "Canvas.hpp"

namespace Core {

InputController::InputController(Drawing::Canvas* canvas, SceneModel* model, ApplicationContext* context)
    : canvas(canvas), model(model), context(context) {}

void InputController::handleInput() {
    // Delegate to Canvas for now
    canvas->handleInput();
}

void InputController::handleSelection(const glm::dvec2& mousePos) {
    canvas->handleSelection(mousePos);
}

void InputController::handleDrawing(const glm::dvec2& mousePos) {
    canvas->handleDrawing(mousePos);
}

void InputController::handleCurveManipulation(const glm::dvec2& mousePos) {
    canvas->handleCurveManipulation(mousePos);
}

void InputController::deleteSelectedShape() {
    canvas->deleteSelectedShape();
}

void InputController::duplicateSelectedShape() {
    canvas->duplicateSelectedShape();
}

void InputController::moveSelectedShape(const glm::dvec2& delta) {
    canvas->moveSelectedShape(delta);
}

void InputController::rotateSelectedShape(float angle) {
    canvas->rotateSelectedShape(angle);
}

void InputController::scaleSelectedShape(float factor) {
    canvas->scaleSelectedShape(factor);
}

void InputController::handlePointDrawing(const glm::dvec2& currentPos) {}
void InputController::handleLineDrawing(const glm::dvec2& currentPos) {}
void InputController::handleCircleDrawing(const glm::dvec2& currentPos) {}
void InputController::handleTriangleDrawing(const glm::dvec2& currentPos) {}
void InputController::handleSquareDrawing(const glm::dvec2& currentPos) {}
void InputController::handleRectangleDrawing(const glm::dvec2& currentPos) {}
void InputController::handleSplineDrawing(const glm::dvec2& currentPos) {}
void InputController::handleBezierDrawing(const glm::dvec2& currentPos) {}
void InputController::handleBellowsDrawing(const glm::dvec2& currentPos) {}
void InputController::handleBallBearingDrawing(const glm::dvec2& currentPos) {}
void InputController::handleSpring2DDrawing(const glm::dvec2& currentPos) {}

} // namespace Core
