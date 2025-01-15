#include "Canvas.hpp"
#include <cmath>
#include <algorithm>
#include <SDL2/SDL.h>

// ImVec2 operator overloads
inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) {
    return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y);
}

inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) {
    return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y);
}

// Scalar multiplication operators
inline ImVec2 operator*(const ImVec2& v, float s) {
    return ImVec2(v.x * s, v.y * s);
}

inline ImVec2 operator*(float s, const ImVec2& v) {
    return ImVec2(v.x * s, v.y * s);
}

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

Canvas::Canvas()
    : currentMode(DrawingMode::None)
    , isDrawing(false)
    , isFirstClick(true)
    , clickCount(0)
    , snapToGrid(true)
    , zoomLevel(1.0f)
    , panOffset(0.0f, 0.0f)
    , gridSpacing(Constants::DEFAULT_GRID_SPACING)
    , isEditingCurve(false)
    , curveStartT(0.0f)
    , curveEndT(1.0f)
{}

ImVec2 Canvas::getSnappedPoint(const ImVec2& point) const {
    return point; // No grid snapping
}

bool Canvas::trySnapToExistingPoint(ImVec2& point) const {
    auto snapResult = findSnapPoint(point);
    if (snapResult.has_value()) {
        point = snapResult.value().point;
        return true;
    }
    return false;
}

std::optional<ImVec2> Canvas::findNearestPoint(const ImVec2& point, float threshold) const {
    std::optional<ImVec2> nearest;
    float minDist = threshold;

    // Check points
    for (const auto& p : points) {
        float dist = calculateDistance(point, p.position);
        if (dist < minDist) {
            minDist = dist;
            nearest = p.position;
        }
    }

    // Check line endpoints
    for (const auto& line : lines) {
        float distStart = calculateDistance(point, line.start);
        float distEnd = calculateDistance(point, line.end);
        if (distStart < minDist) {
            minDist = distStart;
            nearest = line.start;
        }
        if (distEnd < minDist) {
            minDist = distEnd;
            nearest = line.end;
        }
    }

    // Check circle centers
    for (const auto& circle : circles) {
        float dist = calculateDistance(point, circle.center);
        if (dist < minDist) {
            minDist = dist;
            nearest = circle.center;
        }
    }

    // Check triangle vertices
    for (const auto& triangle : triangles) {
        for (const auto& vertex : triangle.points) {
            float dist = calculateDistance(point, vertex);
            if (dist < minDist) {
                minDist = dist;
                nearest = vertex;
            }
        }
    }

    // Check square/rectangle corners
    for (const auto& square : squares) {
        ImVec2 corners[] = {
            square.start,
            ImVec2(square.end.x, square.start.y),
            square.end,
            ImVec2(square.start.x, square.end.y)
        };
        for (const auto& corner : corners) {
            float dist = calculateDistance(point, corner);
            if (dist < minDist) {
                minDist = dist;
                nearest = corner;
            }
        }
    }

    for (const auto& rect : rectangles) {
        ImVec2 corners[] = {
            rect.start,
            ImVec2(rect.end.x, rect.start.y),
            rect.end,
            ImVec2(rect.start.x, rect.end.y)
        };
        for (const auto& corner : corners) {
            float dist = calculateDistance(point, corner);
            if (dist < minDist) {
                minDist = dist;
                nearest = corner;
            }
        }
    }

    return nearest;
}

std::optional<Canvas::SnapPoint> Canvas::findSnapPoint(const ImVec2& mousePos) const {
    const float threshold = Constants::SNAP_THRESHOLD / zoomLevel;
    
    // Check endpoints first
    for (const auto& line : lines) {
        if (calculateDistance(mousePos, line.start) < threshold)
            return SnapPoint{line.start, "endpoint"};
        if (calculateDistance(mousePos, line.end) < threshold)
            return SnapPoint{line.end, "endpoint"};
    }
    
    // Check midpoints
    auto midPoint = findMidPoint(mousePos);
    if (midPoint.has_value())
        return SnapPoint{midPoint.value(), "midpoint"};
    
    // Check circle centers
    auto center = findCenter(mousePos);
    if (center.has_value())
        return SnapPoint{center.value(), "center"};
    
    // Check intersections
    auto intersection = findIntersection(mousePos);
    if (intersection.has_value())
        return SnapPoint{intersection.value(), "intersection"};
    
    // Check perpendicular points
    auto perpendicular = findPerpendicular(mousePos);
    if (perpendicular.has_value())
        return SnapPoint{perpendicular.value(), "perpendicular"};
    
    return std::nullopt;
}

std::optional<ImVec2> Canvas::findMidPoint(const ImVec2& mousePos) const {
    const float threshold = Constants::SNAP_THRESHOLD / zoomLevel;
    
    for (const auto& line : lines) {
        ImVec2 midPoint = ImVec2(
            (line.start.x + line.end.x) * 0.5f,
            (line.start.y + line.end.y) * 0.5f
        );
        if (calculateDistance(mousePos, midPoint) < threshold)
            return midPoint;
    }
    return std::nullopt;
}

std::optional<ImVec2> Canvas::findCenter(const ImVec2& mousePos) const {
    const float threshold = Constants::SNAP_THRESHOLD / zoomLevel;
    
    for (const auto& circle : circles) {
        if (calculateDistance(mousePos, circle.center) < threshold)
            return circle.center;
    }
    return std::nullopt;
}

std::optional<ImVec2> Canvas::findIntersection(const ImVec2& mousePos) const {
    const float threshold = Constants::SNAP_THRESHOLD / zoomLevel;
    
    for (size_t i = 0; i < lines.size(); ++i) {
        for (size_t j = i + 1; j < lines.size(); ++j) {
            // Line intersection calculation
            float x1 = lines[i].start.x, y1 = lines[i].start.y;
            float x2 = lines[i].end.x, y2 = lines[i].end.y;
            float x3 = lines[j].start.x, y3 = lines[j].start.y;
            float x4 = lines[j].end.x, y4 = lines[j].end.y;
            
            float denominator = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
            if (std::abs(denominator) < 0.0001f) continue; // Lines are parallel
            
            float t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denominator;
            float u = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) / denominator;
            
            if (t >= 0 && t <= 1 && u >= 0 && u <= 1) {
                ImVec2 intersection(
                    x1 + t * (x2 - x1),
                    y1 + t * (y2 - y1)
                );
                if (calculateDistance(mousePos, intersection) < threshold)
                    return intersection;
            }
        }
    }
    return std::nullopt;
}

std::optional<ImVec2> Canvas::findPerpendicular(const ImVec2& mousePos) const {
    const float threshold = Constants::SNAP_THRESHOLD / zoomLevel;
    
    for (const auto& line : lines) {
        // Calculate perpendicular point
        float dx = line.end.x - line.start.x;
        float dy = line.end.y - line.start.y;
        float len2 = dx * dx + dy * dy;
        if (len2 < 0.0001f) continue;
        
        float t = ((mousePos.x - line.start.x) * dx + (mousePos.y - line.start.y) * dy) / len2;
        if (t >= 0 && t <= 1) {
            ImVec2 perpPoint(
                line.start.x + t * dx,
                line.start.y + t * dy
            );
            if (calculateDistance(mousePos, perpPoint) < threshold)
                return perpPoint;
        }
    }
    return std::nullopt;
}

void Canvas::handleInput() {
    ImGuiIO& io = ImGui::GetIO();
    
    // Only check for hover, not focus, since we want to draw immediately after selecting a tool
    if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows)) {
        return;
    }
    
    ImVec2 mousePos = io.MousePos;
    
    // Get window position and content region
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
    ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
    
    // Convert mouse position to canvas space
    mousePos.x -= (windowPos.x + contentMin.x);
    mousePos.y -= (windowPos.y + contentMin.y);
    
    // Check if mouse is within canvas bounds
    if (mousePos.x < 0 || mousePos.y < 0 || 
        mousePos.x > (contentMax.x - contentMin.x) || 
        mousePos.y > (contentMax.y - contentMin.y)) {
        return;
    }
    
    // Transform to world space
    ImVec2 worldPos = transformCoordinates(mousePos);
    
    // Check for snapping points
    auto snapResult = findSnapPoint(worldPos);
    if (snapResult.has_value()) {
        worldPos = snapResult.value().point;
        renderSnapIndicator(ImGui::GetWindowDrawList(), worldPos, snapResult.value().type);
    }
    
    // Always show preview for current tool
    if (currentMode != DrawingMode::None) {
        renderPreview(ImGui::GetWindowDrawList(), worldPos);
    }
    
    // Handle mouse input based on current mode
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        switch (currentMode) {
            case DrawingMode::Point:
                handlePointDrawing(drawList, worldPos);
                break;
            case DrawingMode::Line:
                handleLineDrawing(drawList, worldPos);
                break;
            case DrawingMode::Circle:
                handleCircleDrawing(drawList, worldPos);
                break;
            case DrawingMode::Triangle:
                handleTriangleDrawing(drawList, worldPos);
                break;
            case DrawingMode::Square:
                handleSquareDrawing(drawList, worldPos);
                break;
            case DrawingMode::Rectangle:
                handleRectangleDrawing(drawList, worldPos);
                break;
            case DrawingMode::Spline:
                handleSplineDrawing(drawList, worldPos);
                break;
            case DrawingMode::BezierCurve:
                handleBezierDrawing(drawList, worldPos);
                break;
            default:
                break;
        }
    }
    
    // Handle right-click for splines
    if (currentMode == DrawingMode::Spline && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        handleSplineDrawing(ImGui::GetWindowDrawList(), worldPos);
    }
    
    // Handle panning with middle mouse button
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
        ImVec2 delta = io.MouseDelta;
        updatePan(delta);
    }
    
    // Handle zooming with mouse wheel
    if (io.MouseWheel != 0.0f) {
        updateZoom(io.MouseWheel);
    }
}

void Canvas::renderPreview(ImDrawList* drawList, const ImVec2& currentPos) {
    switch (currentMode) {
        case DrawingMode::Point:
            previewPoint(drawList, currentPos);
            break;
        case DrawingMode::Line:
            if (isDrawing) {
                previewLine(drawList, startPoint, currentPos);
            }
            break;
        case DrawingMode::Circle:
            if (isDrawing) {
                float radius = calculateDistance(startPoint, currentPos);
                previewCircle(drawList, startPoint, radius);
            }
            break;
        case DrawingMode::Triangle:
            previewTriangle(drawList, trianglePoints, clickCount);
            break;
        case DrawingMode::Square:
            if (isDrawing) {
                previewSquare(drawList, startPoint, currentPos);
            }
            break;
        case DrawingMode::Rectangle:
            if (isDrawing) {
                previewRectangle(drawList, startPoint, currentPos);
            }
            break;
        case DrawingMode::Spline:
            if (isEditingCurve) {
                auto previewPoints = currentCurvePoints;
                previewPoints.push_back(currentPos);
                previewSpline(drawList, previewPoints);
            }
            break;
        case DrawingMode::BezierCurve:
            if (currentCurvePoints.size() < 4) {
                auto previewPoints = currentCurvePoints;
                previewPoints.push_back(currentPos);
                previewBezier(drawList, previewPoints);
            }
            break;
        default:
            break;
    }
}

void Canvas::previewPoint(ImDrawList* drawList, const ImVec2& pos) const {
    ImVec2 screenPos = inverseTransformCoordinates(pos);
    drawList->AddCircleFilled(
        screenPos,
        Constants::DEFAULT_POINT_SIZE * zoomLevel,
        Colors::PREVIEW
    );
}

void Canvas::previewLine(ImDrawList* drawList, const ImVec2& start, const ImVec2& end) const {
    ImVec2 screenStart = inverseTransformCoordinates(start);
    ImVec2 screenEnd = inverseTransformCoordinates(end);
    drawList->AddLine(
        screenStart,
        screenEnd,
        Colors::PREVIEW,
        Constants::DEFAULT_LINE_THICKNESS * zoomLevel
    );
}

void Canvas::previewCircle(ImDrawList* drawList, const ImVec2& center, float radius) const {
    ImVec2 screenCenter = inverseTransformCoordinates(center);
    float screenRadius = radius * zoomLevel;
    drawList->AddCircle(
        screenCenter,
        screenRadius,
        Colors::PREVIEW,
        Constants::CIRCLE_SEGMENTS,
        Constants::DEFAULT_LINE_THICKNESS * zoomLevel
    );
}

void Canvas::previewTriangle(ImDrawList* drawList, const std::array<ImVec2, 3>& points, int count) const {
    // Draw existing points
    for (int i = 0; i < count; i++) {
        ImVec2 screenPoint = inverseTransformCoordinates(points[i]);
        drawList->AddCircleFilled(
            screenPoint,
            Constants::DEFAULT_POINT_SIZE * zoomLevel,
            Colors::PREVIEW
        );
    }

    // Draw lines between points
    if (count >= 2) {
        for (int i = 0; i < count - 1; i++) {
            ImVec2 screenPoint1 = inverseTransformCoordinates(points[i]);
            ImVec2 screenPoint2 = inverseTransformCoordinates(points[i + 1]);
            drawList->AddLine(
                screenPoint1,
                screenPoint2,
                Colors::PREVIEW,
                Constants::DEFAULT_LINE_THICKNESS * zoomLevel
            );
        }

        // If all three points are placed, draw the closing line
        if (count == 3) {
            ImVec2 screenPoint1 = inverseTransformCoordinates(points[2]);
            ImVec2 screenPoint2 = inverseTransformCoordinates(points[0]);
            drawList->AddLine(
                screenPoint1,
                screenPoint2,
                Colors::PREVIEW,
                Constants::DEFAULT_LINE_THICKNESS * zoomLevel
            );
        }
    }
}

void Canvas::previewSquare(ImDrawList* drawList, const ImVec2& start, const ImVec2& end) const {
    float size = std::max(
        std::abs(end.x - start.x),
        std::abs(end.y - start.y)
    );
    float signX = (end.x - start.x) >= 0 ? 1 : -1;
    float signY = (end.y - start.y) >= 0 ? 1 : -1;

    ImVec2 squareEnd(
        start.x + (size * signX),
        start.y + (size * signY)
    );

    ImVec2 screenStart = inverseTransformCoordinates(start);
    ImVec2 screenEnd = inverseTransformCoordinates(squareEnd);
    ImVec2 screenTopRight(screenEnd.x, screenStart.y);
    ImVec2 screenBottomLeft(screenStart.x, screenEnd.y);

    drawList->AddQuad(
        screenStart,
        screenTopRight,
        screenEnd,
        screenBottomLeft,
        Colors::PREVIEW,
        Constants::DEFAULT_LINE_THICKNESS * zoomLevel
    );
}

void Canvas::previewRectangle(ImDrawList* drawList, const ImVec2& start, const ImVec2& end) const {
    ImVec2 screenStart = inverseTransformCoordinates(start);
    ImVec2 screenEnd = inverseTransformCoordinates(end);
    
    ImVec2 screenTopRight(screenEnd.x, screenStart.y);
    ImVec2 screenBottomLeft(screenStart.x, screenEnd.y);

    drawList->AddQuad(
        screenStart,
        screenTopRight,
        screenEnd,
        screenBottomLeft,
        Colors::PREVIEW,
        Constants::DEFAULT_LINE_THICKNESS * zoomLevel
    );
}

void Canvas::render(ImDrawList* drawList) {
    // Update window info
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
    ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
    
    setWindowInfo(
        windowPos.x + contentMin.x,
        windowPos.y + contentMin.y,
        contentMax.x - contentMin.x,
        contentMax.y - contentMin.y
    );
    
    renderGrid(drawList);
    
    // Render all completed shapes
    renderPoints(drawList);
    renderLines(drawList);
    renderCircles(drawList);
    renderTriangles(drawList);
    renderSquares(drawList);
    renderRectangles(drawList);
    renderSplines(drawList);
    renderBezierCurves(drawList);
}

ImVec2 Canvas::transformCoordinates(const ImVec2& screenPos) const {
    // Transform from screen space to world space
    ImVec2 worldPos;
    worldPos.x = (screenPos.x / zoomLevel) - panOffset.x;
    worldPos.y = (screenPos.y / zoomLevel) - panOffset.y;
    return worldPos;
}

ImVec2 Canvas::inverseTransformCoordinates(const ImVec2& worldPos) const {
    // Transform from world space to screen space
    ImVec2 screenPos;
    screenPos.x = (worldPos.x + panOffset.x) * zoomLevel;
    screenPos.y = (worldPos.y + panOffset.y) * zoomLevel;
    
    // Add window position and content region offset
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
    screenPos.x += (windowPos.x + contentMin.x);
    screenPos.y += (windowPos.y + contentMin.y);
    
    return screenPos;
}

void Canvas::setDrawingMode(DrawingMode mode) {
    // Clear all temporary states
    isDrawing = false;
    isEditingCurve = false;
    isFirstClick = true;
    clickCount = 0;
    currentCurvePoints.clear();
    startPoint = ImVec2(0, 0);
    endPoint = ImVec2(0, 0);
    
    // Set new mode
    currentMode = mode;
    
    // Force focus to the window containing the canvas
    ImGui::SetWindowFocus();
}

void Canvas::reset() {
    isDrawing = false;
    isEditingCurve = false;
    isFirstClick = true;
    clickCount = 0;
    currentCurvePoints.clear();
    startPoint = ImVec2(0, 0);
    endPoint = ImVec2(0, 0);
}

void Canvas::renderGrid(ImDrawList* drawList) {
    // Get window position and content region
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
    ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
    
    // Calculate actual window area in screen space
    ImVec2 canvasStart(windowPos.x + contentMin.x, windowPos.y + contentMin.y);
    ImVec2 canvasEnd(windowPos.x + contentMax.x, windowPos.y + contentMax.y);
    
    // Convert canvas bounds to world space
    ImVec2 worldTopLeft = transformCoordinates(ImVec2(0, 0));
    ImVec2 worldBottomRight = transformCoordinates(ImVec2(canvasEnd.x - canvasStart.x, canvasEnd.y - canvasStart.y));
    
    // Base grid spacing in world coordinates
    float baseSpacing = gridSpacing;
    
    // Adjust grid spacing based on zoom level
    float adjustedSpacing = baseSpacing;
    float spacingInPixels = adjustedSpacing * zoomLevel;
    
    // Adjust grid spacing to maintain reasonable pixel size
    while (spacingInPixels < 40.0f) {
        adjustedSpacing *= 2.0f;
        spacingInPixels = adjustedSpacing * zoomLevel;
    }
    while (spacingInPixels > 80.0f) {
        adjustedSpacing *= 0.5f;
        spacingInPixels = adjustedSpacing * zoomLevel;
    }
    
    // Add some padding to ensure we cover the entire visible area
    float padding = adjustedSpacing * 2.0f;
    float startX = std::floor((worldTopLeft.x - padding) / adjustedSpacing) * adjustedSpacing;
    float startY = std::floor((worldTopLeft.y - padding) / adjustedSpacing) * adjustedSpacing;
    float endX = std::ceil((worldBottomRight.x + padding) / adjustedSpacing) * adjustedSpacing;
    float endY = std::ceil((worldBottomRight.y + padding) / adjustedSpacing) * adjustedSpacing;

    // Draw vertical grid lines
    for (float x = startX; x <= endX; x += adjustedSpacing) {
        ImVec2 p1 = inverseTransformCoordinates(ImVec2(x, startY));
        ImVec2 p2 = inverseTransformCoordinates(ImVec2(x, endY));
        drawList->AddLine(p1, p2, Colors::GRID, 1.0f);
    }

    // Draw horizontal grid lines
    for (float y = startY; y <= endY; y += adjustedSpacing) {
        ImVec2 p1 = inverseTransformCoordinates(ImVec2(startX, y));
        ImVec2 p2 = inverseTransformCoordinates(ImVec2(endX, y));
        drawList->AddLine(p1, p2, Colors::GRID, 1.0f);
    }
}

void Canvas::updateZoom(float delta) {
    // Get window position and content region
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
    
    // Get mouse position relative to canvas
    ImVec2 mousePos = ImGui::GetMousePos();
    mousePos.x -= (windowPos.x + contentMin.x);
    mousePos.y -= (windowPos.y + contentMin.y);
    
    // Get world position under mouse before zoom
    ImVec2 worldPos = transformCoordinates(mousePos);
    
    // Update zoom level with smoother transitions
    float oldZoom = zoomLevel;
    float targetZoom = delta > 0 ? 
        zoomLevel * Constants::ZOOM_SPEED : 
        zoomLevel / Constants::ZOOM_SPEED;
    
    zoomLevel = std::clamp(targetZoom, Constants::MIN_ZOOM, Constants::MAX_ZOOM);
    
    if (zoomLevel != oldZoom) {
        // Calculate the screen position after zoom without window offset
        ImVec2 newScreenPos = ImVec2(
            (worldPos.x + panOffset.x) * zoomLevel,
            (worldPos.y + panOffset.y) * zoomLevel
        );
        
        // Adjust pan offset to keep the world point under mouse
        panOffset.x += (mousePos.x - newScreenPos.x) / zoomLevel;
        panOffset.y += (mousePos.y - newScreenPos.y) / zoomLevel;
    }
}

void Canvas::updatePan(const ImVec2& delta) {
    // Convert screen space delta to world space delta
    // Note: we add delta instead of subtracting to match mouse movement direction
    panOffset.x += delta.x / zoomLevel;
    panOffset.y += delta.y / zoomLevel;
}

void Canvas::handlePointDrawing(ImDrawList* drawList, const ImVec2& currentPos) {
    Point newPoint{
        currentPos,
        Colors::POINT,
        Constants::DEFAULT_POINT_SIZE
    };
    points.push_back(newPoint);
}

void Canvas::handleLineDrawing(ImDrawList* drawList, const ImVec2& currentPos) {
    if (!isDrawing) {
        startPoint = currentPos;
        isDrawing = true;
    } else {
        Line newLine{
            startPoint,
            currentPos,
            Colors::LINE,
            Constants::DEFAULT_LINE_THICKNESS
        };
        if (newLine.isValid()) {
            lines.push_back(newLine);
            reset();
        }
    }
}

void Canvas::handleCircleDrawing(ImDrawList* drawList, const ImVec2& currentPos) {
    if (!isDrawing) {
        startPoint = currentPos;
        isDrawing = true;
    } else {
        Circle newCircle{
            startPoint,
            calculateDistance(startPoint, currentPos),
            Colors::CIRCLE,
            Constants::DEFAULT_LINE_THICKNESS
        };
        if (newCircle.isValid()) {
            circles.push_back(newCircle);
            reset();
        }
    }
}

void Canvas::handleTriangleDrawing(ImDrawList* drawList, const ImVec2& currentPos) {
    trianglePoints[clickCount] = currentPos;
    clickCount++;

    if (clickCount == 3) {
        Triangle newTriangle{
            trianglePoints,
            Colors::TRIANGLE,
            Constants::DEFAULT_LINE_THICKNESS
        };
        if (newTriangle.isValid()) {
            triangles.push_back(newTriangle);
            reset();
        } else {
            clickCount--;
        }
    }
}

void Canvas::handleSquareDrawing(ImDrawList* drawList, const ImVec2& currentPos) {
    if (!isDrawing) {
        startPoint = currentPos;
        isDrawing = true;
    } else {
        float size = std::max(
            std::abs(currentPos.x - startPoint.x),
            std::abs(currentPos.y - startPoint.y)
        );
        float signX = (currentPos.x - startPoint.x) >= 0 ? 1 : -1;
        float signY = (currentPos.y - startPoint.y) >= 0 ? 1 : -1;

        ImVec2 endPoint(
            startPoint.x + (size * signX),
            startPoint.y + (size * signY)
        );

        Square newSquare{
            startPoint,
            endPoint,
            Colors::SQUARE,
            Constants::DEFAULT_LINE_THICKNESS
        };
        if (newSquare.isValid()) {
            squares.push_back(newSquare);
            reset();
        }
    }
}

void Canvas::handleRectangleDrawing(ImDrawList* drawList, const ImVec2& currentPos) {
    if (!isDrawing) {
        startPoint = currentPos;
        isDrawing = true;
    } else {
        Rectangle newRect{
            startPoint,
            currentPos,
            Colors::RECTANGLE,
            Constants::DEFAULT_LINE_THICKNESS
        };
        if (newRect.isValid()) {
            rectangles.push_back(newRect);
            reset();
        }
    }
}

void Canvas::handleSplineDrawing(ImDrawList* drawList, const ImVec2& currentPos) {
    if (!isEditingCurve) {
        isEditingCurve = true;
    }
    
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        currentCurvePoints.push_back(currentPos);
    }
    
    // Right-click to finish the spline
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && currentCurvePoints.size() >= 2) {
        Spline newSpline{
            currentCurvePoints,
            Colors::SPLINE,
            Constants::DEFAULT_LINE_THICKNESS,
            false  // Not closed by default
        };
        if (newSpline.isValid()) {
            splines.push_back(newSpline);
            currentCurvePoints.clear();
            isEditingCurve = false;
        }
    }
}

void Canvas::handleBezierDrawing(ImDrawList* drawList, const ImVec2& currentPos) {
    if (!isEditingCurve) {
        isEditingCurve = true;
    }
    
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && currentCurvePoints.size() < 4) {
        currentCurvePoints.push_back(currentPos);
        
        // Automatically finish bezier curve when we have 4 points
        if (currentCurvePoints.size() == 4) {
            BezierCurve newCurve{
                currentCurvePoints,
                Colors::BEZIER,
                Constants::DEFAULT_LINE_THICKNESS,
                0.0f,  // Start from beginning
                1.0f   // Go to end
            };
            if (newCurve.isValid()) {
                bezierCurves.push_back(newCurve);
                currentCurvePoints.clear();
                isEditingCurve = false;
            }
        }
    }
}

void Canvas::previewSpline(ImDrawList* drawList, const std::vector<ImVec2>& points) const {
    // Draw the spline curve preview
    if (points.size() >= 2) {
        std::vector<ImVec2> curvePoints = calculateSplinePoints(points, false);
        
        // Draw the curve with a thicker, semi-transparent line
        for (size_t i = 1; i < curvePoints.size(); ++i) {
            ImVec2 p1 = inverseTransformCoordinates(curvePoints[i - 1]);
            ImVec2 p2 = inverseTransformCoordinates(curvePoints[i]);
            drawList->AddLine(p1, p2, Colors::PREVIEW, (Constants::DEFAULT_LINE_THICKNESS + 2.0f) * zoomLevel);
        }
    }
    
    // Draw control points with labels
    for (size_t i = 0; i < points.size(); ++i) {
        ImVec2 screenPos = inverseTransformCoordinates(points[i]);
        
        // Draw point highlight
        drawList->AddCircle(screenPos, 6.0f * zoomLevel, Colors::CONTROL_LINE, 0, 2.0f * zoomLevel);
        drawList->AddCircleFilled(screenPos, 4.0f * zoomLevel, Colors::CONTROL_POINT);
        
        // Draw label
        char label[8];
        snprintf(label, sizeof(label), "P%zu", i);
        drawList->AddText(screenPos + ImVec2(8.0f, 8.0f), Colors::CONTROL_POINT, label);
    }
    
    // Draw control polygon with dashed lines
    for (size_t i = 1; i < points.size(); ++i) {
        ImVec2 p1 = inverseTransformCoordinates(points[i - 1]);
        ImVec2 p2 = inverseTransformCoordinates(points[i]);
        drawDashedLine(drawList, p1, p2, Colors::CONTROL_LINE, 1.0f * zoomLevel, 5.0f * zoomLevel);
    }
}

void Canvas::previewBezier(ImDrawList* drawList, const std::vector<ImVec2>& points) const {
    // Draw the Bezier curve preview
    if (points.size() >= 2) {
        std::vector<ImVec2> curvePoints;
        const int STEPS = 50;
        
        if (points.size() == 4) {
            // Full Bezier curve
            for (int i = 0; i <= STEPS; ++i) {
                float t = static_cast<float>(i) / static_cast<float>(STEPS);
                curvePoints.push_back(calculateBezierPoint(points, t));
            }
        } else {
            // Partial preview based on current control points
            auto previewPoints = points;
            while (previewPoints.size() < 4) {
                previewPoints.push_back(previewPoints.back());
            }
            for (int i = 0; i <= STEPS; ++i) {
                float t = static_cast<float>(i) / static_cast<float>(STEPS);
                curvePoints.push_back(calculateBezierPoint(previewPoints, t));
            }
        }
        
        // Draw the curve with a thicker, semi-transparent line
        for (size_t i = 1; i < curvePoints.size(); ++i) {
            ImVec2 p1 = inverseTransformCoordinates(curvePoints[i - 1]);
            ImVec2 p2 = inverseTransformCoordinates(curvePoints[i]);
            drawList->AddLine(p1, p2, Colors::PREVIEW, (Constants::DEFAULT_LINE_THICKNESS + 2.0f) * zoomLevel);
        }
    }
    
    // Draw control points with labels
    for (size_t i = 0; i < points.size(); ++i) {
        ImVec2 screenPos = inverseTransformCoordinates(points[i]);
        
        // Draw point highlight
        drawList->AddCircle(screenPos, 6.0f * zoomLevel, Colors::CONTROL_LINE, 0, 2.0f * zoomLevel);
        drawList->AddCircleFilled(screenPos, 4.0f * zoomLevel, Colors::CONTROL_POINT);
        
        // Draw label with role
        const char* labels[] = {"Start", "Ctrl 1", "Ctrl 2", "End"};
        drawList->AddText(screenPos + ImVec2(8.0f, 8.0f), Colors::CONTROL_POINT, 
                         i < 4 ? labels[i] : "");
    }
    
    // Draw control polygon with dashed lines
    for (size_t i = 1; i < points.size(); ++i) {
        ImVec2 p1 = inverseTransformCoordinates(points[i - 1]);
        ImVec2 p2 = inverseTransformCoordinates(points[i]);
        drawDashedLine(drawList, p1, p2, Colors::CONTROL_LINE, 1.0f * zoomLevel, 5.0f * zoomLevel);
    }
}

void Canvas::drawDashedLine(ImDrawList* drawList, const ImVec2& p1, const ImVec2& p2, 
                          ImU32 color, float thickness, float dash_length) const {
    ImVec2 dir = p2 - p1;
    float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    dir.x /= length;
    dir.y /= length;
    
    float dash_step = dash_length * 2.0f;
    float steps = length / dash_step;
    
    for (float i = 0.0f; i < steps; i += 1.0f) {
        float start = i * dash_step;
        float end = start + dash_length;
        if (end > length) end = length;
        
        ImVec2 dash_start = ImVec2(p1.x + dir.x * start, p1.y + dir.y * start);
        ImVec2 dash_end = ImVec2(p1.x + dir.x * end, p1.y + dir.y * end);
        
        drawList->AddLine(dash_start, dash_end, color, thickness);
    }
}

void Canvas::renderPoints(ImDrawList* drawList) const {
    for (const auto& point : points) {
        ImVec2 screenPos = inverseTransformCoordinates(point.position);
        drawList->AddCircleFilled(
            screenPos,
            point.size * zoomLevel,
            point.color
        );
    }
}

void Canvas::renderLines(ImDrawList* drawList) const {
    for (const auto& line : lines) {
        ImVec2 screenStart = inverseTransformCoordinates(line.start);
        ImVec2 screenEnd = inverseTransformCoordinates(line.end);
        drawList->AddLine(
            screenStart,
            screenEnd,
            line.color,
            line.thickness * zoomLevel
        );
    }
}

void Canvas::renderCircles(ImDrawList* drawList) const {
    for (const auto& circle : circles) {
        ImVec2 screenCenter = inverseTransformCoordinates(circle.center);
        float screenRadius = circle.radius * zoomLevel;
        drawList->AddCircle(
            screenCenter,
            screenRadius,
            circle.color,
            Constants::CIRCLE_SEGMENTS,
            circle.thickness * zoomLevel
        );
    }
}

void Canvas::renderTriangles(ImDrawList* drawList) const {
    for (const auto& triangle : triangles) {
        ImVec2 screenPoints[3];
        for (int i = 0; i < 3; i++) {
            screenPoints[i] = inverseTransformCoordinates(triangle.points[i]);
        }
        drawList->AddTriangle(
            screenPoints[0],
            screenPoints[1],
            screenPoints[2],
            triangle.color,
            triangle.thickness * zoomLevel
        );
    }
}

void Canvas::renderSquares(ImDrawList* drawList) const {
    for (const auto& square : squares) {
        ImVec2 screenStart = inverseTransformCoordinates(square.start);
        ImVec2 screenEnd = inverseTransformCoordinates(square.end);
        ImVec2 screenTopRight(screenEnd.x, screenStart.y);
        ImVec2 screenBottomLeft(screenStart.x, screenEnd.y);
        
        drawList->AddQuad(
            screenStart,
            screenTopRight,
            screenEnd,
            screenBottomLeft,
            square.color,
            square.thickness * zoomLevel
        );
    }
}

void Canvas::renderRectangles(ImDrawList* drawList) const {
    for (const auto& rect : rectangles) {
        ImVec2 screenStart = inverseTransformCoordinates(rect.start);
        ImVec2 screenEnd = inverseTransformCoordinates(rect.end);
        ImVec2 screenTopRight(screenEnd.x, screenStart.y);
        ImVec2 screenBottomLeft(screenStart.x, screenEnd.y);
        
        drawList->AddQuad(
            screenStart,
            screenTopRight,
            screenEnd,
            screenBottomLeft,
            rect.color,
            rect.thickness * zoomLevel
        );
    }
}

void Canvas::renderSplines(ImDrawList* drawList) const {
    for (const auto& spline : splines) {
        std::vector<ImVec2> curvePoints = calculateSplinePoints(spline.controlPoints, spline.isClosed);
        
        for (size_t i = 1; i < curvePoints.size(); ++i) {
            ImVec2 p1 = inverseTransformCoordinates(curvePoints[i - 1]);
            ImVec2 p2 = inverseTransformCoordinates(curvePoints[i]);
            drawList->AddLine(p1, p2, spline.color, spline.thickness * zoomLevel);
        }
    }
}

void Canvas::renderBezierCurves(ImDrawList* drawList) const {
    for (const auto& curve : bezierCurves) {
        // Draw control points and handles
        for (size_t i = 0; i < curve.controlPoints.size(); ++i) {
            ImVec2 screenPos = inverseTransformCoordinates(curve.controlPoints[i]);
            drawList->AddCircleFilled(screenPos, 3.0f * zoomLevel, Colors::CONTROL_POINT);
            
            if (i > 0) {
                ImVec2 prevPos = inverseTransformCoordinates(curve.controlPoints[i - 1]);
                drawList->AddLine(prevPos, screenPos, Colors::CONTROL_LINE, 1.0f * zoomLevel);
            }
        }
        
        // Draw the actual curve
        std::vector<ImVec2> curvePoints;
        const int STEPS = 50;
        for (int i = 0; i <= STEPS; ++i) {
            float t = curve.startT + (curve.endT - curve.startT) * static_cast<float>(i) / static_cast<float>(STEPS);
            curvePoints.push_back(calculateBezierPoint(curve.controlPoints, t));
        }
        
        for (size_t i = 1; i < curvePoints.size(); ++i) {
            ImVec2 p1 = inverseTransformCoordinates(curvePoints[i - 1]);
            ImVec2 p2 = inverseTransformCoordinates(curvePoints[i]);
            drawList->AddLine(p1, p2, curve.color, curve.thickness * zoomLevel);
        }
    }
}

void Canvas::setTruncationPoints(float start, float end) {
    curveStartT = std::clamp(start, 0.0f, 1.0f);
    curveEndT = std::clamp(end, 0.0f, 1.0f);
    if (curveEndT < curveStartT) {
        std::swap(curveStartT, curveEndT);
    }
    
    // Update the most recently created Bezier curve if it exists
    if (!bezierCurves.empty()) {
        bezierCurves.back().startT = curveStartT;
        bezierCurves.back().endT = curveEndT;
    }
}

std::pair<float, float> Canvas::getTruncationPoints() const {
    return {curveStartT, curveEndT};
}

void Canvas::toggleSplineClosure() {
    if (!splines.empty()) {
        splines.back().isClosed = !splines.back().isClosed;
    }
}

bool Canvas::isSplineClosed() const {
    return !splines.empty() && splines.back().isClosed;
}

ImVec2 Canvas::calculateBezierPoint(const std::vector<ImVec2>& points, float t) const {
    if (points.size() != 4) return ImVec2(0, 0);
    
    float u = 1.0f - t;
    float u2 = u * u;
    float u3 = u2 * u;
    float t2 = t * t;
    float t3 = t2 * t;
    
    // Cubic Bezier formula: B(t) = (1-t)³P₀ + 3(1-t)²tP₁ + 3(1-t)t²P₂ + t³P₃
    ImVec2 result = points[0] * u3 +                  // First term:  (1-t)³P₀
                    points[1] * (3.0f * u2 * t) +     // Second term: 3(1-t)²tP₁
                    points[2] * (3.0f * u * t2) +     // Third term:  3(1-t)t²P₂
                    points[3] * t3;                   // Fourth term: t³P₃
    
    return result;
}

std::vector<ImVec2> Canvas::calculateSplinePoints(const std::vector<ImVec2>& controlPoints, bool isClosed) const {
    if (controlPoints.size() < 2) return {};
    
    std::vector<ImVec2> points;
    const int STEPS = 20; // Points per segment
    
    auto catmullRom = [](const ImVec2& p0, const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, float t) {
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
    };
    
    size_t size = controlPoints.size();
    for (size_t i = 0; i < (isClosed ? size : size - 1); ++i) {
        ImVec2 p0 = controlPoints[(i - 1 + size) % size];
        ImVec2 p1 = controlPoints[i];
        ImVec2 p2 = controlPoints[(i + 1) % size];
        ImVec2 p3 = controlPoints[(i + 2) % size];
        
        for (int step = 0; step < STEPS; ++step) {
            float t = static_cast<float>(step) / static_cast<float>(STEPS);
            points.push_back(catmullRom(p0, p1, p2, p3, t));
        }
    }
    
    if (!isClosed) {
        points.push_back(controlPoints.back());
    }
    
    return points;
}

void Canvas::renderSnapIndicator(ImDrawList* drawList, const ImVec2& pos, const std::string& type) const {
    ImVec2 screenPos = inverseTransformCoordinates(pos);
    ImU32 color = IM_COL32(0, 255, 255, 255); // Cyan color for snap indicators
    float size = 5.0f * zoomLevel; // Scale indicator size with zoom
    
    if (type == "endpoint") {
        drawList->AddCircle(screenPos, size, color);
    } else if (type == "midpoint") {
        drawList->AddRect(
            screenPos - ImVec2(size, size),
            screenPos + ImVec2(size, size),
            color
        );
    } else if (type == "intersection") {
        drawList->AddCircle(screenPos, size, color);
        drawList->AddCircle(screenPos, size * 1.6f, color);
    } else if (type == "center") {
        drawList->AddCircle(screenPos, size, color);
        float lineSize = size * 1.6f;
        drawList->AddLine(
            screenPos - ImVec2(lineSize, 0),
            screenPos + ImVec2(lineSize, 0),
            color
        );
        drawList->AddLine(
            screenPos - ImVec2(0, lineSize),
            screenPos + ImVec2(0, lineSize),
            color
        );
    } else if (type == "perpendicular") {
        drawList->AddRect(
            screenPos - ImVec2(size, size),
            screenPos + ImVec2(size, size),
            color,
            45.0f
        );
    }
}

void Canvas::clearAll() {
    points.clear();
    lines.clear();
    circles.clear();
    triangles.clear();
    squares.clear();
    rectangles.clear();
    splines.clear();
    bezierCurves.clear();
    isDrawing = false;
    isEditingCurve = false;
    currentCurvePoints.clear();
    clickCount = 0;
}

} // namespace Drawing 