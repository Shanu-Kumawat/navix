#include "Canvas.hpp"
#include "utils/MathUtils.hpp"
#include "Constants.hpp"

namespace Drawing {

using Math::calculateDistance;
using Math::calculateTriangleArea;
using Math::calculateMidpoint;
using Math::calculateAngle;
using Math::snapToGrid;
using Math::isPointInRect;
using Math::isPointNearPoint;
using Math::normalizeVector;
using Math::dotProduct;
using Math::rotatePoint;

// ... existing code ...

Canvas::Canvas() : 
    currentMode(DrawingMode::None),
    isDrawing(false),
    isFirstClick(true),
    clickCount(0),
    snapToGrid(true),
    zoomLevel(1.0f),
    panOffset(0.0f, 0.0f),
    gridSpacing(200.0f),
    showControlPoints(true),
    selectedShape(nullptr),
    currentHistoryIndex(0)
{
    history.push_back(HistoryState{}); // Initial empty state
}

void Canvas::setDrawingMode(DrawingMode mode) {
    currentMode = mode;
    isDrawing = false;
    isFirstClick = true;
    clickCount = 0;
    currentSplinePoints.clear();
    currentCurvePoints.clear();
}

void Canvas::clearAll() {
    saveToHistory();
    shapes.clear();
    selectedShape = nullptr;
    isDrawing = false;
    isFirstClick = true;
    clickCount = 0;
    currentSplinePoints.clear();
    currentCurvePoints.clear();
}

void Canvas::toggleSplineClosure() {
    if (selectedShape && selectedShape->type == ShapeType::SPLINE) {
        auto* spline = static_cast<Spline*>(selectedShape);
        spline->isClosed = !spline->isClosed;
    }
}

bool Canvas::isSplineClosed() const {
    if (selectedShape && selectedShape->type == ShapeType::SPLINE) {
        auto* spline = static_cast<const Spline*>(selectedShape);
        return spline->isClosed;
    }
    return false;
}

void Canvas::setTruncationPoints(float start, float end) {
    curveStartT = std::clamp(start, 0.0f, 1.0f);
    curveEndT = std::clamp(end, 0.0f, 1.0f);
    if (curveEndT < curveStartT) {
        std::swap(curveStartT, curveEndT);
    }
}

std::pair<float, float> Canvas::getTruncationPoints() const {
    return {curveStartT, curveEndT};
}

void Canvas::handleInput() {
    ImGuiIO& io = ImGui::GetIO();
    
    // Handle zooming
    if (io.MouseWheel != 0.0f && !io.KeyCtrl) {
        updateZoom(io.MouseWheel);
    }
    
    // Handle panning
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle) || 
        (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && io.KeyAlt)) {
        updatePan(io.MouseDelta);
    }
    
    // Get mouse position in canvas space
    ImVec2 mousePos = inverseTransformCoordinates(io.MousePos);
    
    // Handle drawing and shape manipulation
    if (!io.KeyAlt && !io.KeyCtrl) {
        if (currentMode == DrawingMode::Select) {
            handleSelection(mousePos);
        } else {
            handleDrawing(mousePos);
        }
    }
}

void Canvas::handleSelection(const ImVec2& mousePos) {
    // Handle selection of shapes
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        selectedShape = nullptr;
        for (auto& shape : shapes) {
            if (shape->isPointNear(mousePos, Constants::SNAP_THRESHOLD / zoomLevel)) {
                selectedShape = shape.get();
                break;
            }
        }
        
        // Initialize selected point for curves
        if (selectedShape) {
            if (selectedShape->type == ShapeType::SPLINE) {
                auto* spline = static_cast<Spline*>(selectedShape);
                spline->selectedPoint = -1;
                float minDist = Constants::SNAP_THRESHOLD / zoomLevel;
                for (size_t i = 0; i < spline->controlPoints.size(); ++i) {
                    float dist = calculateDistance(mousePos, spline->controlPoints[i]);
                    if (dist < minDist) {
                        minDist = dist;
                        spline->selectedPoint = static_cast<int>(i);
                    }
                }
            } else if (selectedShape->type == ShapeType::BEZIER) {
                auto* bezier = static_cast<BezierCurve*>(selectedShape);
                bezier->selectedPoint = -1;
                float minDist = Constants::SNAP_THRESHOLD / zoomLevel;
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
    if (selectedShape) {
        handleCurveManipulation(mousePos);
    }
}

void Canvas::render(ImDrawList* drawList) {
    // Render grid first
    if (snapToGrid) {
        renderGrid(drawList);
    }
    
    // Render all shapes
    renderShapes(drawList);
    
    // Render preview if drawing
    if (isDrawing) {
        ImVec2 mousePos = inverseTransformCoordinates(ImGui::GetMousePos());
        renderPreview(drawList, mousePos);
    }
    
    // Render selection indicators
    if (selectedShape) {
        // Render selection highlight
        switch (selectedShape->type) {
            case ShapeType::SPLINE: {
                auto* spline = static_cast<Spline*>(selectedShape);
                for (const auto& point : spline->controlPoints) {
                    CurveUI::drawControlPoint(drawList, transformCoordinates(point), true, 5.0f);
                }
                break;
            }
            case ShapeType::BEZIER: {
                auto* bezier = static_cast<BezierCurve*>(selectedShape);
                for (const auto& point : bezier->controlPoints) {
                    CurveUI::drawControlPoint(drawList, transformCoordinates(point), true, 5.0f);
                }
                break;
            }
            default:
                break;
        }
    }
}

void Canvas::handleDrawing(const ImVec2& mousePos) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    switch (currentMode) {
        case DrawingMode::Point:
            handlePointDrawing(drawList, mousePos);
            break;
        case DrawingMode::Line:
            handleLineDrawing(drawList, mousePos);
            break;
        case DrawingMode::Circle:
            handleCircleDrawing(drawList, mousePos);
            break;
        case DrawingMode::Triangle:
            handleTriangleDrawing(drawList, mousePos);
            break;
        case DrawingMode::Square:
            handleSquareDrawing(drawList, mousePos);
            break;
        case DrawingMode::Rectangle:
            handleRectangleDrawing(drawList, mousePos);
            break;
        case DrawingMode::Spline:
            handleSplineDrawing(drawList, mousePos);
            break;
        case DrawingMode::BezierCurve:
            handleBezierDrawing(drawList, mousePos);
            break;
        default:
            break;
    }
}

void Canvas::update(const ImVec2& mousePos) {
    if (currentMode == DrawingMode::Select) {
        updateSelectedShape(mousePos);
    } else if (isDrawing) {
        updateDrawingPreview(mousePos);
    }

    // Handle keyboard shortcuts
    if (ImGui::GetIO().KeyCtrl) {
        if (ImGui::IsKeyPressed(ImGuiKey_Z)) {
            undo();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Y)) {
            redo();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_D) && selectedShape) {
            duplicateSelectedShape();
        }
    }
    
    if (ImGui::IsKeyPressed(ImGuiKey_Delete) && selectedShape) {
        deleteSelectedShape();
    }
}

void Canvas::updateSelectedShape(const ImVec2& mousePos) {
    if (!selectedShape) return;

    if (selectedShape->type == ShapeType::SPLINE || selectedShape->type == ShapeType::BEZIER) {
        handleCurveManipulation(mousePos);
    }
}

void Canvas::updateDrawingPreview(const ImVec2& mousePos) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    handleDrawing(mousePos);
}

void Canvas::deleteSelectedShape() {
    if (!selectedShape) return;
    
    auto it = std::find_if(shapes.begin(), shapes.end(),
        [this](const std::unique_ptr<Shape>& shape) {
            return shape.get() == selectedShape;
        });
    
    if (it != shapes.end()) {
        saveToHistory();
        shapes.erase(it);
        selectedShape = nullptr;
    }
}

void Canvas::duplicateSelectedShape() {
    if (!selectedShape) return;
    
    saveToHistory();
    
    // Create a new shape based on the type of the selected shape
    std::unique_ptr<Shape> newShape;
    switch (selectedShape->type) {
        case ShapeType::POINT: {
            const auto& point = static_cast<const Point&>(*selectedShape);
            ImVec2 newPos = {point.position.x + 10.0f, point.position.y + 10.0f};
            newShape = std::make_unique<Point>(newPos, point.color, point.size);
            break;
        }
        case ShapeType::LINE: {
            const auto& line = static_cast<const Line&>(*selectedShape);
            ImVec2 offset = {10.0f, 10.0f};
            ImVec2 newStart = {line.start.x + offset.x, line.start.y + offset.y};
            ImVec2 newEnd = {line.end.x + offset.x, line.end.y + offset.y};
            newShape = std::make_unique<Line>(newStart, newEnd, line.color, line.thickness);
            break;
        }
        // ... similar cases for other shape types ...
    }
    
    if (newShape) {
        shapes.push_back(std::move(newShape));
        selectedShape = shapes.back().get();
    }
}

void Canvas::moveSelectedShape(const ImVec2& delta) {
    if (!selectedShape) return;
    
    saveToHistory();
    
    switch (selectedShape->type) {
        case ShapeType::SPLINE: {
            auto& spline = static_cast<Spline&>(*selectedShape);
            spline.moveEntireSpline(delta);
            break;
        }
        case ShapeType::BEZIER: {
            auto& bezier = static_cast<BezierCurve&>(*selectedShape);
            bezier.moveEntireCurve(delta);
            break;
        }
        // ... handle other shape types ...
    }
}

void Canvas::rotateSelectedShape(float angle) {
    if (!selectedShape) return;
    
    saveToHistory();
    
    // Implementation depends on shape type
    // For now, only implement for shapes that support rotation
}

void Canvas::scaleSelectedShape(float factor) {
    if (!selectedShape) return;
    
    saveToHistory();
    
    // Implementation depends on shape type
    // For now, only implement for shapes that support scaling
}

void Canvas::saveToHistory() {
    // Remove any redo history
    if (currentHistoryIndex < history.size() - 1) {
        history.erase(history.begin() + currentHistoryIndex + 1, history.end());
    }
    
    // Create new history state
    HistoryState state;
    state.panOffset = panOffset;
    state.zoomLevel = zoomLevel;
    
    // Deep copy shapes
    for (const auto& shape : shapes) {
        // Implementation of shape cloning would go here
        // For now, just store the original shape
        state.shapes.push_back(std::move(std::unique_ptr<Shape>(shape->clone())));
    }
    
    // Add to history
    history.push_back(std::move(state));
    if (history.size() > MAX_HISTORY_SIZE) {
        history.erase(history.begin());
    } else {
        currentHistoryIndex++;
    }
}

void Canvas::undo() {
    if (currentHistoryIndex > 0) {
        currentHistoryIndex--;
        restoreHistoryState(history[currentHistoryIndex]);
    }
}

void Canvas::redo() {
    if (currentHistoryIndex < history.size() - 1) {
        currentHistoryIndex++;
        restoreHistoryState(history[currentHistoryIndex]);
    }
}

void Canvas::restoreHistoryState(const HistoryState& state) {
    panOffset = state.panOffset;
    zoomLevel = state.zoomLevel;
    
    shapes.clear();
    for (const auto& shape : state.shapes) {
        shapes.push_back(std::move(std::unique_ptr<Shape>(shape->clone())));
    }
    
    // Update selected shape pointer if necessary
    selectedShape = nullptr;  // Reset selection when restoring history
}

void Canvas::updateZoom(float delta) {
    // Store old zoom level for calculations
    float oldZoom = zoomLevel;
    
    // Update zoom level with limits, reduced sensitivity by multiplying delta by 0.1
    zoomLevel = std::clamp(zoomLevel + delta * 0.1f, 0.1f, 10.0f);
    
    // Get the current mouse position in screen space
    ImVec2 mousePos = ImGui::GetMousePos();
    
    // Convert mouse position to canvas space (relative to canvas origin)
    ImVec2 mouseInCanvas = {
        (mousePos.x - windowX - panOffset.x) / oldZoom,
        (mousePos.y - windowY - panOffset.y) / oldZoom
    };
    
    // Calculate new pan offset to keep mouse position fixed in world space
    panOffset.x = mousePos.x - windowX - mouseInCanvas.x * zoomLevel;
    panOffset.y = mousePos.y - windowY - mouseInCanvas.y * zoomLevel;
}

void Canvas::updatePan(const ImVec2& delta) {
    panOffset.x += delta.x;
    panOffset.y += delta.y;
}

ImVec2 Canvas::transformCoordinates(const ImVec2& point) const {
    return ImVec2{
        point.x * zoomLevel + panOffset.x,
        point.y * zoomLevel + panOffset.y
    };
}

ImVec2 Canvas::inverseTransformCoordinates(const ImVec2& point) const {
    return ImVec2{
        (point.x - panOffset.x) / zoomLevel,
        (point.y - panOffset.y) / zoomLevel
    };
}

void Canvas::renderGrid(ImDrawList* drawList) {
    const float MIN_GRID_SPACING = 40.0f;
    float effectiveSpacing = gridSpacing * zoomLevel;
    
    // Adjust grid spacing based on zoom level
    while (effectiveSpacing < MIN_GRID_SPACING) {
        effectiveSpacing *= 2.0f;
    }
    
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();
    
    float startX = windowPos.x - fmodf(panOffset.x, effectiveSpacing);
    float startY = windowPos.y - fmodf(panOffset.y, effectiveSpacing);
    float endX = windowPos.x + windowSize.x;
    float endY = windowPos.y + windowSize.y;
    
    // Draw vertical lines
    for (float x = startX; x < endX; x += effectiveSpacing) {
        drawList->AddLine(
            ImVec2(x, windowPos.y),
            ImVec2(x, endY),
            Colors::GRID,
            1.0f
        );
    }
    
    // Draw horizontal lines
    for (float y = startY; y < endY; y += effectiveSpacing) {
        drawList->AddLine(
            ImVec2(windowPos.x, y),
            ImVec2(endX, y),
            Colors::GRID,
            1.0f
        );
    }
}

void Canvas::renderShapes(ImDrawList* drawList) const {
    for (const auto& shape : shapes) {
        drawShape(*shape);
    }
}

void Canvas::renderPreview(ImDrawList* drawList, const ImVec2& currentPos) {
    ImVec2 transformedPos = transformCoordinates(currentPos);
    ImVec2 transformedStart = transformCoordinates(startPoint);
    
    // Find nearest snap point from existing shapes
    ImVec2 snappedPos = currentPos;
    float minDist = Constants::SNAP_THRESHOLD / zoomLevel;
    bool foundSnap = false;
    
    if (snapToGrid) {
        for (const auto& shape : shapes) {
            switch (shape->type) {
                case ShapeType::POINT: {
                    const auto& point = static_cast<const Point&>(*shape);
                    float dist = calculateDistance(currentPos, point.position);
                    if (dist < minDist) {
                        minDist = dist;
                        snappedPos = point.position;
                        foundSnap = true;
                    }
                    break;
                }
                case ShapeType::LINE: {
                    const auto& line = static_cast<const Line&>(*shape);
                    // Snap to endpoints
                    float distStart = calculateDistance(currentPos, line.start);
                    float distEnd = calculateDistance(currentPos, line.end);
                    if (distStart < minDist) {
                        minDist = distStart;
                        snappedPos = line.start;
                        foundSnap = true;
                    }
                    if (distEnd < minDist) {
                        minDist = distEnd;
                        snappedPos = line.end;
                        foundSnap = true;
                    }
                    // Snap to midpoint
                    ImVec2 midpoint = {
                        (line.start.x + line.end.x) * 0.5f,
                        (line.start.y + line.end.y) * 0.5f
                    };
                    float distMid = calculateDistance(currentPos, midpoint);
                    if (distMid < minDist) {
                        minDist = distMid;
                        snappedPos = midpoint;
                        foundSnap = true;
                    }
                    break;
                }
                // Add cases for other shape types as needed
            }
        }
    }
    
    // Draw snap indicator if we found a snap point
    if (foundSnap) {
        ImVec2 transformedSnap = transformCoordinates(snappedPos);
        drawList->AddCircle(transformedSnap, 3.0f, IM_COL32(255, 255, 0, 255), 0, 2.0f);
        drawList->AddCircle(transformedSnap, 6.0f, IM_COL32(255, 255, 0, 128), 0, 1.0f);
        
        // Draw snap line from current position to snap point
        drawDashedLine(drawList, transformedPos, transformedSnap, 
                      IM_COL32(255, 255, 0, 128), 1.0f, 5.0f);
        
        transformedPos = transformedSnap;
    }
    
    switch (currentMode) {
        case DrawingMode::Point:
            previewPoint(drawList, transformedPos);
            break;
        case DrawingMode::Line:
            if (isDrawing) {
                previewLine(drawList, transformedStart, transformedPos);
            } else {
                previewPoint(drawList, transformedPos);
            }
            break;
        case DrawingMode::Circle:
            if (isDrawing) {
                float radius = calculateDistance(startPoint, snappedPos);
                previewCircle(drawList, transformedStart, radius * zoomLevel);
            } else {
                previewPoint(drawList, transformedPos);
            }
            break;
        case DrawingMode::Triangle:
            if (isDrawing) {
                previewTriangle(drawList, trianglePoints, clickCount);
                // Show preview of next point
                if (clickCount > 0 && clickCount < 3) {
                    ImVec2 lastPoint = transformCoordinates(trianglePoints[clickCount - 1]);
                    drawDashedLine(drawList, lastPoint, transformedPos, Colors::PREVIEW_LIGHT, 
                                 1.0f * zoomLevel, 5.0f * zoomLevel);
                }
            } else {
                previewPoint(drawList, transformedPos);
            }
            break;
        case DrawingMode::Square:
            if (isDrawing) {
                previewSquare(drawList, transformedStart, transformedPos);
            } else {
                previewPoint(drawList, transformedPos);
            }
            break;
        case DrawingMode::Rectangle:
            if (isDrawing) {
                previewRectangle(drawList, transformedStart, transformedPos);
            } else {
                previewPoint(drawList, transformedPos);
            }
            break;
        case DrawingMode::Spline:
            if (!currentSplinePoints.empty()) {
                previewSpline(drawList, currentSplinePoints);
                // Show line from last point to current position
                if (currentSplinePoints.size() >= 1) {
                    ImVec2 lastPoint = transformCoordinates(currentSplinePoints.back());
                    drawDashedLine(drawList, lastPoint, transformedPos, Colors::PREVIEW_LIGHT, 
                                 1.0f * zoomLevel, 5.0f * zoomLevel);
                }
            } else {
                previewPoint(drawList, transformedPos);
            }
            break;
        case DrawingMode::BezierCurve:
            if (!currentCurvePoints.empty()) {
                previewBezier(drawList, currentCurvePoints);
                // Show line from last point to current position
                if (currentCurvePoints.size() < 4) {
                    ImVec2 lastPoint = transformCoordinates(currentCurvePoints.back());
                    drawDashedLine(drawList, lastPoint, transformedPos, Colors::PREVIEW_LIGHT, 
                                 1.0f * zoomLevel, 5.0f * zoomLevel);
                }
            } else {
                previewPoint(drawList, transformedPos);
            }
            break;
        default:
            break;
    }
}

void Canvas::handleCurveManipulation(const ImVec2& mousePos) {
    if (!selectedShape) return;
    
    if (selectedShape->type == ShapeType::SPLINE) {
        auto* spline = static_cast<Spline*>(selectedShape);
        
        // Find and select control point on click
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            float minDist = Constants::SNAP_THRESHOLD / zoomLevel;
            spline->selectedPoint = -1;
            
            for (size_t i = 0; i < spline->controlPoints.size(); ++i) {
                float dist = calculateDistance(mousePos, spline->controlPoints[i]);
                if (dist < minDist) {
                    minDist = dist;
                    spline->selectedPoint = static_cast<int>(i);
                }
            }
        }
        
        // Move selected control point
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && 
            spline->selectedPoint >= 0 && 
            spline->selectedPoint < spline->controlPoints.size()) {
            spline->moveControlPoint(spline->selectedPoint, mousePos);
        }
        
        // Clear selection on release
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            spline->selectedPoint = -1;
        }
    } else if (selectedShape->type == ShapeType::BEZIER) {
        auto* bezier = static_cast<BezierCurve*>(selectedShape);
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            if (bezier->selectedPoint >= 0 && 
                bezier->selectedPoint < bezier->controlPoints.size()) {
                if (ImGui::GetIO().KeyShift) {
                    bezier->adjustSymmetrically(bezier->selectedPoint, mousePos);
                } else {
                    bezier->moveControlPoint(bezier->selectedPoint, mousePos);
                }
            }
        }
    }
}

// Shape drawing handlers
void Canvas::handlePointDrawing(ImDrawList* drawList, const ImVec2& currentPos) {
    ImVec2 snappedPos = findNearestSnapPoint(currentPos);
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        saveToHistory();
        shapes.push_back(std::make_unique<Point>(snappedPos));
    }
}

void Canvas::handleLineDrawing(ImDrawList* drawList, const ImVec2& currentPos) {
    ImVec2 snappedPos = findNearestSnapPoint(currentPos);
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (!isDrawing) {
            isDrawing = true;
            startPoint = snappedPos;
        } else {
            saveToHistory();
            shapes.push_back(std::make_unique<Line>(startPoint, snappedPos));
            isDrawing = false;
        }
    }
}

void Canvas::handleCircleDrawing(ImDrawList* drawList, const ImVec2& currentPos) {
    ImVec2 snappedPos = findNearestSnapPoint(currentPos);
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (!isDrawing) {
            isDrawing = true;
            startPoint = snappedPos;
        } else {
            float radius = calculateDistance(startPoint, snappedPos);
            saveToHistory();
            shapes.push_back(std::make_unique<Circle>(startPoint, radius));
            isDrawing = false;
        }
    }
}

void Canvas::handleTriangleDrawing(ImDrawList* drawList, const ImVec2& currentPos) {
    ImVec2 snappedPos = findNearestSnapPoint(currentPos);
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (clickCount < 3) {
            trianglePoints[clickCount] = snappedPos;
            clickCount++;
            if (clickCount == 1) {
                isDrawing = true;
            } else if (clickCount == 3) {
                saveToHistory();
                shapes.push_back(std::make_unique<Triangle>(trianglePoints));
                isDrawing = false;
                clickCount = 0;
            }
        }
    }
}

void Canvas::handleSquareDrawing(ImDrawList* drawList, const ImVec2& currentPos) {
    ImVec2 snappedPos = findNearestSnapPoint(currentPos);
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (!isDrawing) {
            isDrawing = true;
            startPoint = snappedPos;
        } else {
            saveToHistory();
            shapes.push_back(std::make_unique<Square>(startPoint, snappedPos));
            isDrawing = false;
        }
    }
}

void Canvas::handleRectangleDrawing(ImDrawList* drawList, const ImVec2& currentPos) {
    ImVec2 snappedPos = findNearestSnapPoint(currentPos);
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (!isDrawing) {
            isDrawing = true;
            startPoint = snappedPos;
        } else {
            saveToHistory();
            shapes.push_back(std::make_unique<Rectangle>(startPoint, snappedPos));
            isDrawing = false;
        }
    }
}

void Canvas::handleSplineDrawing(ImDrawList* drawList, const ImVec2& currentPos) {
    ImVec2 snappedPos = findNearestSnapPoint(currentPos);
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (!isDrawing) {
            isDrawing = true;
            currentSplinePoints.clear();
        }
        currentSplinePoints.push_back(snappedPos);
    }
    
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && currentSplinePoints.size() >= 2) {
        saveToHistory();
        auto spline = std::make_unique<Spline>(currentSplinePoints);
        spline->showControlPoints = true;
        shapes.push_back(std::move(spline));
        isDrawing = false;
        currentSplinePoints.clear();
    }
}

void Canvas::handleBezierDrawing(ImDrawList* drawList, const ImVec2& currentPos) {
    ImVec2 snappedPos = findNearestSnapPoint(currentPos);
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (!isDrawing) {
            isDrawing = true;
            currentCurvePoints.clear();
        }
        currentCurvePoints.push_back(snappedPos);
        
        if (currentCurvePoints.size() == 4) {
            saveToHistory();
            auto bezier = std::make_unique<BezierCurve>(currentCurvePoints);
            shapes.push_back(std::move(bezier));
            isDrawing = false;
            currentCurvePoints.clear();
        }
    }
}

void Canvas::previewPoint(ImDrawList* drawList, const ImVec2& pos) const {
    drawList->AddCircleFilled(pos, Constants::DEFAULT_POINT_SIZE * zoomLevel, Colors::PREVIEW);
}

void Canvas::previewLine(ImDrawList* drawList, const ImVec2& start, const ImVec2& end) const {
    drawList->AddLine(start, end, Colors::PREVIEW, Constants::DEFAULT_LINE_THICKNESS * zoomLevel);
}

void Canvas::previewCircle(ImDrawList* drawList, const ImVec2& center, float radius) const {
    drawList->AddCircle(
        center,
        radius,
        Colors::PREVIEW,
        128, // Increased from default to make circle smoother
        Constants::DEFAULT_LINE_THICKNESS * zoomLevel
    );
}

void Canvas::previewTriangle(ImDrawList* drawList, const std::array<ImVec2, 3>& points, int count) const {
    // Transform all points first
    std::array<ImVec2, 3> transformedPoints;
    for (int i = 0; i < count; ++i) {
        transformedPoints[i] = transformCoordinates(points[i]);
    }
    
    // Draw completed lines
    for (int i = 0; i < count - 1; ++i) {
        drawList->AddLine(
            transformedPoints[i],
            transformedPoints[i + 1],
            Colors::PREVIEW,
            Constants::DEFAULT_LINE_THICKNESS * zoomLevel
        );
    }
    
    // Draw preview line to current mouse position if not complete
    if (count > 0 && count < 3) {
        ImVec2 mousePos = transformCoordinates(inverseTransformCoordinates(ImGui::GetMousePos()));
        drawList->AddLine(
            transformedPoints[count - 1],
            mousePos,
            Colors::PREVIEW_LIGHT,
            Constants::DEFAULT_LINE_THICKNESS * zoomLevel
        );
    }
    
    // Draw closing line if all points are placed
    if (count == 3) {
        drawList->AddLine(
            transformedPoints[2],
            transformedPoints[0],
            Colors::PREVIEW,
            Constants::DEFAULT_LINE_THICKNESS * zoomLevel
        );
    }
}

void Canvas::previewSquare(ImDrawList* drawList, const ImVec2& start, const ImVec2& end) const {
    // Calculate the size based on the larger of width or height
    float dx = end.x - start.x;
    float dy = end.y - start.y;
    float size = std::max(std::abs(dx), std::abs(dy));
    
    // Determine direction for the square
    float signX = dx >= 0 ? 1.0f : -1.0f;
    float signY = dy >= 0 ? 1.0f : -1.0f;
    
    // Calculate corners
    ImVec2 topLeft = start;
    ImVec2 topRight(start.x + size * signX, start.y);
    ImVec2 bottomRight(start.x + size * signX, start.y + size * signY);
    ImVec2 bottomLeft(start.x, start.y + size * signY);
    
    // Draw the square
    drawList->AddLine(topLeft, topRight, Colors::PREVIEW, Constants::DEFAULT_LINE_THICKNESS * zoomLevel);
    drawList->AddLine(topRight, bottomRight, Colors::PREVIEW, Constants::DEFAULT_LINE_THICKNESS * zoomLevel);
    drawList->AddLine(bottomRight, bottomLeft, Colors::PREVIEW, Constants::DEFAULT_LINE_THICKNESS * zoomLevel);
    drawList->AddLine(bottomLeft, topLeft, Colors::PREVIEW, Constants::DEFAULT_LINE_THICKNESS * zoomLevel);
}

void Canvas::previewRectangle(ImDrawList* drawList, const ImVec2& start, const ImVec2& end) const {
    drawList->AddRect(start, end, Colors::PREVIEW, 0.0f, ImDrawFlags_None, Constants::DEFAULT_LINE_THICKNESS * zoomLevel);
}

void Canvas::previewSpline(ImDrawList* drawList, const std::vector<ImVec2>& points) const {
    if (points.empty()) return;

    // Draw control points
    for (const auto& point : points) {
        ImVec2 transformed = transformCoordinates(point);
        drawList->AddCircleFilled(transformed, 4.0f, IM_COL32(255, 255, 255, 255));
        drawList->AddCircle(transformed, 5.0f, IM_COL32(0, 0, 0, 255));
    }
    
    // Draw control polygon with dashed lines
    for (size_t i = 0; i < points.size() - 1; ++i) {
        ImVec2 transformed1 = transformCoordinates(points[i]);
        ImVec2 transformed2 = transformCoordinates(points[i + 1]);
        drawDashedLine(drawList, transformed1, transformed2, IM_COL32(128, 128, 128, 200), 
                      1.0f, 5.0f);
    }
    
    // Draw spline preview if we have enough points
    if (points.size() >= 2) {
        // Create temporary points array including current mouse position
        std::vector<ImVec2> previewPoints = points;
        ImVec2 mousePos = inverseTransformCoordinates(ImGui::GetMousePos());
        previewPoints.push_back(mousePos);
        
        // Calculate and draw the preview curve
        std::vector<ImVec2> curvePoints = calculateSplinePoints(previewPoints, false);
        for (size_t i = 1; i < curvePoints.size(); ++i) {
            ImVec2 transformed1 = transformCoordinates(curvePoints[i-1]);
            ImVec2 transformed2 = transformCoordinates(curvePoints[i]);
            drawList->AddLine(transformed1, transformed2, 
                            IM_COL32(255, 255, 255, 200), 
                            2.0f * zoomLevel);
        }
    }
}

void Canvas::previewBezier(ImDrawList* drawList, const std::vector<ImVec2>& points) const {
    // Draw control points
    for (const auto& point : points) {
        ImVec2 transformed = transformCoordinates(point);
        CurveUI::drawControlPoint(drawList, transformed, false);
    }
    
    // Draw control polygon with dashed lines
    for (size_t i = 0; i < points.size() - 1; ++i) {
        ImVec2 transformed1 = transformCoordinates(points[i]);
        ImVec2 transformed2 = transformCoordinates(points[i + 1]);
        drawDashedLine(drawList, transformed1, transformed2, Colors::PREVIEW_LIGHT, 
                      1.0f * zoomLevel, 5.0f * zoomLevel);
    }
    
    // Draw curve preview
    if (points.size() >= 2) {
        std::vector<ImVec2> previewPoints = points;
        if (points.size() < 4) {
            // Add current mouse position and any needed extra points for preview
            ImVec2 mousePos = inverseTransformCoordinates(ImGui::GetMousePos());
            previewPoints.push_back(mousePos);
            
            // For incomplete Bezier curves, duplicate last point as needed
            while (previewPoints.size() < 4) {
                previewPoints.push_back(previewPoints.back());
            }
        }
        
        // Calculate and draw the preview curve
        BezierCurve tempBezier(previewPoints);
        std::vector<ImVec2> curvePoints = tempBezier.calculatePoints();
        
        for (size_t i = 0; i < curvePoints.size() - 1; ++i) {
            ImVec2 transformed1 = transformCoordinates(curvePoints[i]);
            ImVec2 transformed2 = transformCoordinates(curvePoints[i + 1]);
            drawList->AddLine(transformed1, transformed2, Colors::PREVIEW, 
                            Constants::DEFAULT_LINE_THICKNESS * zoomLevel);
        }
    }
}

void Canvas::drawDashedLine(ImDrawList* drawList, const ImVec2& p1, const ImVec2& p2, 
                          ImU32 color, float thickness, float dash_length) const {
    ImVec2 direction = {p2.x - p1.x, p2.y - p1.y};
    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    
    if (length < 0.0001f) return;
    
    direction.x /= length;
    direction.y /= length;
    
    bool draw = true;
    ImVec2 current = p1;
    float remaining = length;
    
    while (remaining > 0) {
        float segment = std::min(dash_length, remaining);
        ImVec2 next = {
            current.x + direction.x * segment,
            current.y + direction.y * segment
        };
        
        if (draw) {
            drawList->AddLine(current, next, color, thickness);
        }
        
        current = next;
        remaining -= segment;
        draw = !draw;
    }
}

void Canvas::drawShape(const Shape& shape) const {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    switch (shape.type) {
        case ShapeType::POINT: {
            const auto& point = static_cast<const Point&>(shape);
            ImVec2 transformedPos = transformCoordinates(point.position);
            drawList->AddCircleFilled(transformedPos, point.size * zoomLevel, point.color);
            break;
        }
        case ShapeType::LINE: {
            const auto& line = static_cast<const Line&>(shape);
            ImVec2 transformedStart = transformCoordinates(line.start);
            ImVec2 transformedEnd = transformCoordinates(line.end);
            drawList->AddLine(transformedStart, transformedEnd, line.color, line.thickness * zoomLevel);
            break;
        }
        case ShapeType::CIRCLE: {
            const auto& circle = static_cast<const Circle&>(shape);
            ImVec2 transformedCenter = transformCoordinates(circle.center);
            drawList->AddCircle(
                transformedCenter,
                circle.radius * zoomLevel,
                circle.color,
                128, // Increased from default to make circle smoother
                circle.thickness * zoomLevel
            );
            break;
        }
        case ShapeType::TRIANGLE: {
            const auto& triangle = static_cast<const Triangle&>(shape);
            std::array<ImVec2, 3> transformedPoints;
            for (int i = 0; i < 3; ++i) {
                transformedPoints[i] = transformCoordinates(triangle.points[i]);
            }
            drawList->AddTriangle(transformedPoints[0], transformedPoints[1], transformedPoints[2], 
                                triangle.color, triangle.thickness * zoomLevel);
            break;
        }
        case ShapeType::SQUARE:
        case ShapeType::RECTANGLE: {
            const auto& rect = static_cast<const Rectangle&>(shape);
            ImVec2 transformedStart = transformCoordinates(rect.getTopLeft());
            ImVec2 transformedSize = rect.getSize();
            transformedSize.x *= zoomLevel;
            transformedSize.y *= zoomLevel;
            drawList->AddRect(transformedStart, 
                            ImVec2(transformedStart.x + transformedSize.x, transformedStart.y + transformedSize.y),
                            rect.color, 0.0f, ImDrawFlags_None, rect.thickness * zoomLevel);
            break;
        }
        case ShapeType::SPLINE: {
            const auto& spline = static_cast<const Spline&>(shape);
            if (spline.controlPoints.size() < 2) break;
            
            std::vector<ImVec2> points = calculateSplinePoints(spline.controlPoints, spline.isClosed);
            for (size_t i = 1; i < points.size(); ++i) {
                ImVec2 p1 = transformCoordinates(points[i-1]);
                ImVec2 p2 = transformCoordinates(points[i]);
                drawList->AddLine(p1, p2, spline.color, spline.thickness * zoomLevel);
            }
            
            // Draw control points if enabled
            if (spline.showControlPoints) {
                for (const auto& point : spline.controlPoints) {
                    ImVec2 transformedPoint = transformCoordinates(point);
                    drawList->AddCircleFilled(transformedPoint, 3.0f, IM_COL32(255, 255, 255, 255));
                    drawList->AddCircle(transformedPoint, 4.0f, IM_COL32(0, 0, 0, 255));
                }
            }
            break;
        }
        case ShapeType::BEZIER: {
            const auto& bezier = static_cast<const BezierCurve&>(shape);
            if (!bezier.isValid()) break;
            
            // Draw the curve
            std::vector<ImVec2> points = bezier.calculatePoints(0.01f);
            for (size_t i = 1; i < points.size(); ++i) {
                ImVec2 p1 = transformCoordinates(points[i-1]);
                ImVec2 p2 = transformCoordinates(points[i]);
                drawList->AddLine(p1, p2, bezier.color, bezier.thickness * zoomLevel);
            }
            
            // Draw control points and handles if enabled
            if (bezier.showControlPoints) {
                for (size_t i = 0; i < bezier.controlPoints.size(); ++i) {
                    ImVec2 transformedPoint = transformCoordinates(bezier.controlPoints[i]);
                    bool isEndPoint = (i == 0 || i == 3);
                    float size = isEndPoint ? 4.0f : 3.0f;
                    ImU32 color = isEndPoint ? IM_COL32(255, 255, 255, 255) : IM_COL32(255, 200, 0, 255);
                    drawList->AddCircleFilled(transformedPoint, size, color);
                    drawList->AddCircle(transformedPoint, size + 1.0f, IM_COL32(0, 0, 0, 255));
                }
                
                // Draw control polygon
                for (size_t i = 1; i < bezier.controlPoints.size(); ++i) {
                    ImVec2 p1 = transformCoordinates(bezier.controlPoints[i-1]);
                    ImVec2 p2 = transformCoordinates(bezier.controlPoints[i]);
                    drawList->AddLine(p1, p2, IM_COL32(128, 128, 128, 128), 1.0f);
                }
            }
            break;
        }
    }
}

ImVec2 Canvas::catmullRomPoint(const ImVec2& p0, const ImVec2& p1, 
                              const ImVec2& p2, const ImVec2& p3, float t) const {
    const float alpha = 0.5f;  // 0.5 for centripetal Catmull-Rom
    float t2 = t * t;
    float t3 = t2 * t;

    ImVec2 result;
    result.x = 0.5f * (
        (2.0f * p1.x) +
        (-p0.x + p2.x) * t +
        (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 +
        (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3
    );
    result.y = 0.5f * (
        (2.0f * p1.y) +
        (-p0.y + p2.y) * t +
        (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 +
        (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3
    );
    return result;
}

void Canvas::reset() {
    currentMode = DrawingMode::None;
    isDrawing = false;
    isFirstClick = true;
    clickCount = 0;
    currentSplinePoints.clear();
    currentCurvePoints.clear();
    selectedShape = nullptr;
}

// Snapping methods
std::optional<Canvas::SnapPoint> Canvas::findSnapPoint(const ImVec2& mousePos) const {
    // Check for grid points first
    if (snapToGrid) {
        ImVec2 snapped = getSnappedPoint(mousePos);
        if (calculateDistance(mousePos, snapped) <= Constants::SNAP_THRESHOLD / zoomLevel) {
            return SnapPoint{snapped, "Grid"};
        }
    }
    
    // Check for existing points
    if (auto nearestPoint = findNearestPoint(mousePos, Constants::SNAP_THRESHOLD / zoomLevel)) {
        return SnapPoint{*nearestPoint, "Point"};
    }
    
    // Check for midpoints
    if (auto midPoint = findMidPoint(mousePos)) {
        return SnapPoint{*midPoint, "Midpoint"};
    }
    
    // Check for intersections
    if (auto intersection = findIntersection(mousePos)) {
        return SnapPoint{*intersection, "Intersection"};
    }
    
    // Check for perpendicular points
    if (auto perpPoint = findPerpendicular(mousePos)) {
        return SnapPoint{*perpPoint, "Perpendicular"};
    }
    
    // Check for center points
    if (auto centerPoint = findCenter(mousePos)) {
        return SnapPoint{*centerPoint, "Center"};
    }
    
    return std::nullopt;
}

std::optional<ImVec2> Canvas::findMidPoint(const ImVec2& mousePos) const {
    float minDist = Constants::SNAP_THRESHOLD / zoomLevel;
    std::optional<ImVec2> result;
    
    for (const auto& shape : shapes) {
        if (shape->type == ShapeType::LINE) {
            const auto& line = static_cast<const Line&>(*shape);
            ImVec2 mid = calculateMidpoint(line.start, line.end);
            float dist = calculateDistance(mousePos, mid);
            if (dist < minDist) {
                minDist = dist;
                result = mid;
            }
        }
    }
    
    return result;
}

std::optional<ImVec2> Canvas::findIntersection(const ImVec2& mousePos) const {
    float minDist = Constants::SNAP_THRESHOLD / zoomLevel;
    std::optional<ImVec2> result;
    
    // Check intersections between lines
    for (size_t i = 0; i < shapes.size(); ++i) {
        if (shapes[i]->type != ShapeType::LINE) continue;
        const auto& line1 = static_cast<const Line&>(*shapes[i]);
        
        for (size_t j = i + 1; j < shapes.size(); ++j) {
            if (shapes[j]->type != ShapeType::LINE) continue;
            const auto& line2 = static_cast<const Line&>(*shapes[j]);
            
            // Calculate intersection point
            float x1 = line1.start.x, y1 = line1.start.y;
            float x2 = line1.end.x, y2 = line1.end.y;
            float x3 = line2.start.x, y3 = line2.start.y;
            float x4 = line2.end.x, y4 = line2.end.y;
            
            float denominator = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
            if (std::abs(denominator) < 0.0001f) continue;  // Lines are parallel
            
            float t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denominator;
            float u = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) / denominator;
            
            if (t >= 0 && t <= 1 && u >= 0 && u <= 1) {
                ImVec2 intersection = {
                    x1 + t * (x2 - x1),
                    y1 + t * (y2 - y1)
                };
                
                float dist = calculateDistance(mousePos, intersection);
                if (dist < minDist) {
                    minDist = dist;
                    result = intersection;
                }
            }
        }
    }
    
    return result;
}

std::optional<ImVec2> Canvas::findPerpendicular(const ImVec2& mousePos) const {
    float minDist = Constants::SNAP_THRESHOLD / zoomLevel;
    std::optional<ImVec2> result;
    
    for (const auto& shape : shapes) {
        if (shape->type == ShapeType::LINE) {
            const auto& line = static_cast<const Line&>(*shape);
            
            // Calculate perpendicular point
            ImVec2 v = {line.end.x - line.start.x, line.end.y - line.start.y};
            float len2 = v.x * v.x + v.y * v.y;
            if (len2 < 0.0001f) continue;
            
            float t = ((mousePos.x - line.start.x) * v.x + (mousePos.y - line.start.y) * v.y) / len2;
            if (t >= 0 && t <= 1) {
                ImVec2 perpPoint = {
                    line.start.x + t * v.x,
                    line.start.y + t * v.y
                };
                
                float dist = calculateDistance(mousePos, perpPoint);
                if (dist < minDist) {
                    minDist = dist;
                    result = perpPoint;
                }
            }
        }
    }
    
    return result;
}

std::optional<ImVec2> Canvas::findCenter(const ImVec2& mousePos) const {
    float minDist = Constants::SNAP_THRESHOLD / zoomLevel;
    std::optional<ImVec2> result;
    
    for (const auto& shape : shapes) {
        if (shape->type == ShapeType::CIRCLE) {
            const auto& circle = static_cast<const Circle&>(*shape);
            float dist = calculateDistance(mousePos, circle.center);
            if (dist < minDist) {
                minDist = dist;
                result = circle.center;
            }
        }
    }
    
    return result;
}

void Canvas::renderSnapIndicator(ImDrawList* drawList, const ImVec2& pos, const std::string& type) const {
    ImVec2 transformed = transformCoordinates(pos);
    float size = 5.0f * zoomLevel;
    
    // Draw a cross
    drawList->AddLine(
        ImVec2(transformed.x - size, transformed.y),
        ImVec2(transformed.x + size, transformed.y),
        Colors::CONTROL_POINT,
        1.0f
    );
    drawList->AddLine(
        ImVec2(transformed.x, transformed.y - size),
        ImVec2(transformed.x, transformed.y + size),
        Colors::CONTROL_POINT,
        1.0f
    );
    
    // Draw the type text
    ImVec2 textPos = ImVec2(transformed.x + size + 2.0f, transformed.y - size - 2.0f);
    drawList->AddText(textPos, Colors::CONTROL_POINT, type.c_str());
}

ImVec2 Canvas::getSnappedPoint(const ImVec2& point) const {
    if (!snapToGrid) return point;
    
    return ImVec2{
        std::round(point.x / gridSpacing) * gridSpacing,
        std::round(point.y / gridSpacing) * gridSpacing
    };
}

bool Canvas::trySnapToExistingPoint(ImVec2& point) const {
    if (auto nearestPoint = findNearestPoint(point, Constants::SNAP_THRESHOLD / zoomLevel)) {
        point = *nearestPoint;
        return true;
    }
    return false;
}

std::optional<ImVec2> Canvas::findNearestPoint(const ImVec2& point, float threshold) const {
    float minDist = threshold;
    std::optional<ImVec2> result;
    
    for (const auto& shape : shapes) {
        switch (shape->type) {
            case ShapeType::POINT: {
                const auto& p = static_cast<const Point&>(*shape);
                float dist = calculateDistance(point, p.position);
                if (dist < minDist) {
                    minDist = dist;
                    result = p.position;
                }
                break;
            }
            case ShapeType::LINE: {
                const auto& line = static_cast<const Line&>(*shape);
                float distStart = calculateDistance(point, line.start);
                float distEnd = calculateDistance(point, line.end);
                if (distStart < minDist) {
                    minDist = distStart;
                    result = line.start;
                }
                if (distEnd < minDist) {
                    minDist = distEnd;
                    result = line.end;
                }
                break;
            }
            case ShapeType::SPLINE: {
                const auto& spline = static_cast<const Spline&>(*shape);
                for (const auto& p : spline.controlPoints) {
                    float dist = calculateDistance(point, p);
                    if (dist < minDist) {
                        minDist = dist;
                        result = p;
                    }
                }
                break;
            }
            case ShapeType::BEZIER: {
                const auto& bezier = static_cast<const BezierCurve&>(*shape);
                for (const auto& p : bezier.controlPoints) {
                    float dist = calculateDistance(point, p);
                    if (dist < minDist) {
                        minDist = dist;
                        result = p;
                    }
                }
                break;
            }
            default:
                break;
        }
    }
    
    return result;
}

// Individual render methods
void Canvas::renderPoints(ImDrawList* drawList) const {
    for (const auto& shape : shapes) {
        if (shape->type == ShapeType::POINT) {
            drawShape(*shape);
        }
    }
}

void Canvas::renderLines(ImDrawList* drawList) const {
    for (const auto& shape : shapes) {
        if (shape->type == ShapeType::LINE) {
            drawShape(*shape);
        }
    }
}

void Canvas::renderCircles(ImDrawList* drawList) const {
    for (const auto& shape : shapes) {
        if (shape->type == ShapeType::CIRCLE) {
            drawShape(*shape);
        }
    }
}

void Canvas::renderTriangles(ImDrawList* drawList) const {
    for (const auto& shape : shapes) {
        if (shape->type == ShapeType::TRIANGLE) {
            drawShape(*shape);
        }
    }
}

void Canvas::renderSquares(ImDrawList* drawList) const {
    for (const auto& shape : shapes) {
        if (shape->type == ShapeType::SQUARE) {
            drawShape(*shape);
        }
    }
}

void Canvas::renderRectangles(ImDrawList* drawList) const {
    for (const auto& shape : shapes) {
        if (shape->type == ShapeType::RECTANGLE) {
            drawShape(*shape);
        }
    }
}

void Canvas::renderSplines(ImDrawList* drawList) const {
    for (const auto& shape : shapes) {
        if (shape->type == ShapeType::SPLINE) {
            drawShape(*shape);
        }
    }
}

void Canvas::renderBezierCurves(ImDrawList* drawList) const {
    for (const auto& shape : shapes) {
        if (shape->type == ShapeType::BEZIER) {
            drawShape(*shape);
        }
    }
}

// Curve calculation methods
ImVec2 Canvas::calculateBezierPoint(const std::vector<ImVec2>& points, float t) const {
    if (points.size() != 4) return ImVec2(0, 0);
    
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;
    
    ImVec2 p = {
        uuu * points[0].x +
        3 * uu * t * points[1].x +
        3 * u * tt * points[2].x +
        ttt * points[3].x,
        
        uuu * points[0].y +
        3 * uu * t * points[1].y +
        3 * u * tt * points[2].y +
        ttt * points[3].y
    };
    
    return p;
}

std::vector<ImVec2> Canvas::calculateSplinePoints(const std::vector<ImVec2>& controlPoints, bool isClosed) const {
    if (controlPoints.size() < 2) return {};
    
    std::vector<ImVec2> result;
    float step = 0.02f;  // Smaller step size for smoother curves
    
    if (controlPoints.size() == 2) {
        // For 2 points, just do linear interpolation
        const ImVec2& p0 = controlPoints[0];
        const ImVec2& p1 = controlPoints[1];
        for (float t = 0; t <= 1.0f; t += step) {
            result.push_back({
                p0.x + (p1.x - p0.x) * t,
                p0.y + (p1.y - p0.y) * t
            });
        }
        result.push_back(p1);
        return result;
    }

    // For Catmull-Rom spline, we need at least 4 points
    std::vector<ImVec2> points = controlPoints;
    if (isClosed) {
        // For closed splines, wrap the points
        points.insert(points.begin(), points.back());
        points.push_back(points[1]);
    } else {
        // For open splines, duplicate end points
        points.insert(points.begin(), points[0]);
        points.push_back(points.back());
    }

    // Calculate points along each segment
    for (size_t i = 0; i < points.size() - 3; ++i) {
        const ImVec2& p0 = points[i];
        const ImVec2& p1 = points[i + 1];
        const ImVec2& p2 = points[i + 2];
        const ImVec2& p3 = points[i + 3];

        for (float t = 0; t < 1.0f; t += step) {
            result.push_back(catmullRomPoint(p0, p1, p2, p3, t));
        }
    }

    return result;
}

ImVec2 Canvas::findNearestSnapPoint(const ImVec2& pos) const {
    if (!snapToGrid) return pos;
    
    float minDist = Constants::SNAP_THRESHOLD / zoomLevel;
    ImVec2 result = pos;
    bool found = false;
    
    for (const auto& shape : shapes) {
        switch (shape->type) {
            case ShapeType::POINT: {
                const auto& point = static_cast<const Point&>(*shape);
                float dist = calculateDistance(pos, point.position);
                if (dist < minDist) {
                    minDist = dist;
                    result = point.position;
                    found = true;
                }
                break;
            }
            case ShapeType::LINE: {
                const auto& line = static_cast<const Line&>(*shape);
                // Snap to endpoints
                float distStart = calculateDistance(pos, line.start);
                float distEnd = calculateDistance(pos, line.end);
                if (distStart < minDist) {
                    minDist = distStart;
                    result = line.start;
                    found = true;
                }
                if (distEnd < minDist) {
                    minDist = distEnd;
                    result = line.end;
                    found = true;
                }
                // Snap to midpoint
                ImVec2 midpoint = {
                    (line.start.x + line.end.x) * 0.5f,
                    (line.start.y + line.end.y) * 0.5f
                };
                float distMid = calculateDistance(pos, midpoint);
                if (distMid < minDist) {
                    minDist = distMid;
                    result = midpoint;
                    found = true;
                }
                break;
            }
            // Add cases for other shape types as needed
        }
    }
    
    return found ? result : pos;
}

// ... rest of existing code ...

} // namespace Drawing 