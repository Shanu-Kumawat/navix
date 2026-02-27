#include <glm/glm.hpp>
#include "Canvas.hpp"
#include "utils/MathUtils.hpp"
#include "imgui.h"
#include <iostream>
#include <memory>
#include <cmath>
#include <sstream>
#include <iomanip>
#include "shapes/ShockAbsorberBottomEnd.hpp"

namespace Drawing {
using namespace Drawing::Math;

using Math::calculateDistance;
using Math::calculateTriangleArea;
using Math::calculateMidpoint;
using Math::calculateAngle;
using Math::snapToGrid;
using Math::isPointInRect;
using Math::isPointNearPoint;
using glm::normalizeVector;
using glm::dotProduct;
using Math::rotatePoint;

// ... existing code ...

Canvas::Canvas() : 
    renderer(std::make_unique<Core::Renderer2D>(this)),
    sceneModel(std::make_unique<Core::SceneModel>()),
    inputController(std::make_unique<Core::InputController>(this, sceneModel.get(), nullptr)),
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
    // Add initial state with current view settings
    HistoryState initialState;
    initialState.panOffset = panOffset;
    initialState.zoomLevel = zoomLevel;
    initialState.showGrid = showGrid;
    history.push_back(std::move(initialState)); // Initial empty state
}

void Canvas::setDrawingMode(DrawingMode mode) {
    currentMode = mode;
    isDrawing = (mode == DrawingMode::Spring2D || mode == DrawingMode::Bellows); // Set for both Spring2D and Bellows
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
    
    // Cache mouse position to avoid multiple transformations
    glm::dvec2 mousePos = Drawing::Math::toDVec2(ImGui::GetMousePos());
    glm::dvec2 transformedMousePos = inverseTransformCoordinates(mousePos);
    
    // Handle mouse wheel for zooming - only if significant change
    if (std::abs(io.MouseWheel) > 0.01f) {
        updateZoom(io.MouseWheel);
    }
    
    // Handle middle mouse button for panning - use delta directly
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
        updatePan(io.MouseDelta);
        isDraggingCanvas = true;
    } else {
        isDraggingCanvas = false;
    }
    
    // Handle drawing or selection based on current mode - only if not dragging canvas
    if (!isDraggingCanvas) {
        if (currentMode == DrawingMode::Select) {
            handleSelection(transformedMousePos);
        } else {
            handleDrawing(transformedMousePos);
        }
    }
}

void Canvas::handleSelection(const glm::dvec2& mousePos) {
    // Handle selection of shapes
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        clearSelection(); // Use our new method to properly clear selection
        
        std::cout << "Selection click at: (" << mousePos.x << ", " << mousePos.y << ")\n";
        
        // First check specifically for bellows shapes as they can be harder to select
        bool bellowsSelected = false;
        for (auto it = shapes.rbegin(); it != shapes.rend(); ++it) {
            auto& shape = *it;
            if (shape->type == ShapeType::BELLOWS) {
                if (shape->isPointNear(mousePos, Constants::SNAP_THRESHOLD / zoomLevel)) {
                    selectedShape = shape.get();
                    static_cast<Bellows*>(selectedShape)->isSelected = true;
                    bellowsSelected = true;
                    break;
                }
            }
        }
        
        // If no bellows was selected, try other shapes
        if (!bellowsSelected) {
            // Check all shapes for selection - prioritize Spring2D and shock absorber components
            // First pass: check Spring2D specifically (they should be selectable even when overlapped by ends)
            for (auto it = shapes.rbegin(); it != shapes.rend(); ++it) {
                auto& shape = *it;
                if (shape->type == ShapeType::SPRING2D) {
                    auto* spring = static_cast<Spring2D*>(shape.get());
                    if (spring->isPointInBoundingBox(mousePos, Constants::SNAP_THRESHOLD / zoomLevel)) {
                        selectedShape = shape.get();
                        std::cout << "Spring2D selected at (" << spring->centerX << ", " << spring->centerY << ")!\n";
                        break;
                    }
                }
            }
            
            // If no spring was selected, try shock absorber ends and other shapes
            if (!selectedShape) {
                for (auto it = shapes.rbegin(); it != shapes.rend(); ++it) {
                    auto& shape = *it;
                    if (shape->type == ShapeType::SPRING2D) {
                        continue; // Already checked
                    }
                    if (shape->isPointNear(mousePos, Constants::SNAP_THRESHOLD / zoomLevel)) {
                        selectedShape = shape.get();
                        std::cout << "Shape selected, type: " << static_cast<int>(shape->type) << "\n";
                        // Set specific selection state for bellows
                        if (selectedShape->type == ShapeType::BELLOWS) {
                            static_cast<Bellows*>(selectedShape)->isSelected = true;
                        } else if (selectedShape->type == ShapeType::BALL_BEARING) {
                            static_cast<BallBearing*>(selectedShape)->isSelected = true;
                        }
                        break;
                    }
                }
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


void Canvas::handleDrawing(const glm::dvec2& mousePos) {
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
        case DrawingMode::Bellows:
            handleBellowsDrawing(drawList, mousePos);
            break;
        case DrawingMode::BallBearing:
            handleBallBearingDrawing(drawList, mousePos);
            break;
        case DrawingMode::Spring2D:
            handleSpring2DDrawing(drawList, mousePos);
            break;
        default:
            break;
    }
}

void Canvas::update(const glm::dvec2& mousePos) {
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

void Canvas::updateSelectedShape(const glm::dvec2& mousePos) {
    if (!selectedShape) return;

    if (selectedShape->type == ShapeType::SPLINE || selectedShape->type == ShapeType::BEZIER) {
        handleCurveManipulation(mousePos);
    }
}

void Canvas::updateDrawingPreview(const glm::dvec2& mousePos) {
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
        clearSelection(); // Use our new method
        
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
            glm::dvec2 newPos = {point.position.x + 10.0f, point.position.y + 10.0f};
            newShape = std::make_unique<Point>(newPos, point.color, point.size);
            break;
        }
        case ShapeType::LINE: {
            const auto& line = static_cast<const Line&>(*selectedShape);
            glm::dvec2 offset = {10.0f, 10.0f};
            glm::dvec2 newStart = {line.start.x + offset.x, line.start.y + offset.y};
            glm::dvec2 newEnd = {line.end.x + offset.x, line.end.y + offset.y};
            newShape = std::make_unique<Line>(newStart, newEnd, line.color, line.thickness);
            break;
        }
        case ShapeType::BELLOWS: {
            const auto& bellows = static_cast<const Bellows&>(*selectedShape);
            auto newBellows = std::make_unique<Bellows>(bellows.color, bellows.thickness);
            
            // Copy all parameters
            newBellows->cuffAInnerDiameter = bellows.cuffAInnerDiameter;
            newBellows->cuffBInnerDiameter = bellows.cuffBInnerDiameter;
            newBellows->cuffALength = bellows.cuffALength;
            newBellows->cuffBLength = bellows.cuffBLength;
            newBellows->baseConvolutionDiameter = bellows.baseConvolutionDiameter;
            newBellows->peakConvolutionDiameter = bellows.peakConvolutionDiameter;
            newBellows->convolutedSectionLength = bellows.convolutedSectionLength;
            newBellows->numConvolutions = bellows.numConvolutions;
            newBellows->wallThickness = bellows.wallThickness;
            newBellows->showDimensions = bellows.showDimensions;
            
            // Position the duplicate slightly offset from the original
            // We'll need to implement translation for bellows later
            
            newShape = std::move(newBellows);
            break;
        }
        case ShapeType::BALL_BEARING: {
            const auto& ballBearing = static_cast<const BallBearing&>(*selectedShape);
            auto newBallBearing = std::make_unique<BallBearing>(ballBearing.color, ballBearing.thickness);
            
            // Copy all parameters
            newBallBearing->outerDiameter = ballBearing.outerDiameter;
            newBallBearing->innerDiameter = ballBearing.innerDiameter;
            newBallBearing->width = ballBearing.width;
            newBallBearing->ballDiameter = ballBearing.ballDiameter;
            newBallBearing->numBalls = ballBearing.numBalls;
            newBallBearing->raceRadius = ballBearing.raceRadius;
            newBallBearing->contactAngle = ballBearing.contactAngle;
            newBallBearing->showBalls = ballBearing.showBalls;
            newBallBearing->showCage = ballBearing.showCage;
            newBallBearing->showDimensions = ballBearing.showDimensions;
            
            // Position the duplicate slightly offset from the original
            newBallBearing->position = glm::dvec2(ballBearing.position.x + 50.0f, ballBearing.position.y + 50.0f);
            newBallBearing->angle = ballBearing.angle;
            
            newShape = std::move(newBallBearing);
            break;
        }
        // ... similar cases for other shape types ...
    }
    
    if (newShape) {
        shapes.push_back(std::move(newShape));
        selectedShape = shapes.back().get();
    }
}

void Canvas::moveSelectedShape(const glm::dvec2& delta) {
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
        case ShapeType::BELLOWS: {
            // For bellows, we would need to implement movement
            // by translating all profile points
            // This will be implemented later, as bellows don't have a 
            // direct translation method yet
            break;
        }
        case ShapeType::BALL_BEARING: {
            auto& ballBearing = static_cast<BallBearing&>(*selectedShape);
            ballBearing.position.x += delta.x;
            ballBearing.position.y += delta.y;
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
    
    // Add view state to history
    state.panOffset = panOffset;
    state.zoomLevel = zoomLevel;
    state.showGrid = showGrid;
    
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
    
    // Restore view settings
    panOffset = state.panOffset;
    zoomLevel = state.zoomLevel;
    showGrid = state.showGrid;
}

void Canvas::updateZoom(float delta) {
    // Store old zoom level for calculations
    float oldZoom = zoomLevel;
    
    // Update zoom level with limits, reduced sensitivity by multiplying delta by 0.1
    zoomLevel = std::clamp(zoomLevel + delta * 0.1f, 0.1f, 10.0f);
    
    // Get the current mouse position in screen space
    glm::dvec2 mousePos = Drawing::Math::toDVec2(ImGui::GetMousePos());
    
    // Convert mouse position to canvas space (relative to canvas origin)
    glm::dvec2 mouseInCanvas = {
        (mousePos.x - windowX - panOffset.x) / oldZoom,
        (mousePos.y - windowY - panOffset.y) / oldZoom
    };
    
    // Calculate new pan offset to keep mouse position fixed in world space
    panOffset.x = mousePos.x - windowX - mouseInCanvas.x * zoomLevel;
    panOffset.y = mousePos.y - windowY - mouseInCanvas.y * zoomLevel;
}

void Canvas::updatePan(const glm::dvec2& delta) {
    panOffset.x += delta.x;
    panOffset.y += delta.y;
}

glm::dvec2 Canvas::transformCoordinates(const glm::dvec2& point) const {
    return glm::dvec2{
        point.x * zoomLevel + panOffset.x,
        point.y * zoomLevel + panOffset.y
    };
}

glm::dvec2 Canvas::inverseTransformCoordinates(const glm::dvec2& point) const {
    return glm::dvec2{
        (point.x - panOffset.x) / zoomLevel,
        (point.y - panOffset.y) / zoomLevel
    };
}





void Canvas::handleCurveManipulation(const glm::dvec2& mousePos) {
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
void Canvas::handlePointDrawing(ImDrawList* drawList, const glm::dvec2& currentPos) {
    glm::dvec2 snappedPos = findNearestSnapPoint(currentPos);
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered()) {
        // Create and add the new shape FIRST
        shapes.push_back(std::make_unique<Point>(snappedPos));
        std::cout << "Point added at (" << snappedPos.x << ", " << snappedPos.y << ")\n";
        
        // THEN save the history with the new shape included
        saveToHistory();
    }
}

void Canvas::handleLineDrawing(ImDrawList* drawList, const glm::dvec2& currentPos) {
    glm::dvec2 snappedPos = findNearestSnapPoint(currentPos);
    
    // Display snap indicator for better visual feedback
    if (auto snapPoint = findSnapPoint(currentPos)) {
        renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
    }
    
    if (fixedLineLength) {
        // Fixed length mode - two click drawing
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            if (isDrawing) {
                // Second click sets the direction
                endPoint = snappedPos;
                
                // Use fixed length from property panel
                glm::dvec2 direction = normalizeVector(glm::dvec2(endPoint.x - startPoint.x, endPoint.y - startPoint.y));
                glm::dvec2 actualEnd = glm::dvec2(
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
                    renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
                }
            }
        }
        
        if (isDrawing) {
            // Preview with fixed length
            glm::dvec2 direction = normalizeVector(glm::dvec2(currentPos.x - startPoint.x, currentPos.y - startPoint.y));
            glm::dvec2 previewEnd = glm::dvec2(
                startPoint.x + direction.x * lineLength,
                startPoint.y + direction.y * lineLength
            );
            renderer->previewLine(drawList, startPoint, previewEnd);
            
            // Show snap indicator at the start point for continuous feedback
            if (auto snapPoint = findSnapPoint(startPoint)) {
                renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
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
                renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
            }
        }
        else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && isDrawing) {
            isDrawing = false; // Fix: set before adding the shape
            endPoint = snappedPos;
            // Calculate length
            float length = calculateDistance(startPoint, endPoint);
            // Only create if line is long enough
            if (length > Constants::MIN_SHAPE_SIZE) {
                shapes.push_back(std::make_unique<Line>(startPoint, endPoint));
                saveToHistory();
            }
        }
        
        if (isDrawing) {
            // Preview line while dragging
            renderer->previewLine(drawList, startPoint, snappedPos);
            
            // Show snap indicators at both the start and end points
            if (auto snapPoint = findSnapPoint(startPoint)) {
                renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
            }
        }
    }
}

void Canvas::handleCircleDrawing(ImDrawList* drawList, const glm::dvec2& currentPos) {
    glm::dvec2 snappedPos = findNearestSnapPoint(currentPos);
    
    // Display snap indicator for better visual feedback
    if (auto snapPoint = findSnapPoint(currentPos)) {
        renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
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
                    renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
                }
            }
        }
        
        if (isDrawing) {
            // Preview with fixed radius
            renderer->previewCircle(drawList, startPoint, circleRadius);
            
            // Show snap indicator at the center point for continuous feedback
            if (auto snapPoint = findSnapPoint(startPoint)) {
                renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
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
                renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
            }
        }
        else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && isDrawing) {
            isDrawing = false; // Fix: set before adding the shape
            // Calculate radius from center to mouse position
            float radius = calculateDistance(startPoint, snappedPos);
            // Only create if radius is large enough
            if (radius > Constants::MIN_SHAPE_SIZE) {
                shapes.push_back(std::make_unique<Circle>(startPoint, radius));
                saveToHistory();
            }
        }
        
        if (isDrawing) {
            // Preview circle while dragging
            endPoint = snappedPos;
            float radius = calculateDistance(startPoint, endPoint);
            renderer->previewCircle(drawList, startPoint, radius);
            
            // Show snap indicators at both the center and current radius point
            if (auto snapPoint = findSnapPoint(startPoint)) {
                renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
            }
        }
    }
}

void Canvas::handleTriangleDrawing(ImDrawList* drawList, const glm::dvec2& currentPos) {
    glm::dvec2 snappedPos = findNearestSnapPoint(currentPos);
    
    // Display snap indicator for better visual feedback
    if (auto snapPoint = findSnapPoint(currentPos)) {
        renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
    }
    
    if (!isDrawing) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            isDrawing = true;
            startPoint = snappedPos;
            clickCount = 1;
            
            // Show snap indicator at the start point
            if (auto snapPoint = findSnapPoint(startPoint)) {
                renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
            }
        }
    } else {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (isFixedTriangleSize()) {
                // Fixed size mode: first click sets position, second click sets direction
                if (clickCount == 1) {
                    // Calculate direction vector
                    glm::dvec2 direction = glm::dvec2(snappedPos.x - startPoint.x, snappedPos.y - startPoint.y);
                    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                    if (length > 0.0f) {
                        direction = glm::dvec2(direction.x / length, direction.y / length);
                        
                        // Calculate the three points of the equilateral triangle
                        float height = triangleSide * std::sqrt(3.0f) / 2.0f;
                        glm::dvec2 p1 = startPoint;
                        glm::dvec2 p2 = glm::dvec2(startPoint.x + triangleSide * direction.x,
                                         startPoint.y + triangleSide * direction.y);
                        glm::dvec2 p3 = glm::dvec2(startPoint.x + triangleSide * 0.5f * direction.x - height * direction.y,
                                         startPoint.y + triangleSide * 0.5f * direction.y + height * direction.x);
                        
                        // Draw the triangle
                        std::array<glm::dvec2, 3> points = {p1, p2, p3};
                        shapes.push_back(std::make_unique<Triangle>(points));
                        saveToHistory();
                        isDrawing = false;
                        clickCount = 0;
                    }
                }
            } else {
                // Dynamic size mode: drag to set size
                glm::dvec2 direction = glm::dvec2(snappedPos.x - startPoint.x, snappedPos.y - startPoint.y);
                float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                if (length > 0.0f) {
                    direction = glm::dvec2(direction.x / length, direction.y / length);
                    
                    // Calculate the three points of the equilateral triangle
                    float height = length * std::sqrt(3.0f) / 2.0f;
                    glm::dvec2 p1 = startPoint;
                    glm::dvec2 p2 = glm::dvec2(startPoint.x + length * direction.x,
                                     startPoint.y + length * direction.y);
                    glm::dvec2 p3 = glm::dvec2(startPoint.x + length * 0.5f * direction.x - height * direction.y,
                                     startPoint.y + length * 0.5f * direction.y + height * direction.x);
                    
                    // Draw preview
                    std::array<glm::dvec2, 3> previewPoints = {p1, p2, p3};
                    renderer->previewTriangle(drawList, previewPoints, 3);
                    
                    // Show snap indicators at vertices and midpoints
                    for (const auto& point : {p1, p2, p3}) {
                        if (auto snapPoint = findSnapPoint(point)) {
                            renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
                        }
                    }
                    
                    // Show snap indicators at midpoints of edges
                    for (int i = 0; i < 3; ++i) {
                        glm::dvec2 midpoint = {
                            (previewPoints[i].x + previewPoints[(i + 1) % 3].x) * 0.5f,
                            (previewPoints[i].y + previewPoints[(i + 1) % 3].y) * 0.5f
                        };
                        if (auto snapPoint = findSnapPoint(midpoint)) {
                            renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
                        }
                    }
                }
            }
        } else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            if (!isFixedTriangleSize()) {
                // Complete the triangle in dynamic mode
                glm::dvec2 direction = glm::dvec2(snappedPos.x - startPoint.x, snappedPos.y - startPoint.y);
                float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                if (length > Constants::MIN_SHAPE_SIZE) {
                    direction = glm::dvec2(direction.x / length, direction.y / length);
                    
                    // Calculate the three points of the equilateral triangle
                    float height = length * std::sqrt(3.0f) / 2.0f;
                    glm::dvec2 p1 = startPoint;
                    glm::dvec2 p2 = glm::dvec2(startPoint.x + length * direction.x,
                                     startPoint.y + length * direction.y);
                    glm::dvec2 p3 = glm::dvec2(startPoint.x + length * 0.5f * direction.x - height * direction.y,
                                     startPoint.y + length * 0.5f * direction.y + height * direction.x);
                    
                    // Draw the triangle
                    std::array<glm::dvec2, 3> points = {p1, p2, p3};
                    shapes.push_back(std::make_unique<Triangle>(points));
                    saveToHistory();
                }
                isDrawing = false;
            }
        }
    }
}

void Canvas::handleSquareDrawing(ImDrawList* drawList, const glm::dvec2& currentPos) {
    glm::dvec2 snappedPos = findNearestSnapPoint(currentPos);
    
    // Display snap indicator for better visual feedback
    if (auto snapPoint = findSnapPoint(currentPos)) {
        renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
    }
    
    if (!isDrawing) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            isDrawing = true;
            startPoint = snappedPos;
            clickCount = 1;
            
            // Show snap indicator at the start point
            if (auto snapPoint = findSnapPoint(startPoint)) {
                renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
            }
        }
    } else {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (isFixedSquareSize()) {
                // Fixed size mode: first click sets position, second click sets direction
                if (clickCount == 1) {
                    // Calculate direction vector
                    glm::dvec2 direction = glm::dvec2(snappedPos.x - startPoint.x, snappedPos.y - startPoint.y);
                    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                    if (length > 0.0f) {
                        direction = glm::dvec2(direction.x / length, direction.y / length);
                        
                        // Calculate the four corners of the square
                        glm::dvec2 p1 = startPoint;
                        glm::dvec2 p2 = glm::dvec2(startPoint.x + squareSize * direction.x,
                                         startPoint.y + squareSize * direction.y);
                        glm::dvec2 p3 = glm::dvec2(p2.x - squareSize * direction.y,
                                         p2.y + squareSize * direction.x);
                        glm::dvec2 p4 = glm::dvec2(p1.x - squareSize * direction.y,
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
                
                glm::dvec2 p1 = startPoint;
                glm::dvec2 p2 = glm::dvec2(startPoint.x + size * signX, startPoint.y);
                glm::dvec2 p3 = glm::dvec2(startPoint.x + size * signX, startPoint.y + size * signY);
                glm::dvec2 p4 = glm::dvec2(startPoint.x, startPoint.y + size * signY);
                
                // Draw preview
                renderer->previewSquare(drawList, startPoint, snappedPos);
                
                // Show snap indicators at corners and midpoints
                for (const auto& point : {p1, p2, p3, p4}) {
                    if (auto snapPoint = findSnapPoint(point)) {
                        renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
                    }
                }
                
                // Show snap indicators at midpoints of sides
                for (int i = 0; i < 4; ++i) {
                    glm::dvec2 midpoint = {
                        (p1.x + p2.x) * 0.5f,
                        (p1.y + p2.y) * 0.5f
                    };
                    if (auto snapPoint = findSnapPoint(midpoint)) {
                        renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
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
                    
                    glm::dvec2 p1 = startPoint;
                    glm::dvec2 p2 = glm::dvec2(startPoint.x + size * signX, startPoint.y);
                    glm::dvec2 p3 = glm::dvec2(startPoint.x + size * signX, startPoint.y + size * signY);
                    glm::dvec2 p4 = glm::dvec2(startPoint.x, startPoint.y + size * signY);
                    
                    shapes.push_back(std::make_unique<Square>(p1, p2));
                    saveToHistory();
                }
                isDrawing = false;
            }
        }
    }
}

void Canvas::handleRectangleDrawing(ImDrawList* drawList, const glm::dvec2& currentPos) {
    glm::dvec2 snappedPos = findNearestSnapPoint(currentPos);
    
    // Display snap indicator for better visual feedback
    if (auto snapPoint = findSnapPoint(currentPos)) {
        renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
    }
    
    if (!isDrawing) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            isDrawing = true;
            startPoint = snappedPos;
            clickCount = 1;
            
            // Show snap indicator at the start point
            if (auto snapPoint = findSnapPoint(startPoint)) {
                renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
            }
        }
    } else {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (isFixedRectangleSize()) {
                // Fixed size mode: first click sets position, second click sets direction
                if (clickCount == 1) {
                    // Calculate direction vector
                    glm::dvec2 direction = glm::dvec2(snappedPos.x - startPoint.x, snappedPos.y - startPoint.y);
                    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                    if (length > 0.0f) {
                        direction = glm::dvec2(direction.x / length, direction.y / length);
                        
                        // Calculate the four corners of the rectangle
                        glm::dvec2 p1 = startPoint;
                        glm::dvec2 p2 = glm::dvec2(startPoint.x + rectangleWidth * direction.x,
                                         startPoint.y + rectangleWidth * direction.y);
                        glm::dvec2 p3 = glm::dvec2(p2.x - rectangleHeight * direction.y,
                                         p2.y + rectangleHeight * direction.x);
                        glm::dvec2 p4 = glm::dvec2(p1.x - rectangleHeight * direction.y,
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
                
                glm::dvec2 p1 = startPoint;
                glm::dvec2 p2 = glm::dvec2(startPoint.x + width * signX, startPoint.y);
                glm::dvec2 p3 = glm::dvec2(startPoint.x + width * signX, startPoint.y + height * signY);
                glm::dvec2 p4 = glm::dvec2(startPoint.x, startPoint.y + height * signY);
                
                // Draw preview
                renderer->previewRectangle(drawList, startPoint, snappedPos);
                
                // Show snap indicators at corners and midpoints
                for (const auto& point : {p1, p2, p3, p4}) {
                    if (auto snapPoint = findSnapPoint(point)) {
                        renderer->renderSnapIndicator(drawList, snapPoint->point, snapPoint->type);
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
                    
                    glm::dvec2 p1 = startPoint;
                    glm::dvec2 p2 = glm::dvec2(startPoint.x + width * signX, startPoint.y);
                    glm::dvec2 p3 = glm::dvec2(startPoint.x + width * signX, startPoint.y + height * signY);
                    glm::dvec2 p4 = glm::dvec2(startPoint.x, startPoint.y + height * signY);
                    
                    shapes.push_back(std::make_unique<Rectangle>(p1, p2, p3, p4));
                    saveToHistory();
                }
                isDrawing = false;
            }
        }
    }
}

void Canvas::handleSplineDrawing(ImDrawList* drawList, const glm::dvec2& currentPos) {
    glm::dvec2 snappedPos = findNearestSnapPoint(currentPos);
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

void Canvas::handleBezierDrawing(ImDrawList* drawList, const glm::dvec2& currentPos) {
    glm::dvec2 snappedPos = findNearestSnapPoint(currentPos);
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

void Canvas::handleBellowsDrawing(ImDrawList* drawList, const glm::dvec2& currentPos) {
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered()) {
        if (isFirstClick) {
            startPoint = getSnappedPoint(currentPos);
            isFirstClick = false;
            isDrawing = true;
        } else {
            endPoint = getSnappedPoint(currentPos);
            float dx = endPoint.x - startPoint.x;
            float dy = endPoint.y - startPoint.y;
            float length = std::sqrt(dx * dx + dy * dy);
            auto bellows = std::make_unique<Bellows>();
            float minLength = bellows->cuffALength + bellows->cuffBLength + 10.0f;
            if (length > minLength) {
                bellows->convolutedSectionLength = length - bellows->cuffALength - bellows->cuffBLength;
                bellows->invalidateCache();
                bellows->position = startPoint;
                bellows->angle = std::atan2(dy, dx);
                
                std::cout << "[DEBUG] Creating bellows: length=" << length 
                         << " convolutedSectionLength=" << bellows->convolutedSectionLength
                         << " isValid=" << bellows->isValid() << std::endl;
                
                selectedShape = bellows.get(); // Select the new bellows
                shapes.push_back(std::move(bellows));
                saveToHistory();
                isFirstClick = true;
                isDrawing = false;
            } else {
                std::cout << "[DEBUG] Bellows too short: length=" << length 
                         << " minLength=" << minLength << std::endl;
                // Optionally: show a message to the user
                // Do NOT reset state; allow the user to try again
            }
        }
    }
}











void Canvas::drawShape(const Shape& shape) const {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    // Use the shape's own color (black from Constants.hpp) instead of hardcoding white
    ImU32 color = (selectedShape == &shape) ? IM_COL32(255, 255, 0, 255) : shape.color;
    
    switch (shape.type) {
        case ShapeType::POINT: {
            const auto& point = static_cast<const Point&>(shape);
            glm::dvec2 transformedPos = transformCoordinates(point.position);
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
            glm::dvec2 transformedStart = transformCoordinates(line.start);
            glm::dvec2 transformedEnd = transformCoordinates(line.end);
            
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
            glm::dvec2 transformedCenter = transformCoordinates(circle.center);
            
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
            
            glm::dvec2 transformedPoints[3];
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
            glm::dvec2 topLeft = square.getTopLeft();
            float size = square.getSize();
            
            // Calculate transformed points for the square
            glm::dvec2 points[4] = {
                transformCoordinates(topLeft),
                transformCoordinates(glm::dvec2(topLeft.x + size, topLeft.y)),
                transformCoordinates(glm::dvec2(topLeft.x + size, topLeft.y + size)),
                transformCoordinates(glm::dvec2(topLeft.x, topLeft.y + size))
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
            glm::dvec2 topLeft = rect.getTopLeft();
            glm::dvec2 size = rect.getSize();
            
            // Calculate transformed points for the rectangle
            glm::dvec2 points[4] = {
                transformCoordinates(topLeft),
                transformCoordinates(glm::dvec2(topLeft.x + size.x, topLeft.y)),
                transformCoordinates(glm::dvec2(topLeft.x + size.x, topLeft.y + size.y)),
                transformCoordinates(glm::dvec2(topLeft.x, topLeft.y + size.y))
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
            std::vector<glm::dvec2> points = spline.calculatePoints(0.01f);
            
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
            std::vector<glm::dvec2> points = bezier.calculatePoints(0.01f);
            
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
        case ShapeType::SPRING2D: {
            const auto& spring = static_cast<const Spring2D&>(shape);
            // Draw a 2D spring as a sine wave helix
            int pointsPerCoil = 40;
            int totalPoints = spring.numCoils * pointsPerCoil;
            float halfLength = spring.freeLength / 2.0f;
            float radius = spring.outerDiameter / 2.0f - spring.wireDiameter / 2.0f;
            std::vector<glm::dvec2> points;
            points.reserve(totalPoints);
            for (int i = 0; i < totalPoints; ++i) {
                float t = (float)i / (totalPoints - 1);
                float y = spring.centerY - halfLength + t * spring.freeLength;
                float angle = t * spring.numCoils * 2.0f * M_PI;
                float x = spring.centerX + radius * std::sin(angle);
                points.emplace_back(x, y);
            }
            for (int i = 0; i < (int)points.size() - 1; ++i) {
                glm::dvec2 p1 = transformCoordinates(points[i]);
                glm::dvec2 p2 = transformCoordinates(points[i + 1]);
                drawList->AddLine(p1, p2, color, std::max(spring.wireDiameter * zoomLevel, 2.0f));
            }
            break;
        }
        case ShapeType::SHOCK_ABSORBER_END_2D: {
            const auto& end = static_cast<const ShockAbsorberEnd2D&>(shape);
            float x = end.baseCenter.x;
            float od = end.parentSpring->outerDiameter;
            float wd = end.parentSpring->wireDiameter;
            float fl = end.parentSpring->freeLength;
            
            // New dimensions matching the 3D model reference
            float damperTubeWidth = wd * 1.2f;  // Thicker outer tube from top
            float pistonRodWidth = wd * 0.5f;  // Thin central rod from bottom
            float collarHeight = fl * 0.05f;
            float collarWidth = od * 0.5f;
            float nutHeight = fl * 0.08f;
            float nutWidth = od * 0.4f;
            float capHeight = fl * 0.06f;
            float capWidth = od * 0.35f;
            float springSeatWidth = od * 1.05f;
            float springSeatThickness = fl * 0.035f;
            
            if (end.position == EndPosition::Top) {
                // Top end: spring seat, damper tube going down into spring, collar, nut, and cap going upward
                float springTop = end.parentSpring->centerY - fl / 2;
                float springBottom = end.parentSpring->centerY + fl / 2;
                
                // Spring seat (wide flange at top of spring)
                float seatBottom = springTop;
                float seatTop = seatBottom - springSeatThickness;
                glm::dvec2 seatL(x - springSeatWidth/2, seatTop);
                glm::dvec2 seatR(x + springSeatWidth/2, seatBottom);
                drawList->AddRect(transformCoordinates(seatL), transformCoordinates(seatR), end.color, 0, 0, end.thickness);
                
                // Damper tube (thicker tube extending DOWN into spring area - 70% into spring)
                float tubeTop = seatBottom;
                float tubeBottom = springTop + fl * 0.7f;  // Goes 70% down into spring
                glm::dvec2 tubeL(x - damperTubeWidth/2, tubeTop);
                glm::dvec2 tubeR(x + damperTubeWidth/2, tubeBottom);
                drawList->AddRect(transformCoordinates(tubeL), transformCoordinates(tubeR), IM_COL32(160,160,160,255), 0, 0, end.thickness);
                
                // Piston rod from bottom (thinner rod extending UP into spring area - shown as dashed or different color)
                // This represents the rod coming from the bottom mount
                float rodBottom = springBottom;
                float rodTop = springTop + fl * 0.3f;  // Goes 70% up into spring (overlaps with tube)
                glm::dvec2 rodL(x - pistonRodWidth/2, rodTop);
                glm::dvec2 rodR(x + pistonRodWidth/2, rodBottom);
                drawList->AddRect(transformCoordinates(rodL), transformCoordinates(rodR), IM_COL32(200,200,200,255), 0, 0, end.thickness);
                
                // Collar (wider section above spring seat, below nut)
                float collarBottom = seatTop;
                float collarTop = collarBottom - collarHeight;
                glm::dvec2 collarL(x - collarWidth/2, collarTop);
                glm::dvec2 collarR(x + collarWidth/2, collarBottom);
                drawList->AddRect(transformCoordinates(collarL), transformCoordinates(collarR), end.color, 0, 0, end.thickness);
                
                // Nut (hexagonal shape - in 2D shown as rectangle with diagonal lines)
                float nutBottom = collarTop;
                float nutTop = nutBottom - nutHeight;
                glm::dvec2 nutL(x - nutWidth/2, nutTop);
                glm::dvec2 nutR(x + nutWidth/2, nutBottom);
                drawList->AddRect(transformCoordinates(nutL), transformCoordinates(nutR), end.color, 0, 0, end.thickness);
                // Draw diagonal lines to indicate hex nut (cross-hatch pattern)
                float hexOffset = nutWidth * 0.3f;
                glm::dvec2 hexL1(x - hexOffset, nutTop);
                glm::dvec2 hexL2(x - nutWidth/2, nutTop + nutHeight * 0.3f);
                glm::dvec2 hexR1(x + hexOffset, nutTop);
                glm::dvec2 hexR2(x + nutWidth/2, nutTop + nutHeight * 0.3f);
                drawList->AddLine(transformCoordinates(hexL1), transformCoordinates(hexL2), end.color, end.thickness);
                drawList->AddLine(transformCoordinates(hexR1), transformCoordinates(hexR2), end.color, end.thickness);
                glm::dvec2 hexL3(x - hexOffset, nutBottom);
                glm::dvec2 hexL4(x - nutWidth/2, nutBottom - nutHeight * 0.3f);
                glm::dvec2 hexR3(x + hexOffset, nutBottom);
                glm::dvec2 hexR4(x + nutWidth/2, nutBottom - nutHeight * 0.3f);
                drawList->AddLine(transformCoordinates(hexL3), transformCoordinates(hexL4), end.color, end.thickness);
                drawList->AddLine(transformCoordinates(hexR3), transformCoordinates(hexR4), end.color, end.thickness);
                
                // Cross-bars/handles extending from nut (horizontal mounting points)
                float nutMid = (nutTop + nutBottom) / 2.0f;
                float barExtend = nutWidth * 1.5f;  // How far bars extend
                float barThick = nutHeight * 0.15f;
                // Left bar
                glm::dvec2 barLL(x - barExtend, nutMid - barThick);
                glm::dvec2 barLR(x - nutWidth/2, nutMid + barThick);
                drawList->AddRect(transformCoordinates(barLL), transformCoordinates(barLR), end.color, 0, 0, end.thickness);
                // Right bar
                glm::dvec2 barRL(x + nutWidth/2, nutMid - barThick);
                glm::dvec2 barRR(x + barExtend, nutMid + barThick);
                drawList->AddRect(transformCoordinates(barRL), transformCoordinates(barRR), end.color, 0, 0, end.thickness);
                
                // Cap (small cylinder on top)
                float capBottom = nutTop;
                float capTop = capBottom - capHeight;
                glm::dvec2 capL(x - capWidth/2, capTop);
                glm::dvec2 capR(x + capWidth/2, capBottom);
                drawList->AddRect(transformCoordinates(capL), transformCoordinates(capR), end.color, 0, 0, end.thickness);
            } else {
                // Bottom end: currently handled by ShockAbsorberBottomEnd
                // Draw stepped shaft (section view, extends downward from bottom of spring)
                float y0 = end.baseCenter.y - end.shaftLength / 2.0f;
                float y1 = y0 + end.step1Length;
                float y2 = y1 + end.step2Length;
                float y3 = y2 + end.step3Length;
                // Step 1 (top, largest)
                glm::dvec2 s1L(x - end.step1Diameter/2, y0);
                glm::dvec2 s1R(x + end.step1Diameter/2, y0);
                glm::dvec2 s1L2(x - end.step1Diameter/2, y1);
                glm::dvec2 s1R2(x + end.step1Diameter/2, y1);
                drawList->AddRect(transformCoordinates(s1L), transformCoordinates(s1R2), end.color, 0, 0, end.thickness);
                // Step 2 (middle, spring seat)
                glm::dvec2 s2L(x - end.step2Diameter/2, y1);
                glm::dvec2 s2R(x + end.step2Diameter/2, y1);
                glm::dvec2 s2L2(x - end.step2Diameter/2, y2);
                glm::dvec2 s2R2(x + end.step2Diameter/2, y2);
                drawList->AddRect(transformCoordinates(s2L), transformCoordinates(s2R2), end.color, 0, 0, end.thickness);
                // Step 3 (bottom, smallest)
                glm::dvec2 s3L(x - end.step3Diameter/2, y2);
                glm::dvec2 s3R(x + end.step3Diameter/2, y2);
                glm::dvec2 s3L2(x - end.step3Diameter/2, y3);
                glm::dvec2 s3R2(x + end.step3Diameter/2, y3);
                drawList->AddRect(transformCoordinates(s3L), transformCoordinates(s3R2), end.color, 0, 0, end.thickness);
                // Central bore (section view: vertical rectangle)
                float boreX1 = x - end.boreDiameter/2;
                float boreX2 = x + end.boreDiameter/2;
                glm::dvec2 boreL(boreX1, y0);
                glm::dvec2 boreR(boreX2, y3);
                drawList->AddRect(transformCoordinates(boreL), transformCoordinates(boreR), IM_COL32(200,200,200,255), 0, 0, end.thickness);
                // Chamfers (draw as lines at ends)
                glm::dvec2 chamferL1(x - end.step1Diameter/2, y0);
                glm::dvec2 chamferL2(x - end.step1Diameter/2 + end.chamfer, y0 + end.chamfer);
                glm::dvec2 chamferR1(x + end.step1Diameter/2, y0);
                glm::dvec2 chamferR2(x + end.step1Diameter/2 - end.chamfer, y0 + end.chamfer);
                drawList->AddLine(transformCoordinates(chamferL1), transformCoordinates(chamferL2), end.color, end.thickness);
                drawList->AddLine(transformCoordinates(chamferR1), transformCoordinates(chamferR2), end.color, end.thickness);
            }
            break;
        }
        case ShapeType::SHOCK_ABSORBER_BOTTOM_END: {
            const auto& bottomEnd = static_cast<const ShockAbsorberBottomEnd&>(shape);
            bottomEnd.draw(drawList, this);
            break;
        }
        case ShapeType::BELLOWS: {
            const auto& bellows = static_cast<const Bellows&>(shape);
            
            // Skip if not valid
            if (!bellows.isValid()) break;
            
            // Get profile points and transform them
            const std::vector<glm::dvec2>& profilePoints = bellows.getCachedProfile();
            std::vector<glm::dvec2> transformedPoints;
            transformedPoints.reserve(profilePoints.size());
            
            float s = sin(bellows.angle);
            float c = cos(bellows.angle);
            
            for (const auto& point : profilePoints) {
                float rotatedX = point.x * c - point.y * s;
                float rotatedY = point.x * s + point.y * c;
                glm::dvec2 translatedPoint(
                    bellows.position.x + rotatedX,
                    bellows.position.y + rotatedY
                );
                transformedPoints.push_back(translatedPoint);
            }
            
            // Use appropriate color and thickness
            ImU32 profileColor = bellows.isSelected ? IM_COL32(255, 255, 0, 255) : color;
            float profileThickness = std::max(bellows.thickness * zoomLevel, 2.0f);
            
            // Draw the bellows profile
            for (size_t i = 0; i < transformedPoints.size() - 1; ++i) {
                glm::dvec2 p1 = transformCoordinates(transformedPoints[i]);
                glm::dvec2 p2 = transformCoordinates(transformedPoints[i + 1]);
                drawList->AddLine(p1, p2, profileColor, profileThickness);
            }
            break;
        }
        case ShapeType::BALL_BEARING: {
            const auto& ballBearing = static_cast<const BallBearing&>(shape);
            
            // Skip if not valid
            if (!ballBearing.isValid()) break;
            
            // Transform center position
            glm::dvec2 transformedCenter = transformCoordinates(ballBearing.position);
            
            float outerRadius = (ballBearing.outerDiameter / 2.0f) * zoomLevel;
            float innerRadius = (ballBearing.innerDiameter / 2.0f) * zoomLevel;
            
            // Determine colors based on selection
            ImU32 outerColor = ballBearing.isSelected ? IM_COL32(255, 255, 0, 255) : color;
            ImU32 innerColor = ballBearing.isSelected ? IM_COL32(255, 255, 0, 255) : IM_COL32(100, 100, 100, 255);
            
            // Draw outer race
            drawList->AddCircle(transformedCenter, outerRadius, outerColor, 64, std::max(ballBearing.thickness * zoomLevel, 2.0f));
            
            // Draw inner race
            drawList->AddCircle(transformedCenter, innerRadius, innerColor, 64, std::max(ballBearing.thickness * zoomLevel, 1.0f));
            
            // Draw balls if enabled
            if (ballBearing.showBalls) {
                float pitchRadius = (outerRadius + innerRadius) / 2.0f;
                float ballRadius = (ballBearing.ballDiameter / 2.0f) * zoomLevel;
                
                ImU32 ballColor = ballBearing.isSelected ? IM_COL32(255, 255, 150, 255) : IM_COL32(150, 150, 150, 255);
                
                for (int i = 0; i < ballBearing.numBalls; ++i) {
                    float ballAngle = (float)i / ballBearing.numBalls * 2.0f * M_PI;
                    glm::dvec2 ballPos = glm::dvec2(
                        transformedCenter.x + pitchRadius * cos(ballAngle),
                        transformedCenter.y + pitchRadius * sin(ballAngle)
                    );
                    drawList->AddCircleFilled(ballPos, ballRadius, ballColor);
                }
            }
            
            // Draw cage if enabled
            if (ballBearing.showCage) {
                float cageRadius = (outerRadius + innerRadius) / 2.0f;
                ImU32 cageColor = ballBearing.isSelected ? IM_COL32(255, 255, 100, 180) : IM_COL32(200, 200, 200, 180);
                drawList->AddCircle(transformedCenter, cageRadius, cageColor, 32, 1.0f);
            }
            break;
        }
        default:
            break;
    }
}

glm::dvec2 Canvas::catmullRomPoint(const glm::dvec2& p0, const glm::dvec2& p1, 
                              const glm::dvec2& p2, const glm::dvec2& p3, float t) const {
    const float alpha = 0.5f;  // 0.5 for centripetal Catmull-Rom
    float t2 = t * t;
    float t3 = t2 * t;

    glm::dvec2 result;
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
std::optional<Canvas::SnapPoint> Canvas::findSnapPoint(const glm::dvec2& mousePos) const {
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
                glm::dvec2 midpoint = {
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
                std::vector<std::pair<glm::dvec2, std::string>> cardinalPoints = {
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
                    glm::dvec2 midpoint = {
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
                glm::dvec2 centroid = {
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
                glm::dvec2 topLeft = square.getTopLeft();
                float size = square.getSize();
                
                // Create the four corners
                std::vector<glm::dvec2> corners = {
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
                    glm::dvec2 midpoint = {
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
                glm::dvec2 center = {
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
                glm::dvec2 topLeft = rect.getTopLeft();
                glm::dvec2 size = rect.getSize();
                
                // Create the four corners
                std::vector<glm::dvec2> corners = {
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
                    glm::dvec2 midpoint = {
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
                glm::dvec2 center = {
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
    glm::dvec2 gridSnapped = getSnappedPoint(mousePos);
    float gridDist = calculateDistance(mousePos, gridSnapped);
    if (gridDist < minDist) {
        result = SnapPoint{gridSnapped, "Grid"};
    }
    
    return result;
}

std::optional<glm::dvec2> Canvas::findMidPoint(const glm::dvec2& mousePos) const {
    float minDist = Constants::SNAP_THRESHOLD / zoomLevel;
    std::optional<glm::dvec2> result;
    
    for (const auto& shape : shapes) {
        if (shape->type == ShapeType::LINE) {
            const auto& line = static_cast<const Line&>(*shape);
            glm::dvec2 mid = calculateMidpoint(line.start, line.end);
            float dist = calculateDistance(mousePos, mid);
            if (dist < minDist) {
                minDist = dist;
                result = mid;
            }
        }
    }
    
    return result;
}

std::optional<glm::dvec2> Canvas::findIntersection(const glm::dvec2& mousePos) const {
    float minDist = Constants::SNAP_THRESHOLD / zoomLevel;
    std::optional<glm::dvec2> result;
    
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
                glm::dvec2 intersection = {
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

std::optional<glm::dvec2> Canvas::findPerpendicular(const glm::dvec2& mousePos) const {
    float minDist = Constants::SNAP_THRESHOLD / zoomLevel;
    std::optional<glm::dvec2> result;
    
    for (const auto& shape : shapes) {
        if (shape->type == ShapeType::LINE) {
            const auto& line = static_cast<const Line&>(*shape);
            
            // Calculate perpendicular point
            glm::dvec2 v = {line.end.x - line.start.x, line.end.y - line.start.y};
            float len2 = v.x * v.x + v.y * v.y;
            if (len2 < 0.0001f) continue;
            
            float t = ((mousePos.x - line.start.x) * v.x + (mousePos.y - line.start.y) * v.y) / len2;
            if (t >= 0 && t <= 1) {
                glm::dvec2 perpPoint = {
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

std::optional<glm::dvec2> Canvas::findCenter(const glm::dvec2& mousePos) const {
    float minDist = Constants::SNAP_THRESHOLD / zoomLevel;
    std::optional<glm::dvec2> result;
    
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


glm::dvec2 Canvas::getSnappedPoint(const glm::dvec2& point) const {
    if (!snapToGrid) return point;
    
    return glm::dvec2{
        std::round(point.x / gridSpacing) * gridSpacing,
        std::round(point.y / gridSpacing) * gridSpacing
    };
}

bool Canvas::trySnapToExistingPoint(glm::dvec2& point) const {
    if (auto nearestPoint = findNearestPoint(point, Constants::SNAP_THRESHOLD / zoomLevel)) {
        point = *nearestPoint;
        return true;
    }
    return false;
}

std::optional<glm::dvec2> Canvas::findNearestPoint(const glm::dvec2& point, float threshold) const {
    float minDist = threshold;
    std::optional<glm::dvec2> result;
    
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








// Curve calculation methods
glm::dvec2 Canvas::calculateBezierPoint(const std::vector<glm::dvec2>& points, float t) const {
    if (points.size() != 4) return glm::dvec2(0, 0);
    
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;
    
    glm::dvec2 p = {
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

std::vector<glm::dvec2> Canvas::calculateSplinePoints(const std::vector<glm::dvec2>& controlPoints, bool isClosed) const {
    if (controlPoints.size() < 2) return {};
    
    std::vector<glm::dvec2> result;
    float step = 0.02f;  // Smaller step size for smoother curves
    
    if (controlPoints.size() == 2) {
        // For 2 points, just do linear interpolation
        const glm::dvec2& p0 = controlPoints[0];
        const glm::dvec2& p1 = controlPoints[1];
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
    std::vector<glm::dvec2> points = controlPoints;
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
        const glm::dvec2& p0 = points[i];
        const glm::dvec2& p1 = points[i + 1];
        const glm::dvec2& p2 = points[i + 2];
        const glm::dvec2& p3 = points[i + 3];

        for (float t = 0; t < 1.0f; t += step) {
            result.push_back(catmullRomPoint(p0, p1, p2, p3, t));
        }
    }

    return result;
}

glm::dvec2 Canvas::findNearestSnapPoint(const glm::dvec2& pos) const {
    if (!snapToGrid) return pos;
    
    // Default to returning grid-snapped position
    glm::dvec2 result = getSnappedPoint(pos);
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


void Canvas::clearSelection() {
    // Clear selection state for special shape types
    if (selectedShape) {
        if (selectedShape->type == ShapeType::SPLINE) {
            auto* spline = static_cast<Spline*>(selectedShape);
            spline->selectedPoint = -1;
            spline->isSelected = false;
        } else if (selectedShape->type == ShapeType::BEZIER) {
            auto* bezier = static_cast<BezierCurve*>(selectedShape);
            bezier->selectedPoint = -1;
            bezier->isSelected = false;
        } else if (selectedShape->type == ShapeType::BELLOWS) {
            auto* bellows = static_cast<Bellows*>(selectedShape);
            bellows->isSelected = false;
        } else if (selectedShape->type == ShapeType::BALL_BEARING) {
            auto* ballBearing = static_cast<BallBearing*>(selectedShape);
            ballBearing->isSelected = false;
        }
        
        // Clear the main selection pointer
        selectedShape = nullptr;
    }
}

void Canvas::fitBellowsToView() {
    // Find the selected bellows
    for (const auto& shape : shapes) {
        if (shape->type == ShapeType::BELLOWS && shape.get() == selectedShape) {
            const Bellows* bellows = static_cast<const Bellows*>(shape.get());
            
            // Calculate the bounding box
            ImVec4 bbox = bellows->calculateBoundingBox();
            
            // Transform to account for position and rotation
            float s = sin(bellows->angle);
            float c = cos(bellows->angle);
            
            // Calculate transformed corners
            std::vector<glm::dvec2> corners = {
                glm::dvec2(bbox.x, bbox.y),
                glm::dvec2(bbox.x, bbox.w),
                glm::dvec2(bbox.z, bbox.y),
                glm::dvec2(bbox.z, bbox.w)
            };
            
            // Transform corners
            float minX = FLT_MAX, minY = FLT_MAX;
            float maxX = -FLT_MAX, maxY = -FLT_MAX;
            
            for (auto& corner : corners) {
                // Rotate
                float rotatedX = corner.x * c - corner.y * s;
                float rotatedY = corner.x * s + corner.y * c;
                
                // Translate
                rotatedX += bellows->position.x;
                rotatedY += bellows->position.y;
                
                // Update bounds
                minX = std::min(minX, rotatedX);
                minY = std::min(minY, rotatedY);
                maxX = std::max(maxX, rotatedX);
                maxY = std::max(maxY, rotatedY);
            }
            
            // Add padding
            float padding = 50.0f;
            minX -= padding;
            minY -= padding;
            maxX += padding;
            maxY += padding;
            
            // Calculate required zoom level
            float viewWidth = ImGui::GetWindowWidth() - 20.0f;
            float viewHeight = ImGui::GetWindowHeight() - 20.0f;
            
            float xScale = viewWidth / (maxX - minX);
            float yScale = viewHeight / (maxY - minY);
            
            // Use the smaller of the two scales to ensure everything fits
            float newZoom = std::min(xScale, yScale) * 0.9f;
            
            // Set the zoom and pan position
            zoomLevel = newZoom;
            panOffset = glm::dvec2(
                -minX * zoomLevel + viewWidth / 2.0f - (maxX - minX) * zoomLevel / 2.0f,
                -minY * zoomLevel + viewHeight / 2.0f - (maxY - minY) * zoomLevel / 2.0f
            );
            
            return;
        }
    }
}

// Implementation of findOrCreateBellows
const Drawing::Bellows* Drawing::Canvas::findOrCreateBellows() const {
    // First, try to find an existing bellows in the shapes collection
    for (const auto& shape : shapes) {
        if (shape->type == Drawing::ShapeType::BELLOWS) {
            return static_cast<const Drawing::Bellows*>(shape.get());
        }
    }
    
    // If no bellows found and we have a selected shape that is a bellows, return it
    if (selectedShape && selectedShape->type == Drawing::ShapeType::BELLOWS) {
        return static_cast<const Drawing::Bellows*>(selectedShape);
    }
    
    // If no bellows found at all, return nullptr
    return nullptr;
}

// Implementation of findOrCreateBallBearing
const Drawing::BallBearing* Drawing::Canvas::findOrCreateBallBearing() const {
    // First, try to find an existing ball bearing in the shapes collection
    for (const auto& shape : shapes) {
        if (shape->type == Drawing::ShapeType::BALL_BEARING) {
            return static_cast<const Drawing::BallBearing*>(shape.get());
        }
    }
    
    // If no ball bearing found and we have a selected shape that is a ball bearing, return it
    if (selectedShape && selectedShape->type == Drawing::ShapeType::BALL_BEARING) {
        return static_cast<const Drawing::BallBearing*>(selectedShape);
    }
    
    // If no ball bearing found at all, return nullptr
    return nullptr;
}

void Canvas::handleBallBearingDrawing(ImDrawList* drawList, const glm::dvec2& currentPos) {
    // Only handle click events, not mouse movement
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (isFirstClick) {
            // First click sets the center position of the ball bearing
            startPoint = getSnappedPoint(currentPos);
            isFirstClick = false;
            isDrawing = true;
        } else {
            // Second click determines the radius/size
            endPoint = getSnappedPoint(currentPos);
            
            // Calculate radius from center to mouse position
            float dx = endPoint.x - startPoint.x;
            float dy = endPoint.y - startPoint.y;
            float radius = std::sqrt(dx * dx + dy * dy);
            
            if (radius > 5.0f) {
                // Create ball bearing with radius-based sizing
                auto ballBearing = std::make_unique<BallBearing>();
                
                // Set position
                ballBearing->position = startPoint;
                
                // Set size based on radius (outer diameter = 2 * radius)
                ballBearing->outerDiameter = radius * 2.0f;
                ballBearing->innerDiameter = radius * 1.2f; // 60% of outer diameter
                ballBearing->width = radius * 0.4f; // 20% of outer diameter
                ballBearing->ballDiameter = (ballBearing->outerDiameter - ballBearing->innerDiameter) / 4.0f;
                
                // Add to shapes collection
                shapes.push_back(std::move(ballBearing));
                
                // Save to history
                saveToHistory();
            }
            
            // Reset drawing state
            isFirstClick = true;
            isDrawing = false;
        }
    }
}



void Canvas::setSpring2DShape(std::unique_ptr<Shape> spring) {
    // Remove any existing Spring2D shape
    shapes.erase(std::remove_if(shapes.begin(), shapes.end(),
        [](const std::unique_ptr<Shape>& s) {
            return s->type == ShapeType::SPRING2D;
        }), shapes.end());
    // Add the new spring
    shapes.push_back(std::move(spring));
    // Optionally select the new spring
    selectShape(shapes.back().get());
    saveToHistory();
}

void Canvas::handleSpring2DDrawing(ImDrawList* drawList, const glm::dvec2& mousePos) {
    glm::dvec2 snappedPos = findNearestSnapPoint(mousePos);

    if (!isDrawing) {
        // Start drawing when tool is selected
        isDrawing = true;
    }

    if (isDrawing) {
        // Show preview
        renderer->previewSpring2D(drawList, snappedPos);

        // Place spring on click
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered()) {
            std::cout << "Placing Spring2D at: " << snappedPos.x << ", " << snappedPos.y << std::endl;
            shapes.push_back(std::make_unique<Spring2D>(
                snappedPos.x, snappedPos.y,
                springOuterDiameter, springWireDiameter, springFreeLength, springNumCoils,
                IM_COL32(80, 80, 80, 255)
            ));
            saveToHistory();
            isDrawing = false; // Stop preview after placing
        }
    }
}


void Canvas::updateShockAbsorberEndsForSpring(const Drawing::Spring2D* spring) {
    for (auto& shape : shapes) {
        if (shape->type == Drawing::ShapeType::SHOCK_ABSORBER_END_2D) {
            auto* end = static_cast<Drawing::ShockAbsorberEnd2D*>(shape.get());
            if (end->parentSpring == spring) {
                end->updateGeometry();
            }
        }
    }
}

std::vector<Canvas::ShockAbsorberAssembly> Canvas::findShockAbsorberAssemblies() const {
    std::vector<ShockAbsorberAssembly> assemblies;
    
    // Find all springs first
    std::vector<const Spring2D*> springs;
    for (const auto& shape : shapes) {
        if (shape->type == ShapeType::SPRING2D) {
            springs.push_back(static_cast<const Spring2D*>(shape.get()));
        }
    }
    
    // For each spring, try to find matching ends
    for (const auto* spring : springs) {
        ShockAbsorberAssembly assembly;
        assembly.spring = spring;
        assembly.topEnd = nullptr;
        assembly.bottomEnd = nullptr;
        
        // Find shock absorber ends that are positioned correctly relative to this spring
        float springCenterY = spring->centerY;
        float springCenterX = spring->centerX;
        float tolerance = 200.0f; // Large tolerance for alignment
        
        for (const auto& shape : shapes) {
            if (shape->type == ShapeType::SHOCK_ABSORBER_END_2D) {
                const auto* end = static_cast<const ShockAbsorberEnd2D*>(shape.get());
                // Check if this end belongs to this spring (by parent reference or proximity)
                if (end->parentSpring == spring || 
                    (std::abs(end->baseCenter.x - springCenterX) < tolerance &&
                     std::abs(end->baseCenter.y - springCenterY) < tolerance)) {
                    assembly.topEnd = end;
                }
            }
            else if (shape->type == ShapeType::SHOCK_ABSORBER_BOTTOM_END) {
                const auto* bottomEnd = static_cast<const ShockAbsorberBottomEnd*>(shape.get());
                // Check if this bottom end is near the spring
                if (std::abs(bottomEnd->baseCenter.x - springCenterX) < tolerance &&
                    std::abs(bottomEnd->baseCenter.y - springCenterY) < tolerance) {
                    assembly.bottomEnd = bottomEnd;
                }
            }
        }
        
        // Only add assemblies that have all components
        if (assembly.isComplete()) {
            assemblies.push_back(assembly);
        }
    }
    
    return assemblies;
}

bool Canvas::hasCompleteShockAbsorberAssembly() const {
    auto assemblies = findShockAbsorberAssemblies();
    return !assemblies.empty();
}

} // namespace Drawing 