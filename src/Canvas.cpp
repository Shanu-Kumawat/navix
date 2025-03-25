#include "Canvas.hpp"
#include "utils/MathUtils.hpp"
#include "imgui.h"
#include <iostream>
#include <memory>
#include <cmath>

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
    gridSpacing(Constants::DEFAULT_GRID_SPACING),
    showControlPoints(true),
    selectedShape(nullptr),
    currentHistoryIndex(0),
    isDraggingCanvas(false),
    showGrid(true)
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

void Canvas::handleInput() {
    const ImGuiIO& io = ImGui::GetIO();
    
    // Skip input handling if we're not hovering the window
    if (!ImGui::IsWindowHovered())
        return;
    
    // Handle mouse wheel for zooming
    if (io.MouseWheel != 0) {
        updateZoom(io.MouseWheel);
    }
    
    // Handle middle mouse button for panning
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle) && ImGui::IsWindowHovered()) {
        ImVec2 delta = ImGui::GetIO().MouseDelta;
        updatePan(delta);
        isDraggingCanvas = true;
    } else {
        isDraggingCanvas = false;
    }
    
    // Handle drawing or selection based on current mode
    if (!isDraggingCanvas && ImGui::IsWindowHovered()) {
        ImVec2 mousePos = inverseTransformCoordinates(ImGui::GetMousePos());
        
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
    // Only render the grid if it's enabled
    if (showGrid) {
    renderGrid(drawList);
    }
    
    // Render all shapes
    renderShapes(drawList);
    
    // Render preview if drawing
    if (isDrawing && currentMode != DrawingMode::None && currentMode != DrawingMode::Select && ImGui::IsWindowHovered()) {
        ImVec2 currentPos = inverseTransformCoordinates(ImGui::GetMousePos());
        renderPreview(drawList, currentPos);
    }
    
    // Render selection indicators
    if (selectedShape) {
        // Render selection highlight
        ImU32 selectionColor = IM_COL32(255, 165, 0, 200); // Orange with transparency
        
                switch (selectedShape->type) {
                    case ShapeType::POINT: {
                const auto& point = static_cast<const Point&>(*selectedShape);
                ImVec2 transformedPos = transformCoordinates(point.position);
                drawList->AddCircle(transformedPos, (point.size + 3.0f) * zoomLevel, selectionColor, 0, 2.0f);
                        break;
                    }
                    case ShapeType::LINE: {
                const auto& line = static_cast<const Line&>(*selectedShape);
                ImVec2 transformedStart = transformCoordinates(line.start);
                ImVec2 transformedEnd = transformCoordinates(line.end);
                
                float thickness = std::max(line.thickness * zoomLevel + 4.0f, 3.0f);
                drawList->AddLine(transformedStart, transformedEnd, selectionColor, thickness);
                
                // Add selection indicators at endpoints
                drawList->AddCircleFilled(transformedStart, 5.0f * zoomLevel, selectionColor);
                drawList->AddCircleFilled(transformedEnd, 5.0f * zoomLevel, selectionColor);
                        break;
                    }
            // Handle other shape types for selection highlight
                    default:
                        break;
                }
    }
}

void Canvas::handleDrawing(const ImVec2& mousePos) {
    if (isDraggingCanvas) return;
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    // Only process drawing if the mouse is over the canvas window
    if (!ImGui::IsWindowHovered()) return;
    
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
        // First remove the shape
        shapes.erase(it);
        std::cout << "Shape deleted, remaining shapes: " << shapes.size() << "\n";
        selectedShape = nullptr;
        
        // Then save the new state to history
        saveToHistory();
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
    // If we're not at the end of the history, truncate future states
    if (currentHistoryIndex < history.size() - 1) {
        history.resize(currentHistoryIndex + 1);
    }
    
    // Create a new history state with deep copy of shapes
    HistoryState state;
    for (const auto& shape : shapes) {
        state.shapes.push_back(shape->clone());
    }
    
    // Add to history
    history.push_back(std::move(state));
    if (history.size() > MAX_HISTORY_SIZE) {
        history.erase(history.begin());
    } else {
        currentHistoryIndex++;
    }
    
    std::cout << "Saved to history. History states: " << history.size() << ", Current index: " << currentHistoryIndex << "\n";
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
    shapes.clear();
    for (const auto& shape : state.shapes) {
        shapes.push_back(shape->clone());
    }
    selectedShape = nullptr;
    
    // Restore pan offset and zoom level
    panOffset = state.panOffset;
    zoomLevel = state.zoomLevel;
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
    // Get the canvas window position and size
    float startX = windowX;
    float startY = windowY;
    float endX = windowX + windowWidth;
    float endY = windowY + windowHeight;

    // Only render grid if it's enabled
    if (gridSpacing <= 0) return;

    // Calculate adjusted grid spacing based on zoom level
    float effectiveSpacing = gridSpacing * zoomLevel;
    
    // Ensure grid isn't too dense or sparse based on zoom level
    if (effectiveSpacing < 10.0f) {
        effectiveSpacing *= std::ceil(10.0f / effectiveSpacing);
    } else if (effectiveSpacing > 200.0f) {
        effectiveSpacing /= std::floor(effectiveSpacing / 100.0f);
    }

    // Calculate grid offset based on pan
    float offsetX = std::fmod(panOffset.x * zoomLevel, effectiveSpacing);
    float offsetY = std::fmod(panOffset.y * zoomLevel, effectiveSpacing);
    
    // Calculate number of lines needed
    int numLinesX = static_cast<int>(windowWidth / effectiveSpacing) + 2;
    int numLinesY = static_cast<int>(windowHeight / effectiveSpacing) + 2;

    // Draw vertical grid lines
        for (int i = 0; i <= numLinesX; i++) {
        float x = startX + i * effectiveSpacing + offsetX;
        if (x < startX) continue;
        if (x > endX) break;
        
        bool isMajor = (i % 5 == 0);
        ImU32 lineColor = isMajor ? Colors::GRID_MAJOR : Colors::GRID_MINOR;
        float lineThickness = isMajor ? 1.0f : 0.5f;
        
        drawList->AddLine(
            ImVec2(x, startY),
            ImVec2(x, endY),
            lineColor,
            lineThickness
        );
    }

    // Draw horizontal grid lines
    for (int i = 0; i <= numLinesY; i++) {
        float y = startY + i * effectiveSpacing + offsetY;
        if (y < startY) continue;
        if (y > endY) break;
        
        bool isMajor = (i % 5 == 0);
        ImU32 lineColor = isMajor ? Colors::GRID_MAJOR : Colors::GRID_MINOR;
        float lineThickness = isMajor ? 1.0f : 0.5f;
        
        drawList->AddLine(
            ImVec2(startX, y),
            ImVec2(endX, y),
            lineColor,
            lineThickness
        );
    }
    
    // Add axes lines at (0,0) if visible
    ImVec2 origin = transformCoordinates(ImVec2(0, 0));
    ImU32 axisColor = IM_COL32(180, 180, 180, 220);
    
    if (origin.x >= startX && origin.x <= endX) {
        drawList->AddLine(
            ImVec2(origin.x, startY),
            ImVec2(origin.x, endY),
            axisColor,
            1.5f
        );
    }
    
    if (origin.y >= startY && origin.y <= endY) {
        drawList->AddLine(
            ImVec2(startX, origin.y),
            ImVec2(endX, origin.y),
            axisColor,
            1.5f
        );
    }
}

void Canvas::renderShapes(ImDrawList* drawList) const {
    renderPoints(drawList);
    renderLines(drawList);
    renderCircles(drawList);
    renderTriangles(drawList);
    renderSquares(drawList);
    renderRectangles(drawList);
    renderSplines(drawList);
    renderBezierCurves(drawList);
}

void Canvas::renderPreview(ImDrawList* drawList, const ImVec2& currentPos) {
    if (!isDrawing) return;
    
    switch (currentMode) {
        case DrawingMode::Line:
            previewLine(drawList, startPoint, currentPos);
            break;
        case DrawingMode::Circle:
            previewCircle(drawList, startPoint, calculateDistance(startPoint, currentPos));
            break;
        case DrawingMode::Triangle:
            previewTriangle(drawList, trianglePoints, clickCount);
            break;
        case DrawingMode::Square:
            previewSquare(drawList, startPoint, currentPos);
            break;
        case DrawingMode::Rectangle:
            previewRectangle(drawList, startPoint, currentPos);
            break;
        case DrawingMode::Spline:
            previewSpline(drawList, currentSplinePoints);
            break;
        case DrawingMode::BezierCurve:
            previewBezier(drawList, currentCurvePoints);
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
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered()) {
        // Create and add the new shape FIRST
        shapes.push_back(std::make_unique<Point>(snappedPos));
        std::cout << "Point added at (" << snappedPos.x << ", " << snappedPos.y << ")\n";
        
        // THEN save the history with the new shape included
        saveToHistory();
    }
}

void Canvas::handleLineDrawing(ImDrawList* drawList, const ImVec2& currentPos) {
    ImVec2 snappedPos = findNearestSnapPoint(currentPos);
    
    // Display snap indicator for better visual feedback
    if (auto snapPoint = findSnapPoint(currentPos)) {
        renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
    }
    
    if (fixedLineLength) {
        // Fixed length mode - two click drawing
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            if (isDrawing) {
                // Second click sets the direction
                endPoint = snappedPos;
                
                // Use fixed length from property panel
                ImVec2 direction = normalizeVector(ImVec2(endPoint.x - startPoint.x, endPoint.y - startPoint.y));
                ImVec2 actualEnd = ImVec2(
                    startPoint.x + direction.x * lineLength,
                    startPoint.y + direction.y * lineLength
                );
                shapes.push_back(std::make_unique<Line>(startPoint, actualEnd));
                saveToHistory();
                isDrawing = false;
            } else {
                // First click sets the start point
                startPoint = snappedPos;
                isDrawing = true;
                
                // Show snap indicator at the start point
                if (auto snapPoint = findSnapPoint(startPoint)) {
                    renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
                }
            }
        }
        
        if (isDrawing) {
            // Preview with fixed length
            ImVec2 direction = normalizeVector(ImVec2(currentPos.x - startPoint.x, currentPos.y - startPoint.y));
            ImVec2 previewEnd = ImVec2(
                startPoint.x + direction.x * lineLength,
                startPoint.y + direction.y * lineLength
            );
            previewLine(drawList, startPoint, previewEnd);
            
            // Show snap indicator at the start point for continuous feedback
            if (auto snapPoint = findSnapPoint(startPoint)) {
                renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
            }
        }
    } else {
        // Dynamic length mode - click and drag
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            // Start drawing on mouse down
            startPoint = snappedPos;
            isDrawing = true;
            
            // Show snap indicator at the start point
            if (auto snapPoint = findSnapPoint(startPoint)) {
                renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
            }
        }
        else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && isDrawing) {
            // Complete line on mouse up
            endPoint = snappedPos;
            
            // Calculate length
            float length = calculateDistance(startPoint, endPoint);
            
            // Only create if line is long enough
            if (length > Constants::MIN_SHAPE_SIZE) {
                shapes.push_back(std::make_unique<Line>(startPoint, endPoint));
                saveToHistory();
            }
            isDrawing = false;
        }
        
        if (isDrawing) {
            // Preview line while dragging
            previewLine(drawList, startPoint, snappedPos);
            
            // Show snap indicators at both the start and end points
            if (auto snapPoint = findSnapPoint(startPoint)) {
                renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
            }
        }
    }
}

void Canvas::handleCircleDrawing(ImDrawList* drawList, const ImVec2& currentPos) {
    ImVec2 snappedPos = findNearestSnapPoint(currentPos);
    
    // Display snap indicator for better visual feedback
    if (auto snapPoint = findSnapPoint(currentPos)) {
        renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
    }
    
    if (fixedCircleRadius) {
        // Fixed radius mode - two click drawing
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            if (isDrawing) {
                // Second click sets the direction
                endPoint = snappedPos;
                
                // Use fixed radius from property panel
                shapes.push_back(std::make_unique<Circle>(startPoint, circleRadius));
                saveToHistory();
                isDrawing = false;
            } else {
                // First click sets the center
                startPoint = snappedPos;
                isDrawing = true;
                
                // Show snap indicator at the center point
                if (auto snapPoint = findSnapPoint(startPoint)) {
                    renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
                }
            }
        }
        
        if (isDrawing) {
            // Preview with fixed radius
            previewCircle(drawList, startPoint, circleRadius);
            
            // Show snap indicator at the center point for continuous feedback
            if (auto snapPoint = findSnapPoint(startPoint)) {
                renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
            }
        }
    } else {
        // Dynamic radius mode - click and drag
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            // Start drawing on mouse down
            startPoint = snappedPos;
            isDrawing = true;
            
            // Show snap indicator at the center point
            if (auto snapPoint = findSnapPoint(startPoint)) {
                renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
            }
        }
        else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && isDrawing) {
            // Calculate radius from center to mouse position
            float radius = calculateDistance(startPoint, snappedPos);
            
            // Only create if radius is large enough
            if (radius > Constants::MIN_SHAPE_SIZE) {
                shapes.push_back(std::make_unique<Circle>(startPoint, radius));
                saveToHistory();
            }
            isDrawing = false;
        }
        
        if (isDrawing) {
            // Preview circle while dragging
            endPoint = snappedPos;
            float radius = calculateDistance(startPoint, endPoint);
            previewCircle(drawList, startPoint, radius);
            
            // Show snap indicators at both the center and current radius point
            if (auto snapPoint = findSnapPoint(startPoint)) {
                renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
            }
        }
    }
}

void Canvas::handleTriangleDrawing(ImDrawList* drawList, const ImVec2& currentPos) {
    ImVec2 snappedPos = findNearestSnapPoint(currentPos);
    
    // Display snap indicator for better visual feedback
    if (auto snapPoint = findSnapPoint(currentPos)) {
        renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
    }
    
    if (!isDrawing) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            isDrawing = true;
            startPoint = snappedPos;
            clickCount = 1;
            
            // Show snap indicator at the start point
            if (auto snapPoint = findSnapPoint(startPoint)) {
                renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
            }
        }
    } else {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (isFixedTriangleSize()) {
                // Fixed size mode: first click sets position, second click sets direction
                if (clickCount == 1) {
                    // Calculate direction vector
                    ImVec2 direction = ImVec2(snappedPos.x - startPoint.x, snappedPos.y - startPoint.y);
                    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                    if (length > 0.0f) {
                        direction = ImVec2(direction.x / length, direction.y / length);
                        
                        // Calculate the three points of the equilateral triangle
                        float height = triangleSide * std::sqrt(3.0f) / 2.0f;
                        ImVec2 p1 = startPoint;
                        ImVec2 p2 = ImVec2(startPoint.x + triangleSide * direction.x,
                                         startPoint.y + triangleSide * direction.y);
                        ImVec2 p3 = ImVec2(startPoint.x + triangleSide * 0.5f * direction.x - height * direction.y,
                                         startPoint.y + triangleSide * 0.5f * direction.y + height * direction.x);
                        
                        // Draw the triangle
                        std::array<ImVec2, 3> points = {p1, p2, p3};
                        shapes.push_back(std::make_unique<Triangle>(points));
                        saveToHistory();
                        isDrawing = false;
                        clickCount = 0;
                    }
                }
            } else {
                // Dynamic size mode: drag to set size
                ImVec2 direction = ImVec2(snappedPos.x - startPoint.x, snappedPos.y - startPoint.y);
                float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                if (length > 0.0f) {
                    direction = ImVec2(direction.x / length, direction.y / length);
                    
                    // Calculate the three points of the equilateral triangle
                    float height = length * std::sqrt(3.0f) / 2.0f;
                    ImVec2 p1 = startPoint;
                    ImVec2 p2 = ImVec2(startPoint.x + length * direction.x,
                                     startPoint.y + length * direction.y);
                    ImVec2 p3 = ImVec2(startPoint.x + length * 0.5f * direction.x - height * direction.y,
                                     startPoint.y + length * 0.5f * direction.y + height * direction.x);
                    
                    // Draw preview
                    std::array<ImVec2, 3> previewPoints = {p1, p2, p3};
                    previewTriangle(drawList, previewPoints, 3);
                    
                    // Show snap indicators at vertices and midpoints
                    for (const auto& point : {p1, p2, p3}) {
                        if (auto snapPoint = findSnapPoint(point)) {
                            renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
                        }
                    }
                    
                    // Show snap indicators at midpoints of edges
                    for (int i = 0; i < 3; ++i) {
                        ImVec2 midpoint = {
                            (previewPoints[i].x + previewPoints[(i + 1) % 3].x) * 0.5f,
                            (previewPoints[i].y + previewPoints[(i + 1) % 3].y) * 0.5f
                        };
                        if (auto snapPoint = findSnapPoint(midpoint)) {
                            renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
                        }
                    }
                }
            }
        } else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            if (!isFixedTriangleSize()) {
                // Complete the triangle in dynamic mode
                ImVec2 direction = ImVec2(snappedPos.x - startPoint.x, snappedPos.y - startPoint.y);
                float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                if (length > Constants::MIN_SHAPE_SIZE) {
                    direction = ImVec2(direction.x / length, direction.y / length);
                    
                    // Calculate the three points of the equilateral triangle
                    float height = length * std::sqrt(3.0f) / 2.0f;
                    ImVec2 p1 = startPoint;
                    ImVec2 p2 = ImVec2(startPoint.x + length * direction.x,
                                     startPoint.y + length * direction.y);
                    ImVec2 p3 = ImVec2(startPoint.x + length * 0.5f * direction.x - height * direction.y,
                                     startPoint.y + length * 0.5f * direction.y + height * direction.x);
                    
                    // Draw the triangle
                    std::array<ImVec2, 3> points = {p1, p2, p3};
                    shapes.push_back(std::make_unique<Triangle>(points));
                    saveToHistory();
                }
                isDrawing = false;
            }
        }
    }
}

void Canvas::handleSquareDrawing(ImDrawList* drawList, const ImVec2& currentPos) {
    ImVec2 snappedPos = findNearestSnapPoint(currentPos);
    
    // Display snap indicator for better visual feedback
    if (auto snapPoint = findSnapPoint(currentPos)) {
        renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
    }
    
    if (!isDrawing) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            isDrawing = true;
            startPoint = snappedPos;
            clickCount = 1;
            
            // Show snap indicator at the start point
            if (auto snapPoint = findSnapPoint(startPoint)) {
                renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
            }
        }
    } else {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (isFixedSquareSize()) {
                // Fixed size mode: first click sets position, second click sets direction
                if (clickCount == 1) {
                    // Calculate direction vector
                    ImVec2 direction = ImVec2(snappedPos.x - startPoint.x, snappedPos.y - startPoint.y);
                    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                    if (length > 0.0f) {
                        direction = ImVec2(direction.x / length, direction.y / length);
                        
                        // Calculate the four corners of the square
                        ImVec2 p1 = startPoint;
                        ImVec2 p2 = ImVec2(startPoint.x + squareSize * direction.x,
                                         startPoint.y + squareSize * direction.y);
                        ImVec2 p3 = ImVec2(p2.x - squareSize * direction.y,
                                         p2.y + squareSize * direction.x);
                        ImVec2 p4 = ImVec2(p1.x - squareSize * direction.y,
                                         p1.y + squareSize * direction.x);
                        
                        // Draw the square
                        shapes.push_back(std::make_unique<Square>(p1, p2));
                        saveToHistory();
                        isDrawing = false;
                        clickCount = 0;
                    }
                }
            } else {
                // Dynamic size mode: drag to set size
                // Calculate size based on the larger of width or height
                float size = std::max(
                    std::abs(snappedPos.x - startPoint.x),
                    std::abs(snappedPos.y - startPoint.y)
                );
                
                // Calculate the four corners of the square
                float signX = (snappedPos.x >= startPoint.x) ? 1.0f : -1.0f;
                float signY = (snappedPos.y >= startPoint.y) ? 1.0f : -1.0f;
                
                ImVec2 p1 = startPoint;
                ImVec2 p2 = ImVec2(startPoint.x + size * signX, startPoint.y);
                ImVec2 p3 = ImVec2(startPoint.x + size * signX, startPoint.y + size * signY);
                ImVec2 p4 = ImVec2(startPoint.x, startPoint.y + size * signY);
                
                // Draw preview
                previewSquare(drawList, startPoint, snappedPos);
                
                // Show snap indicators at corners and midpoints
                for (const auto& point : {p1, p2, p3, p4}) {
                    if (auto snapPoint = findSnapPoint(point)) {
                        renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
                    }
                }
                
                // Show snap indicators at midpoints of sides
                for (int i = 0; i < 4; ++i) {
                    ImVec2 midpoint = {
                        (p1.x + p2.x) * 0.5f,
                        (p1.y + p2.y) * 0.5f
                    };
                    if (auto snapPoint = findSnapPoint(midpoint)) {
                        renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
                    }
                }
            }
        } else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            if (!isFixedSquareSize()) {
                // Complete the square in dynamic mode
                float size = std::max(
                    std::abs(snappedPos.x - startPoint.x),
                    std::abs(snappedPos.y - startPoint.y)
                );
                
                // Only create if square is large enough
                if (size > Constants::MIN_SHAPE_SIZE) {
                    // Calculate the four corners of the square
                    float signX = (snappedPos.x >= startPoint.x) ? 1.0f : -1.0f;
                    float signY = (snappedPos.y >= startPoint.y) ? 1.0f : -1.0f;
                    
                    ImVec2 p1 = startPoint;
                    ImVec2 p2 = ImVec2(startPoint.x + size * signX, startPoint.y);
                    ImVec2 p3 = ImVec2(startPoint.x + size * signX, startPoint.y + size * signY);
                    ImVec2 p4 = ImVec2(startPoint.x, startPoint.y + size * signY);
                    
                    shapes.push_back(std::make_unique<Square>(p1, p2));
                    saveToHistory();
                }
                isDrawing = false;
            }
        }
    }
}

void Canvas::handleRectangleDrawing(ImDrawList* drawList, const ImVec2& currentPos) {
    ImVec2 snappedPos = findNearestSnapPoint(currentPos);
    
    // Display snap indicator for better visual feedback
    if (auto snapPoint = findSnapPoint(currentPos)) {
        renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
    }
    
    if (!isDrawing) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            isDrawing = true;
            startPoint = snappedPos;
            clickCount = 1;
            
            // Show snap indicator at the start point
            if (auto snapPoint = findSnapPoint(startPoint)) {
                renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
            }
        }
    } else {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (isFixedRectangleSize()) {
                // Fixed size mode: first click sets position, second click sets direction
                if (clickCount == 1) {
                    // Calculate direction vector
                    ImVec2 direction = ImVec2(snappedPos.x - startPoint.x, snappedPos.y - startPoint.y);
                    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                    if (length > 0.0f) {
                        direction = ImVec2(direction.x / length, direction.y / length);
                        
                        // Calculate the four corners of the rectangle
                        ImVec2 p1 = startPoint;
                        ImVec2 p2 = ImVec2(startPoint.x + rectangleWidth * direction.x,
                                         startPoint.y + rectangleWidth * direction.y);
                        ImVec2 p3 = ImVec2(p2.x - rectangleHeight * direction.y,
                                         p2.y + rectangleHeight * direction.x);
                        ImVec2 p4 = ImVec2(p1.x - rectangleHeight * direction.y,
                                         p1.y + rectangleHeight * direction.x);
                        
                        // Draw the rectangle
                        shapes.push_back(std::make_unique<Rectangle>(p1, p2, p3, p4));
                        saveToHistory();
                        isDrawing = false;
                        clickCount = 0;
                    }
                }
            } else {
                // Dynamic size mode: drag to set size
                // Calculate width and height independently
                float width = std::abs(snappedPos.x - startPoint.x);
                float height = std::abs(snappedPos.y - startPoint.y);
                
                // Calculate the four corners of the rectangle
                float signX = (snappedPos.x >= startPoint.x) ? 1.0f : -1.0f;
                float signY = (snappedPos.y >= startPoint.y) ? 1.0f : -1.0f;
                
                ImVec2 p1 = startPoint;
                ImVec2 p2 = ImVec2(startPoint.x + width * signX, startPoint.y);
                ImVec2 p3 = ImVec2(startPoint.x + width * signX, startPoint.y + height * signY);
                ImVec2 p4 = ImVec2(startPoint.x, startPoint.y + height * signY);
                
                // Draw preview
                previewRectangle(drawList, startPoint, snappedPos);
                
                // Show snap indicators at corners and midpoints
                for (const auto& point : {p1, p2, p3, p4}) {
                    if (auto snapPoint = findSnapPoint(point)) {
                        renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
                    }
                }
            }
        } else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            if (!isFixedRectangleSize()) {
                // Complete the rectangle in dynamic mode
                float width = std::abs(snappedPos.x - startPoint.x);
                float height = std::abs(snappedPos.y - startPoint.y);
                
                // Only create if rectangle is large enough
                if (width > Constants::MIN_SHAPE_SIZE && height > Constants::MIN_SHAPE_SIZE) {
                    // Calculate the four corners of the rectangle
                    float signX = (snappedPos.x >= startPoint.x) ? 1.0f : -1.0f;
                    float signY = (snappedPos.y >= startPoint.y) ? 1.0f : -1.0f;
                    
                    ImVec2 p1 = startPoint;
                    ImVec2 p2 = ImVec2(startPoint.x + width * signX, startPoint.y);
                    ImVec2 p3 = ImVec2(startPoint.x + width * signX, startPoint.y + height * signY);
                    ImVec2 p4 = ImVec2(startPoint.x, startPoint.y + height * signY);
                    
                    shapes.push_back(std::make_unique<Rectangle>(p1, p2, p3, p4));
                    saveToHistory();
                }
                isDrawing = false;
            }
        }
    }
}

void Canvas::handleSplineDrawing(ImDrawList* drawList, const ImVec2& currentPos) {
    ImVec2 snappedPos = findNearestSnapPoint(currentPos);
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered()) {
        if (!isDrawing) {
            isDrawing = true;
            currentSplinePoints.clear();
        }
        currentSplinePoints.push_back(snappedPos);
    }
    
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsWindowHovered() && currentSplinePoints.size() >= 2) {
        // Create and add the new shape FIRST
        auto spline = std::make_unique<Spline>(currentSplinePoints);
        spline->showControlPoints = true;
        shapes.push_back(std::move(spline));
        std::cout << "Spline added with " << currentSplinePoints.size() << " control points\n";
        
        // THEN save the history with the new shape included
        saveToHistory();
        
        isDrawing = false;
        currentSplinePoints.clear();
    }
}

void Canvas::handleBezierDrawing(ImDrawList* drawList, const ImVec2& currentPos) {
    ImVec2 snappedPos = findNearestSnapPoint(currentPos);
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered()) {
        if (!isDrawing) {
            isDrawing = true;
            currentCurvePoints.clear();
        }
        currentCurvePoints.push_back(snappedPos);
        
        if (currentCurvePoints.size() == 4) {
            // Create and add the new shape FIRST
            auto bezier = std::make_unique<BezierCurve>(currentCurvePoints);
            shapes.push_back(std::move(bezier));
            std::cout << "Bezier curve added with 4 control points\n";
            
            // THEN save the history with the new shape included
            saveToHistory();
            
            isDrawing = false;
            currentCurvePoints.clear();
        }
    }
}

void Canvas::previewPoint(ImDrawList* drawList, const ImVec2& pos) const {
    ImVec2 transformedPos = transformCoordinates(pos);
    drawList->AddCircleFilled(transformedPos, Constants::DEFAULT_POINT_SIZE * zoomLevel, Colors::PREVIEW);
}

void Canvas::previewLine(ImDrawList* drawList, const ImVec2& start, const ImVec2& end) const {
    // Transform coordinates to screen space
    ImVec2 transformedStart = transformCoordinates(start);
    ImVec2 transformedEnd = transformCoordinates(end);
    
    // Draw the line
    drawList->AddLine(
        transformedStart, 
        transformedEnd, 
        Colors::PREVIEW, 
        Constants::DEFAULT_LINE_THICKNESS * zoomLevel
    );
    
    // Draw small circles at endpoints for better visibility
    drawList->AddCircleFilled(
        transformedStart,
        4.0f * zoomLevel,
        Colors::PREVIEW,
        8
    );
    
    drawList->AddCircleFilled(
        transformedEnd,
        4.0f * zoomLevel,
        Colors::PREVIEW,
        8
    );
}

void Canvas::previewCircle(ImDrawList* drawList, const ImVec2& center, float radius) const {
    // Transform coordinates to screen space
    ImVec2 transformedCenter = transformCoordinates(center);
    
    // Draw the circle
    drawList->AddCircle(
        transformedCenter,
        radius * zoomLevel,
        Colors::PREVIEW,
        128, // Increased from default to make circle smoother
        Constants::DEFAULT_LINE_THICKNESS * zoomLevel
    );
    
    // Draw the center point for better visibility
    drawList->AddCircleFilled(
        transformedCenter,
        4.0f * zoomLevel,
        Colors::PREVIEW,
        8
    );
    
    // Draw radius line for better visualization
    ImVec2 radiusPoint = transformCoordinates(ImVec2(
        center.x + radius, 
        center.y
    ));
    
    drawList->AddLine(
        transformedCenter,
        radiusPoint,
        Colors::PREVIEW_LIGHT,
        1.0f * zoomLevel
    );
    
    // Draw a small circle at the radius point
    drawList->AddCircleFilled(
        radiusPoint,
        3.0f * zoomLevel,
        Colors::PREVIEW,
        8
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
    
    // Draw corner points for better visibility
    for (int i = 0; i < count; ++i) {
        drawList->AddCircleFilled(
            transformedPoints[i],
            4.0f * zoomLevel,
            Colors::PREVIEW,
            8
        );
    }
    
    // Draw midpoints of edges for better visualization
    for (int i = 0; i < count; ++i) {
        ImVec2 midpoint = {
            (transformedPoints[i].x + transformedPoints[(i + 1) % count].x) * 0.5f,
            (transformedPoints[i].y + transformedPoints[(i + 1) % count].y) * 0.5f
        };
        drawList->AddCircleFilled(
            midpoint,
            3.0f * zoomLevel,
            Colors::PREVIEW_LIGHT,
            8
        );
    }
}

void Canvas::previewSquare(ImDrawList* drawList, const ImVec2& start, const ImVec2& end) const {
    // Calculate the size based on the larger of width or height
    float size = std::max(
        std::abs(end.x - start.x),
        std::abs(end.y - start.y)
    );
    
    // Calculate signs for direction
    float signX = (end.x >= start.x) ? 1.0f : -1.0f;
    float signY = (end.y >= start.y) ? 1.0f : -1.0f;
    
    // Calculate corners in world space
    ImVec2 p1 = start;
    ImVec2 p2 = ImVec2(start.x + size * signX, start.y);
    ImVec2 p3 = ImVec2(start.x + size * signX, start.y + size * signY);
    ImVec2 p4 = ImVec2(start.x, start.y + size * signY);
    
    // Transform to screen space
    ImVec2 transformedPoints[4] = {
        transformCoordinates(p1),
        transformCoordinates(p2),
        transformCoordinates(p3),
        transformCoordinates(p4)
    };
    
    // Draw the square outline
    drawList->AddPolyline(transformedPoints, 4, Colors::PREVIEW, ImDrawFlags_Closed, 2.0f * zoomLevel);
    
    // Draw corner points for better visibility
    for (const auto& point : transformedPoints) {
        drawList->AddCircleFilled(point, 4.0f * zoomLevel, Colors::PREVIEW, 8);
    }
    
    // Draw diagonal line for better visualization
    drawList->AddLine(
        transformedPoints[0],
        transformedPoints[2],
        Colors::PREVIEW_LIGHT,
        1.0f * zoomLevel
    );
}

void Canvas::previewRectangle(ImDrawList* drawList, const ImVec2& start, const ImVec2& end) const {
    // Calculate width and height
    float width = std::abs(end.x - start.x);
    float height = std::abs(end.y - start.y);
    
    // Calculate signs for direction
    float signX = (end.x >= start.x) ? 1.0f : -1.0f;
    float signY = (end.y >= start.y) ? 1.0f : -1.0f;
    
    // Calculate corners in world space
    ImVec2 p1 = start;
    ImVec2 p2 = ImVec2(start.x + width * signX, start.y);
    ImVec2 p3 = ImVec2(start.x + width * signX, start.y + height * signY);
    ImVec2 p4 = ImVec2(start.x, start.y + height * signY);
    
    // Transform to screen space
    ImVec2 transformedPoints[4] = {
        transformCoordinates(p1),
        transformCoordinates(p2),
        transformCoordinates(p3),
        transformCoordinates(p4)
    };
    
    // Draw the rectangle outline
    drawList->AddPolyline(transformedPoints, 4, Colors::PREVIEW, ImDrawFlags_Closed, 2.0f * zoomLevel);
    
    // Draw corner points for better visibility
    for (const auto& point : transformedPoints) {
        drawList->AddCircleFilled(point, 4.0f * zoomLevel, Colors::PREVIEW, 8);
    }
    
    // Draw diagonal line for better visualization
    drawList->AddLine(
        transformedPoints[0],
        transformedPoints[2],
        Colors::PREVIEW_LIGHT,
        1.0f * zoomLevel
    );
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
    
    // Use the shape's own color (black from Constants.hpp) instead of hardcoding white
    ImU32 color = (selectedShape == &shape) ? IM_COL32(255, 255, 0, 255) : shape.color;
    
    switch (shape.type) {
        case ShapeType::POINT: {
            const auto& point = static_cast<const Point&>(shape);
            ImVec2 transformedPos = transformCoordinates(point.position);
            // Make points more visible with larger size
            drawList->AddCircleFilled(
                transformedPos,
                std::max(point.size * zoomLevel, 5.0f), // Ensure minimum visible size
                color,
                16
            );
            break;
        }
        case ShapeType::LINE: {
            const auto& line = static_cast<const Line&>(shape);
            ImVec2 transformedStart = transformCoordinates(line.start);
            ImVec2 transformedEnd = transformCoordinates(line.end);
            
            // Debug visualization of line endpoints
            drawList->AddCircleFilled(transformedStart, 3.0f, IM_COL32(0, 0, 0, 255)); // Black start point (was Red)
            drawList->AddCircleFilled(transformedEnd, 3.0f, IM_COL32(0, 0, 0, 255));   // Black end point (was Green)
            
            // Draw the line with increased thickness for visibility
            drawList->AddLine(
                transformedStart,
                transformedEnd,
                color,
                std::max(line.thickness * zoomLevel, 2.0f) // Ensure minimum line thickness
            );
            
            // Debug output
            std::cout << "Drawing line: (" << line.start.x << "," << line.start.y << ") to (" 
                      << line.end.x << "," << line.end.y << ") - transformed: (" 
                      << transformedStart.x << "," << transformedStart.y << ") to ("
                      << transformedEnd.x << "," << transformedEnd.y << ")" << std::endl;
            break;
        }
        case ShapeType::CIRCLE: {
            const auto& circle = static_cast<const Circle&>(shape);
            ImVec2 transformedCenter = transformCoordinates(circle.center);
            
            // Add a visible center point
            drawList->AddCircleFilled(
                transformedCenter,
                3.0f,
                IM_COL32(0, 0, 0, 255) // Black center point (was Red)
            );
            
            // Draw the circle with adequate segments and thickness
            drawList->AddCircle(
                transformedCenter,
                circle.radius * zoomLevel,
                color,
                64, // Increased segment count for smoother circles
                std::max(circle.thickness * zoomLevel, 2.0f) // Ensure minimum thickness
            );
            
            // Debug output
            std::cout << "Drawing circle: center (" << circle.center.x << "," << circle.center.y 
                      << ") radius " << circle.radius << " - transformed center: (" 
                      << transformedCenter.x << "," << transformedCenter.y 
                      << ") transformed radius: " << (circle.radius * zoomLevel) << std::endl;
            break;
        }
        case ShapeType::TRIANGLE: {
            const auto& triangle = static_cast<const Triangle&>(shape);
            
            ImVec2 transformedPoints[3];
            for (int i = 0; i < 3; ++i) {
                transformedPoints[i] = transformCoordinates(triangle.points[i]);
                // Add visible points at corners
                drawList->AddCircleFilled(transformedPoints[i], 3.0f, IM_COL32(0, 0, 0, 255));
            }
            
            // Draw triangle edges with increased thickness
            for (int i = 0; i < 3; ++i) {
                drawList->AddLine(
                    transformedPoints[i],
                    transformedPoints[(i + 1) % 3],
                    color,
                    std::max(triangle.thickness * zoomLevel, 2.0f)
                );
            }
            break;
        }
        case ShapeType::SQUARE: {
            const auto& square = static_cast<const Square&>(shape);
            ImVec2 topLeft = square.getTopLeft();
            float size = square.getSize();
            
            // Calculate transformed points for the square
            ImVec2 points[4] = {
                transformCoordinates(topLeft),
                transformCoordinates(ImVec2(topLeft.x + size, topLeft.y)),
                transformCoordinates(ImVec2(topLeft.x + size, topLeft.y + size)),
                transformCoordinates(ImVec2(topLeft.x, topLeft.y + size))
            };
            
            // Add visible points at corners
            for (int i = 0; i < 4; ++i) {
                drawList->AddCircleFilled(points[i], 3.0f, IM_COL32(0, 0, 0, 255));
            }
            
            // Draw square with thicker lines
            for (int i = 0; i < 4; ++i) {
                drawList->AddLine(
                    points[i],
                    points[(i + 1) % 4],
                    color,
                    std::max(square.thickness * zoomLevel, 2.0f)
                );
            }
            
            // Debug output
            std::cout << "Drawing square: topLeft (" << topLeft.x << "," << topLeft.y 
                      << ") size " << size << std::endl;
            break;
        }
        case ShapeType::RECTANGLE: {
            const auto& rect = static_cast<const Rectangle&>(shape);
            ImVec2 topLeft = rect.getTopLeft();
            ImVec2 size = rect.getSize();
            
            // Calculate transformed points for the rectangle
            ImVec2 points[4] = {
                transformCoordinates(topLeft),
                transformCoordinates(ImVec2(topLeft.x + size.x, topLeft.y)),
                transformCoordinates(ImVec2(topLeft.x + size.x, topLeft.y + size.y)),
                transformCoordinates(ImVec2(topLeft.x, topLeft.y + size.y))
            };
            
            // Add visible points at corners
            for (int i = 0; i < 4; ++i) {
                drawList->AddCircleFilled(points[i], 3.0f, IM_COL32(0, 0, 0, 255));
            }
            
            // Draw rectangle with thicker lines
            for (int i = 0; i < 4; ++i) {
                drawList->AddLine(
                    points[i],
                    points[(i + 1) % 4],
                    color,
                    std::max(rect.thickness * zoomLevel, 2.0f)
                );
            }
            
            // Debug output
            std::cout << "Drawing rectangle: topLeft (" << topLeft.x << "," << topLeft.y 
                      << ") size (" << size.x << "," << size.y << ")" << std::endl;
            break;
        }
        case ShapeType::SPLINE: {
            const auto& spline = static_cast<const Spline&>(shape);
            
            // Calculate spline points
            std::vector<ImVec2> points = spline.calculatePoints(0.01f);
            
            // Draw spline
            for (size_t i = 0; i < points.size() - 1; ++i) {
                drawList->AddLine(
                    transformCoordinates(points[i]),
                    transformCoordinates(points[i + 1]),
                    color,
                    std::max(spline.thickness * zoomLevel, 2.0f)
                );
            }
            
            // Draw control points if needed
            if (showControlPoints || selectedShape == &shape) {
                for (const auto& cp : spline.controlPoints) {
                    drawList->AddCircleFilled(
                        transformCoordinates(cp),
                        3.0f * zoomLevel,
                        Colors::CONTROL_POINT,
                        8
                    );
                }
                
                // Draw control polygon
                for (size_t i = 0; i < spline.controlPoints.size() - 1; ++i) {
                    drawList->AddLine(
                        transformCoordinates(spline.controlPoints[i]),
                        transformCoordinates(spline.controlPoints[i + 1]),
                        Colors::CONTROL_POINT,
                        1.0f * zoomLevel
                    );
                }
            }
            break;
        }
        case ShapeType::BEZIER: {
            const auto& bezier = static_cast<const BezierCurve&>(shape);
            
            if (bezier.controlPoints.size() < 4) {
                break;
            }
            
            // Draw the curve
            std::vector<ImVec2> points = bezier.calculatePoints(0.01f);
            
            for (size_t i = 0; i < points.size() - 1; ++i) {
                drawList->AddLine(
                    transformCoordinates(points[i]),
                    transformCoordinates(points[i + 1]),
                    color,
                    std::max(bezier.thickness * zoomLevel, 2.0f)
                );
            }
            
            // Draw control points if needed
            if (showControlPoints || selectedShape == &shape) {
                for (const auto& cp : bezier.controlPoints) {
                    drawList->AddCircleFilled(
                        transformCoordinates(cp),
                        3.0f * zoomLevel,
                        Colors::CONTROL_POINT,
                        8
                    );
                }
                
                // Draw control polygon
                for (size_t i = 0; i < bezier.controlPoints.size() - 1; ++i) {
                    drawList->AddLine(
                        transformCoordinates(bezier.controlPoints[i]),
                        transformCoordinates(bezier.controlPoints[i + 1]),
                        Colors::CONTROL_POINT,
                        1.0f * zoomLevel
                    );
                }
            }
            break;
        }
        default:
            break;
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
    if (!snapToGrid) return std::nullopt;
    
    float minDist = Constants::SNAP_THRESHOLD / zoomLevel;
    std::optional<SnapPoint> result = std::nullopt;
    
    // Check for snapping to shape features
    for (const auto& shape : shapes) {
        switch (shape->type) {
            case ShapeType::POINT: {
                const auto& point = static_cast<const Point&>(*shape);
                float dist = calculateDistance(mousePos, point.position);
                if (dist < minDist) {
                    minDist = dist;
                    result = SnapPoint{point.position, "Point"};
                }
                break;
            }
            case ShapeType::LINE: {
                const auto& line = static_cast<const Line&>(*shape);
                // Snap to endpoints
                float distStart = calculateDistance(mousePos, line.start);
                if (distStart < minDist) {
                    minDist = distStart;
                    result = SnapPoint{line.start, "Endpoint"};
                }
                float distEnd = calculateDistance(mousePos, line.end);
                if (distEnd < minDist) {
                    minDist = distEnd;
                    result = SnapPoint{line.end, "Endpoint"};
                }
                // Snap to midpoint
                ImVec2 midpoint = {
                    (line.start.x + line.end.x) * 0.5f,
                    (line.start.y + line.end.y) * 0.5f
                };
                float distMid = calculateDistance(mousePos, midpoint);
                if (distMid < minDist) {
                    minDist = distMid;
                    result = SnapPoint{midpoint, "Midpoint"};
                }
                break;
            }
            case ShapeType::CIRCLE: {
                const auto& circle = static_cast<const Circle&>(*shape);
                // Snap to center
                float distCenter = calculateDistance(mousePos, circle.center);
                if (distCenter < minDist) {
                    minDist = distCenter;
                    result = SnapPoint{circle.center, "Center"};
                }
                
                // Snap to cardinal points on the circle (N, E, S, W)
                std::vector<std::pair<ImVec2, std::string>> cardinalPoints = {
                    {{circle.center.x, circle.center.y - circle.radius}, "North"},
                    {{circle.center.x + circle.radius, circle.center.y}, "East"},
                    {{circle.center.x, circle.center.y + circle.radius}, "South"},
                    {{circle.center.x - circle.radius, circle.center.y}, "West"}
                };
                
                for (const auto& [point, name] : cardinalPoints) {
                    float dist = calculateDistance(mousePos, point);
                    if (dist < minDist) {
                        minDist = dist;
                        result = SnapPoint{point, name};
                    }
                }
                break;
            }
            case ShapeType::TRIANGLE: {
                const auto& triangle = static_cast<const Triangle&>(*shape);
                // Snap to vertices
                for (int i = 0; i < 3; ++i) {
                    float dist = calculateDistance(mousePos, triangle.points[i]);
                    if (dist < minDist) {
                        minDist = dist;
                        result = SnapPoint{triangle.points[i], "Vertex"};
                    }
                }
                
                // Snap to midpoints of edges
                for (int i = 0; i < 3; ++i) {
                    ImVec2 midpoint = {
                        (triangle.points[i].x + triangle.points[(i + 1) % 3].x) * 0.5f,
                        (triangle.points[i].y + triangle.points[(i + 1) % 3].y) * 0.5f
                    };
                    float dist = calculateDistance(mousePos, midpoint);
                    if (dist < minDist) {
                        minDist = dist;
                        result = SnapPoint{midpoint, "Midpoint"};
                    }
                }
                
                // Snap to centroid (center of mass)
                ImVec2 centroid = {
                    (triangle.points[0].x + triangle.points[1].x + triangle.points[2].x) / 3.0f,
                    (triangle.points[0].y + triangle.points[1].y + triangle.points[2].y) / 3.0f
                };
                float distCentroid = calculateDistance(mousePos, centroid);
                if (distCentroid < minDist) {
                    minDist = distCentroid;
                    result = SnapPoint{centroid, "Centroid"};
                }
                break;
            }
            case ShapeType::SQUARE: {
                const auto& square = static_cast<const Square&>(*shape);
                ImVec2 topLeft = square.getTopLeft();
                float size = square.getSize();
                
                // Create the four corners
                std::vector<ImVec2> corners = {
                    topLeft,                                    // Top-left
                    {topLeft.x + size, topLeft.y},              // Top-right
                    {topLeft.x + size, topLeft.y + size},       // Bottom-right
                    {topLeft.x, topLeft.y + size}               // Bottom-left
                };
                
                // Snap to corners
                std::vector<std::string> cornerNames = {"TopLeft", "TopRight", "BottomRight", "BottomLeft"};
                for (int i = 0; i < corners.size(); ++i) {
                    float dist = calculateDistance(mousePos, corners[i]);
                    if (dist < minDist) {
                        minDist = dist;
                        result = SnapPoint{corners[i], cornerNames[i]};
                    }
                }
                
                // Snap to midpoints of sides
                std::vector<std::string> sideNames = {"Top", "Right", "Bottom", "Left"};
                for (int i = 0; i < 4; ++i) {
                    ImVec2 midpoint = {
                        (corners[i].x + corners[(i + 1) % 4].x) * 0.5f,
                        (corners[i].y + corners[(i + 1) % 4].y) * 0.5f
                    };
                    float dist = calculateDistance(mousePos, midpoint);
                    if (dist < minDist) {
                        minDist = dist;
                        result = SnapPoint{midpoint, sideNames[i] + "Mid"};
                    }
                }
                
                // Snap to center
                ImVec2 center = {
                    topLeft.x + size * 0.5f,
                    topLeft.y + size * 0.5f
                };
                float distCenter = calculateDistance(mousePos, center);
                if (distCenter < minDist) {
                    minDist = distCenter;
                    result = SnapPoint{center, "Center"};
                }
                break;
            }
            case ShapeType::RECTANGLE: {
                const auto& rect = static_cast<const Rectangle&>(*shape);
                ImVec2 topLeft = rect.getTopLeft();
                ImVec2 size = rect.getSize();
                
                // Create the four corners
                std::vector<ImVec2> corners = {
                    topLeft,                                    // Top-left
                    {topLeft.x + size.x, topLeft.y},            // Top-right
                    {topLeft.x + size.x, topLeft.y + size.y},   // Bottom-right
                    {topLeft.x, topLeft.y + size.y}             // Bottom-left
                };
                
                // Snap to corners
                std::vector<std::string> cornerNames = {"TopLeft", "TopRight", "BottomRight", "BottomLeft"};
                for (int i = 0; i < corners.size(); ++i) {
                    float dist = calculateDistance(mousePos, corners[i]);
                    if (dist < minDist) {
                        minDist = dist;
                        result = SnapPoint{corners[i], cornerNames[i]};
                    }
                }
                
                // Snap to midpoints of sides
                std::vector<std::string> sideNames = {"Top", "Right", "Bottom", "Left"};
                for (int i = 0; i < 4; ++i) {
                    ImVec2 midpoint = {
                        (corners[i].x + corners[(i + 1) % 4].x) * 0.5f,
                        (corners[i].y + corners[(i + 1) % 4].y) * 0.5f
                    };
                    float dist = calculateDistance(mousePos, midpoint);
                    if (dist < minDist) {
                        minDist = dist;
                        result = SnapPoint{midpoint, sideNames[i] + "Mid"};
                    }
                }
                
                // Snap to center
                ImVec2 center = {
                    topLeft.x + size.x * 0.5f,
                    topLeft.y + size.y * 0.5f
                };
                float distCenter = calculateDistance(mousePos, center);
                if (distCenter < minDist) {
                    minDist = distCenter;
                    result = SnapPoint{center, "Center"};
                }
                break;
            }
            case ShapeType::SPLINE: {
                const auto& spline = static_cast<const Spline&>(*shape);
                // Snap to control points
                for (size_t i = 0; i < spline.controlPoints.size(); ++i) {
                    float dist = calculateDistance(mousePos, spline.controlPoints[i]);
                    if (dist < minDist) {
                        minDist = dist;
                        result = SnapPoint{spline.controlPoints[i], "ControlPt" + std::to_string(i)};
                    }
                }
                break;
            }
            case ShapeType::BEZIER: {
                // Handle bezier curve snapping logic
                // Add actual implementation here
                break;
            }
        }
    }
    
    // Check for grid snapping
    ImVec2 gridSnapped = getSnappedPoint(mousePos);
    float gridDist = calculateDistance(mousePos, gridSnapped);
    if (gridDist < minDist) {
        result = SnapPoint{gridSnapped, "Grid"};
    }
    
    return result;
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
    
    // Choose color based on snap point type
    ImU32 color;
    if (type == "Grid") {
        color = IM_COL32(100, 200, 100, 255); // Green for grid snaps
    } else if (type.find("Endpoint") != std::string::npos || 
               type.find("Vertex") != std::string::npos || 
               type.find("Point") != std::string::npos) {
        color = IM_COL32(240, 200, 40, 255);  // Yellow/gold for points
    } else if (type.find("Midpoint") != std::string::npos) {
        color = IM_COL32(80, 180, 240, 255);  // Blue for midpoints
    } else if (type.find("Center") != std::string::npos) {
        color = IM_COL32(240, 100, 240, 255); // Purple for centers
    } else if (type.find("North") != std::string::npos || 
               type.find("South") != std::string::npos || 
               type.find("East") != std::string::npos || 
               type.find("West") != std::string::npos) {
        color = IM_COL32(240, 120, 80, 255);  // Orange for cardinal points
    } else {
        color = IM_COL32(200, 200, 200, 255); // Default white for other types
    }
    
    // Draw different visual indicators based on type
    if (type == "Grid") {
        // Grid snap: rectangular marker
        drawList->AddRect(
            ImVec2(transformed.x - size, transformed.y - size),
            ImVec2(transformed.x + size, transformed.y + size),
            color,
            0.0f,
            ImDrawFlags_None,
            2.0f * (zoomLevel > 1.0f ? 1.0f : zoomLevel)
        );
    } else if (type.find("Endpoint") != std::string::npos || 
               type.find("Vertex") != std::string::npos || 
               type.find("Point") != std::string::npos) {
        // Point snap: diamond marker
        float diamondSize = size * 1.2f;
        drawList->AddQuad(
            ImVec2(transformed.x, transformed.y - diamondSize),
            ImVec2(transformed.x + diamondSize, transformed.y),
            ImVec2(transformed.x, transformed.y + diamondSize),
            ImVec2(transformed.x - diamondSize, transformed.y),
            color,
            2.0f * (zoomLevel > 1.0f ? 1.0f : zoomLevel)
        );
    } else if (type.find("Midpoint") != std::string::npos) {
        // Midpoint snap: cross in circle
        drawList->AddCircle(
            transformed,
            size * 1.2f,
            color,
            0,
            2.0f * (zoomLevel > 1.0f ? 1.0f : zoomLevel)
        );
        // Add cross inside
    drawList->AddLine(
        ImVec2(transformed.x - size, transformed.y),
        ImVec2(transformed.x + size, transformed.y),
            color,
            2.0f * (zoomLevel > 1.0f ? 1.0f : zoomLevel)
    );
    drawList->AddLine(
        ImVec2(transformed.x, transformed.y - size),
        ImVec2(transformed.x, transformed.y + size),
            color,
            2.0f * (zoomLevel > 1.0f ? 1.0f : zoomLevel)
        );
    } else if (type.find("Center") != std::string::npos) {
        // Center snap: concentric circles
        drawList->AddCircle(
            transformed,
            size * 1.5f,
            color,
            0,
            2.0f * (zoomLevel > 1.0f ? 1.0f : zoomLevel)
        );
        drawList->AddCircle(
            transformed,
            size * 0.7f,
            color,
            0,
            2.0f * (zoomLevel > 1.0f ? 1.0f : zoomLevel)
        );
    } else {
        // Default: cross
        drawList->AddLine(
            ImVec2(transformed.x - size, transformed.y),
            ImVec2(transformed.x + size, transformed.y),
            color,
            2.0f * (zoomLevel > 1.0f ? 1.0f : zoomLevel)
        );
        drawList->AddLine(
            ImVec2(transformed.x, transformed.y - size),
            ImVec2(transformed.x, transformed.y + size),
            color,
            2.0f * (zoomLevel > 1.0f ? 1.0f : zoomLevel)
        );
    }
    
    // Add a semi-transparent background for better text readability
    ImVec2 textSize = ImGui::CalcTextSize(type.c_str());
    ImVec2 textPos = ImVec2(transformed.x + size + 5.0f, transformed.y - textSize.y/2);
    
    drawList->AddRectFilled(
        ImVec2(textPos.x - 2, textPos.y - 2),
        ImVec2(textPos.x + textSize.x + 2, textPos.y + textSize.y + 2),
        IM_COL32(40, 40, 40, 180), // Dark semi-transparent background
        3.0f
    );
    
    // Draw the type text with the same color as the indicator
    drawList->AddText(textPos, color, type.c_str());
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
    
    // Default to returning grid-snapped position
    ImVec2 result = getSnappedPoint(pos);
    float minDist = calculateDistance(pos, result);
    
    // Get best shape snap point if available
    if (auto snapPoint = findSnapPoint(pos)) {
        float snapDist = calculateDistance(pos, snapPoint->point);
        if (snapDist < minDist) {
            result = snapPoint->point;
        }
    }
    
    return result;
}

} // namespace Drawing 