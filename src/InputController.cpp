#include <glm/glm.hpp>
#include "InputController.hpp"
#include "Canvas.hpp"
#include "SceneModel.hpp"
#include "Renderer2D.hpp"
#include "ApplicationContext.hpp"
#include "utils/MathUtils.hpp"
#include "shapes/ShockAbsorberBottomEnd.hpp"
#include <iostream>
#include <cmath>
#include <memory>

namespace Core {

using Drawing::Math::calculateDistance;
using Drawing::Math::toDVec2;

InputController::InputController(Drawing::Canvas* canvas, SceneModel* model, Renderer2D* renderer, ApplicationContext* context)
    : canvas(canvas), model(model), renderer(renderer), context(context) {}

glm::dvec2 InputController::normalizeVec(const glm::dvec2& vec) {
    float length = std::sqrt(vec.x * vec.x + vec.y * vec.y);
    if (length < 0.0001f) return glm::dvec2(1.0f, 0.0f);
    return glm::dvec2(vec.x / length, vec.y / length);
}

void InputController::setDrawingMode(Drawing::DrawingMode mode) {
    isDrawing = (mode == Drawing::DrawingMode::Spring2D || mode == Drawing::DrawingMode::Bellows);
    isFirstClick = true;
    clickCount = 0;
    currentSplinePoints.clear();
    currentCurvePoints.clear();
}

void InputController::resetDrawingState() {
    isDrawing = false;
    isFirstClick = true;
    clickCount = 0;
    isDraggingCanvas = false;
    currentSplinePoints.clear();
    currentCurvePoints.clear();
}

// ============================================================================
// Main Input Dispatch
// ============================================================================

void InputController::handleInput() {
    const ImGuiIO& io = ImGui::GetIO();

    if (!ImGui::IsWindowHovered())
        return;

    glm::dvec2 mousePos = toDVec2(ImGui::GetMousePos());
    glm::dvec2 transformedMousePos = canvas->inverseTransformCoordinates(mousePos);

    // Zoom
    if (std::abs(io.MouseWheel) > 0.01f) {
        canvas->updateZoom(io.MouseWheel);
    }

    // Pan
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
        canvas->updatePan(toDVec2(io.MouseDelta));
        isDraggingCanvas = true;
    } else {
        isDraggingCanvas = false;
    }

    // Drawing or selection
    if (!isDraggingCanvas) {
        if (canvas->getCurrentMode() == Drawing::DrawingMode::Select) {
            handleSelection(transformedMousePos);
        } else {
            handleDrawing(transformedMousePos);
        }
    }
}

void InputController::handleKeyboardShortcuts() {
    if (ImGui::GetIO().KeyCtrl) {
        if (ImGui::IsKeyPressed(ImGuiKey_Z)) {
            canvas->undo();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Y)) {
            canvas->redo();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_D) && canvas->getSelectedShape()) {
            canvas->duplicateSelectedShape();
        }
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Delete) && canvas->getSelectedShape()) {
        canvas->deleteSelectedShape();
    }
}

// ============================================================================
// Selection
// ============================================================================

void InputController::handleSelection(const glm::dvec2& mousePos) {
    auto& shapes = model->getShapesMutable();

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        canvas->clearSelection();

        std::cout << "Selection click at: (" << mousePos.x << ", " << mousePos.y << ")\n";

        float snapThreshold = Drawing::Constants::SNAP_THRESHOLD / canvas->getZoomLevel();

        // Check bellows first
        bool bellowsSelected = false;
        for (auto it = shapes.rbegin(); it != shapes.rend(); ++it) {
            auto& shape = *it;
            if (shape->type == Drawing::ShapeType::BELLOWS) {
                if (shape->isPointNear(mousePos, snapThreshold)) {
                    canvas->selectShape(shape.get());
                    static_cast<Drawing::Bellows*>(canvas->getSelectedShape())->isSelected = true;
                    bellowsSelected = true;
                    break;
                }
            }
        }

        if (!bellowsSelected) {
            // Check Spring2D
            for (auto it = shapes.rbegin(); it != shapes.rend(); ++it) {
                auto& shape = *it;
                if (shape->type == Drawing::ShapeType::SPRING2D) {
                    auto* spring = static_cast<Drawing::Spring2D*>(shape.get());
                    if (spring->isPointInBoundingBox(mousePos, snapThreshold)) {
                        canvas->selectShape(shape.get());
                        std::cout << "Spring2D selected at (" << spring->centerX << ", " << spring->centerY << ")!\n";
                        break;
                    }
                }
            }

            // Check other shapes
            if (!canvas->getSelectedShape()) {
                for (auto it = shapes.rbegin(); it != shapes.rend(); ++it) {
                    auto& shape = *it;
                    if (shape->type == Drawing::ShapeType::SPRING2D) continue;
                    if (shape->isPointNear(mousePos, snapThreshold)) {
                        canvas->selectShape(shape.get());
                        std::cout << "Shape selected, type: " << static_cast<int>(shape->type) << "\n";
                        if (canvas->getSelectedShape()->type == Drawing::ShapeType::BELLOWS) {
                            static_cast<Drawing::Bellows*>(canvas->getSelectedShape())->isSelected = true;
                        } else if (canvas->getSelectedShape()->type == Drawing::ShapeType::BALL_BEARING) {
                            static_cast<Drawing::BallBearing*>(canvas->getSelectedShape())->isSelected = true;
                        }
                        break;
                    }
                }
            }
        }

        // Initialize selected point for curves
        auto* selectedShape = canvas->getSelectedShape();
        if (selectedShape) {
            if (selectedShape->type == Drawing::ShapeType::SPLINE) {
                auto* spline = static_cast<Drawing::Spline*>(selectedShape);
                spline->selectedPoint = -1;
                float minDist = snapThreshold;
                for (size_t i = 0; i < spline->controlPoints.size(); ++i) {
                    float dist = calculateDistance(mousePos, spline->controlPoints[i]);
                    if (dist < minDist) {
                        minDist = dist;
                        spline->selectedPoint = static_cast<int>(i);
                    }
                }
            } else if (selectedShape->type == Drawing::ShapeType::BEZIER) {
                auto* bezier = static_cast<Drawing::BezierCurve*>(selectedShape);
                bezier->selectedPoint = -1;
                float minDist = snapThreshold;
                for (size_t i = 0; i < bezier->controlPoints.size(); ++i) {
                    float dist = calculateDistance(mousePos, bezier->controlPoints[i]);
                    if (dist < minDist) {
                        minDist = dist;
                        bezier->selectedPoint = static_cast<int>(i);
                    }
                }
            }
        }
    }

    // Handle manipulation of selected shape
    if (canvas->getSelectedShape()) {
        handleCurveManipulation(mousePos);
    }
}

// ============================================================================
// Drawing Dispatch
// ============================================================================

void InputController::handleDrawing(const glm::dvec2& mousePos) {
    if (isDraggingCanvas) return;

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    switch (canvas->getCurrentMode()) {
        case Drawing::DrawingMode::Point:
            handlePointDrawing(drawList, mousePos);
            break;
        case Drawing::DrawingMode::Line:
            handleLineDrawing(drawList, mousePos);
            break;
        case Drawing::DrawingMode::Circle:
            handleCircleDrawing(drawList, mousePos);
            break;
        case Drawing::DrawingMode::Triangle:
            handleTriangleDrawing(drawList, mousePos);
            break;
        case Drawing::DrawingMode::Square:
            handleSquareDrawing(drawList, mousePos);
            break;
        case Drawing::DrawingMode::Rectangle:
            handleRectangleDrawing(drawList, mousePos);
            break;
        case Drawing::DrawingMode::Spline:
            handleSplineDrawing(drawList, mousePos);
            break;
        case Drawing::DrawingMode::BezierCurve:
            handleBezierDrawing(drawList, mousePos);
            break;
        case Drawing::DrawingMode::Bellows:
            handleBellowsDrawing(drawList, mousePos);
            break;
        case Drawing::DrawingMode::BallBearing:
            handleBallBearingDrawing(drawList, mousePos);
            break;
        case Drawing::DrawingMode::Spring2D:
            handleSpring2DDrawing(drawList, mousePos);
            break;
        default:
            break;
    }
}

// ============================================================================
// Curve Manipulation
// ============================================================================

void InputController::handleCurveManipulation(const glm::dvec2& mousePos) {
    auto* selectedShape = canvas->getSelectedShape();
    if (!selectedShape) return;

    if (selectedShape->type == Drawing::ShapeType::SPLINE) {
        auto* spline = static_cast<Drawing::Spline*>(selectedShape);

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            float minDist = Drawing::Constants::SNAP_THRESHOLD / canvas->getZoomLevel();
            spline->selectedPoint = -1;

            for (size_t i = 0; i < spline->controlPoints.size(); ++i) {
                float dist = calculateDistance(mousePos, spline->controlPoints[i]);
                if (dist < minDist) {
                    minDist = dist;
                    spline->selectedPoint = static_cast<int>(i);
                }
            }
        }

        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) &&
            spline->selectedPoint >= 0 &&
            spline->selectedPoint < static_cast<int>(spline->controlPoints.size())) {
            spline->moveControlPoint(spline->selectedPoint, mousePos);
        }

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            spline->selectedPoint = -1;
        }
    } else if (selectedShape->type == Drawing::ShapeType::BEZIER) {
        auto* bezier = static_cast<Drawing::BezierCurve*>(selectedShape);
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            if (bezier->selectedPoint >= 0 &&
                bezier->selectedPoint < static_cast<int>(bezier->controlPoints.size())) {
                if (ImGui::GetIO().KeyShift) {
                    bezier->adjustSymmetrically(bezier->selectedPoint, mousePos);
                } else {
                    bezier->moveControlPoint(bezier->selectedPoint, mousePos);
                }
            }
        }
    }
}

// ============================================================================
// Point Drawing
// ============================================================================

void InputController::handlePointDrawing(ImDrawList* drawList, const glm::dvec2& currentPos) {
    glm::dvec2 snappedPos = canvas->findNearestSnapPoint(currentPos);
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        canvas->addShapeWithCommand(std::make_unique<Drawing::Point>(snappedPos));
        std::cout << "Point added at (" << snappedPos.x << ", " << snappedPos.y << ")\n";
    }
}

// ============================================================================
// Line Drawing
// ============================================================================

void InputController::handleLineDrawing(ImDrawList* drawList, const glm::dvec2& currentPos) {
    glm::dvec2 snappedPos = canvas->findNearestSnapPoint(currentPos);

    if (auto snapPoint = canvas->findSnapPoint(currentPos)) {
        renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
    }

    if (canvas->isFixedLineLength()) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            if (isDrawing) {
                endPoint = snappedPos;
                glm::dvec2 direction = normalizeVec(glm::dvec2(endPoint.x - startPoint.x, endPoint.y - startPoint.y));
                float lineLen = canvas->getLineLength();
                glm::dvec2 actualEnd = glm::dvec2(
                    startPoint.x + direction.x * lineLen,
                    startPoint.y + direction.y * lineLen
                );
                canvas->addShapeWithCommand(std::make_unique<Drawing::Line>(startPoint, actualEnd));
                isDrawing = false;
            } else {
                startPoint = snappedPos;
                isDrawing = true;
                if (auto snapPoint = canvas->findSnapPoint(startPoint)) {
                    renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
                }
            }
        }

        if (isDrawing) {
            glm::dvec2 direction = normalizeVec(glm::dvec2(currentPos.x - startPoint.x, currentPos.y - startPoint.y));
            float lineLen = canvas->getLineLength();
            glm::dvec2 previewEnd = glm::dvec2(
                startPoint.x + direction.x * lineLen,
                startPoint.y + direction.y * lineLen
            );
            renderer->previewLine(drawList, startPoint, previewEnd);
            if (auto snapPoint = canvas->findSnapPoint(startPoint)) {
                renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
            }
        }
    } else {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            startPoint = snappedPos;
            isDrawing = true;
            if (auto snapPoint = canvas->findSnapPoint(startPoint)) {
                renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
            }
        }
        else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && isDrawing) {
            isDrawing = false;
            endPoint = snappedPos;
            float length = calculateDistance(startPoint, endPoint);
            if (length > Drawing::Constants::MIN_SHAPE_SIZE) {
                canvas->addShapeWithCommand(std::make_unique<Drawing::Line>(startPoint, endPoint));
            }
        }

        if (isDrawing) {
            renderer->previewLine(drawList, startPoint, snappedPos);
            if (auto snapPoint = canvas->findSnapPoint(startPoint)) {
                renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
            }
        }
    }
}

// ============================================================================
// Circle Drawing
// ============================================================================

void InputController::handleCircleDrawing(ImDrawList* drawList, const glm::dvec2& currentPos) {
    glm::dvec2 snappedPos = canvas->findNearestSnapPoint(currentPos);

    if (auto snapPoint = canvas->findSnapPoint(currentPos)) {
        renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
    }

    if (canvas->isFixedCircleRadius()) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            if (isDrawing) {
                endPoint = snappedPos;
                canvas->addShapeWithCommand(std::make_unique<Drawing::Circle>(startPoint, canvas->getCircleRadius()));
                isDrawing = false;
            } else {
                startPoint = snappedPos;
                isDrawing = true;
                if (auto snapPoint = canvas->findSnapPoint(startPoint)) {
                    renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
                }
            }
        }

        if (isDrawing) {
            renderer->previewCircle(drawList, startPoint, canvas->getCircleRadius());
            if (auto snapPoint = canvas->findSnapPoint(startPoint)) {
                renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
            }
        }
    } else {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            startPoint = snappedPos;
            isDrawing = true;
            if (auto snapPoint = canvas->findSnapPoint(startPoint)) {
                renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
            }
        }
        else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && isDrawing) {
            isDrawing = false;
            float radius = calculateDistance(startPoint, snappedPos);
            if (radius > Drawing::Constants::MIN_SHAPE_SIZE) {
                canvas->addShapeWithCommand(std::make_unique<Drawing::Circle>(startPoint, radius));
            }
        }

        if (isDrawing) {
            endPoint = snappedPos;
            float radius = calculateDistance(startPoint, endPoint);
            renderer->previewCircle(drawList, startPoint, radius);
            if (auto snapPoint = canvas->findSnapPoint(startPoint)) {
                renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
            }
        }
    }
}

// ============================================================================
// Triangle Drawing
// ============================================================================

void InputController::handleTriangleDrawing(ImDrawList* drawList, const glm::dvec2& currentPos) {
    glm::dvec2 snappedPos = canvas->findNearestSnapPoint(currentPos);

    if (auto snapPoint = canvas->findSnapPoint(currentPos)) {
        renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
    }

    if (!isDrawing) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            isDrawing = true;
            startPoint = snappedPos;
            clickCount = 1;
            if (auto snapPoint = canvas->findSnapPoint(startPoint)) {
                renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
            }
        }
    } else {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (canvas->isFixedTriangleSize()) {
                if (clickCount == 1) {
                    glm::dvec2 direction = glm::dvec2(snappedPos.x - startPoint.x, snappedPos.y - startPoint.y);
                    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                    if (length > 0.0f) {
                        direction = glm::dvec2(direction.x / length, direction.y / length);
                        float side = canvas->getTriangleSide();
                        float height = side * std::sqrt(3.0f) / 2.0f;
                        glm::dvec2 p1 = startPoint;
                        glm::dvec2 p2 = glm::dvec2(startPoint.x + side * direction.x,
                                         startPoint.y + side * direction.y);
                        glm::dvec2 p3 = glm::dvec2(startPoint.x + side * 0.5f * direction.x - height * direction.y,
                                         startPoint.y + side * 0.5f * direction.y + height * direction.x);
                        std::array<glm::dvec2, 3> points = {p1, p2, p3};
                        canvas->addShapeWithCommand(std::make_unique<Drawing::Triangle>(points));
                        isDrawing = false;
                        clickCount = 0;
                    }
                }
            } else {
                glm::dvec2 direction = glm::dvec2(snappedPos.x - startPoint.x, snappedPos.y - startPoint.y);
                float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                if (length > 0.0f) {
                    direction = glm::dvec2(direction.x / length, direction.y / length);
                    float height = length * std::sqrt(3.0f) / 2.0f;
                    glm::dvec2 p1 = startPoint;
                    glm::dvec2 p2 = glm::dvec2(startPoint.x + length * direction.x,
                                     startPoint.y + length * direction.y);
                    glm::dvec2 p3 = glm::dvec2(startPoint.x + length * 0.5f * direction.x - height * direction.y,
                                     startPoint.y + length * 0.5f * direction.y + height * direction.x);
                    std::array<glm::dvec2, 3> previewPoints = {p1, p2, p3};
                    renderer->previewTriangle(drawList, previewPoints, 3);
                }
            }
        } else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            if (!canvas->isFixedTriangleSize()) {
                glm::dvec2 direction = glm::dvec2(snappedPos.x - startPoint.x, snappedPos.y - startPoint.y);
                float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                if (length > Drawing::Constants::MIN_SHAPE_SIZE) {
                    direction = glm::dvec2(direction.x / length, direction.y / length);
                    float height = length * std::sqrt(3.0f) / 2.0f;
                    glm::dvec2 p1 = startPoint;
                    glm::dvec2 p2 = glm::dvec2(startPoint.x + length * direction.x,
                                     startPoint.y + length * direction.y);
                    glm::dvec2 p3 = glm::dvec2(startPoint.x + length * 0.5f * direction.x - height * direction.y,
                                     startPoint.y + length * 0.5f * direction.y + height * direction.x);
                    std::array<glm::dvec2, 3> points = {p1, p2, p3};
                    canvas->addShapeWithCommand(std::make_unique<Drawing::Triangle>(points));
                }
                isDrawing = false;
            }
        }
    }
}

// ============================================================================
// Square Drawing
// ============================================================================

void InputController::handleSquareDrawing(ImDrawList* drawList, const glm::dvec2& currentPos) {
    glm::dvec2 snappedPos = canvas->findNearestSnapPoint(currentPos);

    if (auto snapPoint = canvas->findSnapPoint(currentPos)) {
        renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
    }

    if (!isDrawing) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            isDrawing = true;
            startPoint = snappedPos;
            clickCount = 1;
            if (auto snapPoint = canvas->findSnapPoint(startPoint)) {
                renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
            }
        }
    } else {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (canvas->isFixedSquareSize()) {
                if (clickCount == 1) {
                    glm::dvec2 direction = glm::dvec2(snappedPos.x - startPoint.x, snappedPos.y - startPoint.y);
                    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                    if (length > 0.0f) {
                        direction = glm::dvec2(direction.x / length, direction.y / length);
                        float sqSize = canvas->getSquareSize();
                        glm::dvec2 p1 = startPoint;
                        glm::dvec2 p2 = glm::dvec2(startPoint.x + sqSize * direction.x,
                                         startPoint.y + sqSize * direction.y);
                        canvas->addShapeWithCommand(std::make_unique<Drawing::Square>(p1, p2));
                        isDrawing = false;
                        clickCount = 0;
                    }
                }
            } else {
                float size = std::max(
                    std::abs(snappedPos.x - startPoint.x),
                    std::abs(snappedPos.y - startPoint.y)
                );
                renderer->previewSquare(drawList, startPoint, snappedPos);
            }
        } else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            if (!canvas->isFixedSquareSize()) {
                float size = std::max(
                    std::abs(snappedPos.x - startPoint.x),
                    std::abs(snappedPos.y - startPoint.y)
                );
                if (size > Drawing::Constants::MIN_SHAPE_SIZE) {
                    float signX = (snappedPos.x >= startPoint.x) ? 1.0f : -1.0f;
                    float signY = (snappedPos.y >= startPoint.y) ? 1.0f : -1.0f;
                    glm::dvec2 p1 = startPoint;
                    glm::dvec2 p2 = glm::dvec2(startPoint.x + size * signX, startPoint.y);
                    canvas->addShapeWithCommand(std::make_unique<Drawing::Square>(p1, p2));
                }
                isDrawing = false;
            }
        }
    }
}

// ============================================================================
// Rectangle Drawing
// ============================================================================

void InputController::handleRectangleDrawing(ImDrawList* drawList, const glm::dvec2& currentPos) {
    glm::dvec2 snappedPos = canvas->findNearestSnapPoint(currentPos);

    if (auto snapPoint = canvas->findSnapPoint(currentPos)) {
        renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
    }

    if (!isDrawing) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            isDrawing = true;
            startPoint = snappedPos;
            clickCount = 1;
            if (auto snapPoint = canvas->findSnapPoint(startPoint)) {
                renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
            }
        }
    } else {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (canvas->isFixedRectangleSize()) {
                if (clickCount == 1) {
                    glm::dvec2 direction = glm::dvec2(snappedPos.x - startPoint.x, snappedPos.y - startPoint.y);
                    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                    if (length > 0.0f) {
                        direction = glm::dvec2(direction.x / length, direction.y / length);
                        float rw = canvas->getRectangleWidth();
                        float rh = canvas->getRectangleHeight();
                        glm::dvec2 p1 = startPoint;
                        glm::dvec2 p2 = glm::dvec2(startPoint.x + rw * direction.x,
                                         startPoint.y + rw * direction.y);
                        glm::dvec2 p3 = glm::dvec2(p2.x - rh * direction.y,
                                         p2.y + rh * direction.x);
                        glm::dvec2 p4 = glm::dvec2(p1.x - rh * direction.y,
                                         p1.y + rh * direction.x);
                        canvas->addShapeWithCommand(std::make_unique<Drawing::Rectangle>(p1, p2, p3, p4));
                        isDrawing = false;
                        clickCount = 0;
                    }
                }
            } else {
                renderer->previewRectangle(drawList, startPoint, snappedPos);
            }
        } else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            if (!canvas->isFixedRectangleSize()) {
                float width = std::abs(snappedPos.x - startPoint.x);
                float height = std::abs(snappedPos.y - startPoint.y);
                if (width > Drawing::Constants::MIN_SHAPE_SIZE && height > Drawing::Constants::MIN_SHAPE_SIZE) {
                    float signX = (snappedPos.x >= startPoint.x) ? 1.0f : -1.0f;
                    float signY = (snappedPos.y >= startPoint.y) ? 1.0f : -1.0f;
                    glm::dvec2 p1 = startPoint;
                    glm::dvec2 p2 = glm::dvec2(startPoint.x + width * signX, startPoint.y);
                    glm::dvec2 p3 = glm::dvec2(startPoint.x + width * signX, startPoint.y + height * signY);
                    glm::dvec2 p4 = glm::dvec2(startPoint.x, startPoint.y + height * signY);
                    canvas->addShapeWithCommand(std::make_unique<Drawing::Rectangle>(p1, p2, p3, p4));
                }
                isDrawing = false;
            }
        }
    }
}

// ============================================================================
// Spline Drawing
// ============================================================================

void InputController::handleSplineDrawing(ImDrawList* drawList, const glm::dvec2& currentPos) {
    glm::dvec2 snappedPos = canvas->findNearestSnapPoint(currentPos);
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (!isDrawing) {
            isDrawing = true;
            currentSplinePoints.clear();
        }
        currentSplinePoints.push_back(snappedPos);
    }

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && currentSplinePoints.size() >= 2) {
        auto spline = std::make_unique<Drawing::Spline>(currentSplinePoints);
        spline->showControlPoints = true;
        canvas->addShapeWithCommand(std::move(spline));
        std::cout << "Spline added with " << currentSplinePoints.size() << " control points\n";
        isDrawing = false;
        currentSplinePoints.clear();
    }

    if (isDrawing && !currentSplinePoints.empty()) {
        renderer->previewSpline(drawList, currentSplinePoints);
    }
}

// ============================================================================
// Bezier Drawing
// ============================================================================

void InputController::handleBezierDrawing(ImDrawList* drawList, const glm::dvec2& currentPos) {
    glm::dvec2 snappedPos = canvas->findNearestSnapPoint(currentPos);
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (!isDrawing) {
            isDrawing = true;
            currentCurvePoints.clear();
        }
        currentCurvePoints.push_back(snappedPos);

        if (currentCurvePoints.size() == 4) {
            auto bezier = std::make_unique<Drawing::BezierCurve>(currentCurvePoints);
            canvas->addShapeWithCommand(std::move(bezier));
            std::cout << "Bezier curve added with 4 control points\n";
            isDrawing = false;
            currentCurvePoints.clear();
        }
    }

    if (isDrawing && !currentCurvePoints.empty()) {
        renderer->previewBezier(drawList, currentCurvePoints);
    }
}

// ============================================================================
// Bellows Drawing
// ============================================================================

void InputController::handleBellowsDrawing(ImDrawList* drawList, const glm::dvec2& currentPos) {
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (isFirstClick) {
            startPoint = canvas->getSnappedPoint(currentPos);
            isFirstClick = false;
            isDrawing = true;
        } else {
            endPoint = canvas->getSnappedPoint(currentPos);
            float dx = endPoint.x - startPoint.x;
            float dy = endPoint.y - startPoint.y;
            float length = std::sqrt(dx * dx + dy * dy);
            auto bellows = std::make_unique<Drawing::Bellows>();
            float minLength = bellows->cuffALength + bellows->cuffBLength + 10.0f;
            if (length > minLength) {
                bellows->convolutedSectionLength = length - bellows->cuffALength - bellows->cuffBLength;
                bellows->invalidateCache();
                bellows->position = startPoint;
                bellows->angle = std::atan2(dy, dx);

                canvas->selectShape(bellows.get());
                canvas->addShapeWithCommand(std::move(bellows));
                isFirstClick = true;
                isDrawing = false;
            }
        }
    }

    if (isDrawing && !isFirstClick) {
        glm::dvec2 snappedEnd = canvas->getSnappedPoint(currentPos);
        renderer->previewBellows(drawList, startPoint, snappedEnd);
    }
}

// ============================================================================
// Ball Bearing Drawing
// ============================================================================

void InputController::handleBallBearingDrawing(ImDrawList* drawList, const glm::dvec2& currentPos) {
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (isFirstClick) {
            startPoint = canvas->getSnappedPoint(currentPos);
            isFirstClick = false;
            isDrawing = true;
        } else {
            endPoint = canvas->getSnappedPoint(currentPos);
            float dx = endPoint.x - startPoint.x;
            float dy = endPoint.y - startPoint.y;
            float radius = std::sqrt(dx * dx + dy * dy);

            if (radius > 5.0f) {
                auto ballBearing = std::make_unique<Drawing::BallBearing>();
                ballBearing->position = startPoint;
                ballBearing->outerDiameter = radius * 2.0f;
                ballBearing->innerDiameter = radius * 1.2f;
                ballBearing->width = radius * 0.4f;
                ballBearing->ballDiameter = (ballBearing->outerDiameter - ballBearing->innerDiameter) / 4.0f;

                canvas->selectShape(ballBearing.get());
                canvas->addShapeWithCommand(std::move(ballBearing));
            }

            isFirstClick = true;
            isDrawing = false;
        }
    }

    if (isDrawing && !isFirstClick) {
        float dx = currentPos.x - startPoint.x;
        float dy = currentPos.y - startPoint.y;
        float radius = std::sqrt(dx * dx + dy * dy);
        renderer->previewBallBearing(drawList, startPoint, radius);
    }
}

// ============================================================================
// Spring2D Drawing
// ============================================================================

void InputController::handleSpring2DDrawing(ImDrawList* drawList, const glm::dvec2& mousePos) {
    glm::dvec2 snappedPos = canvas->findNearestSnapPoint(mousePos);

    if (!isDrawing) {
        isDrawing = true;
    }

    if (isDrawing) {
        renderer->previewSpring2D(drawList, snappedPos);

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            std::cout << "Placing Spring2D at: " << snappedPos.x << ", " << snappedPos.y << std::endl;
            canvas->addShapeWithCommand(std::make_unique<Drawing::Spring2D>(
                snappedPos.x, snappedPos.y,
                canvas->springOuterDiameter, canvas->springWireDiameter,
                canvas->springFreeLength, canvas->springNumCoils,
                IM_COL32(80, 80, 80, 255)
            ));
            isDrawing = false;
        }
    }
}

} // namespace Core
