#pragma once

#include <imgui.h>
#include <memory>
#include <vector>
#include <optional>
#include <string>
#include "shapes/BasicShapes.hpp"
#include "shapes/ComplexShapes.hpp"
#include "utils/MathUtils.hpp"
#include "Constants.hpp"

namespace Drawing {

class Canvas {
public:
    Canvas();
    ~Canvas() = default;

    void handleInput();
    void render(ImDrawList* drawList);
    
    // Transformation methods
    ImVec2 transformCoordinates(const ImVec2& point) const;
    ImVec2 inverseTransformCoordinates(const ImVec2& point) const;
    
    // Drawing state methods
    void setDrawingMode(DrawingMode mode);
    void reset();
    
    // Grid methods
    void renderGrid(ImDrawList* drawList);
    void updateZoom(float delta);
    void updatePan(const ImVec2& delta);

    // Settings
    void setSnapToGrid(bool enabled) { snapToGrid = enabled; }
    bool isSnapToGridEnabled() const { return snapToGrid; }
    void setGridSpacing(float spacing) { gridSpacing = spacing; }

    void setWindowInfo(float x, float y, float width, float height) {
        windowX = x;
        windowY = y;
        windowWidth = width;
        windowHeight = height;
    }

    void clearAll();  // Declaration only, implementation will be in Canvas.cpp

    struct SnapPoint {
        ImVec2 point;
        std::string type;  // "endpoint", "midpoint", "intersection", "center", "perpendicular"
    };

    // New snapping methods
    std::optional<SnapPoint> findSnapPoint(const ImVec2& mousePos) const;
    std::optional<ImVec2> findMidPoint(const ImVec2& mousePos) const;
    std::optional<ImVec2> findIntersection(const ImVec2& mousePos) const;
    std::optional<ImVec2> findPerpendicular(const ImVec2& mousePos) const;
    std::optional<ImVec2> findCenter(const ImVec2& mousePos) const;
    void renderSnapIndicator(ImDrawList* drawList, const ImVec2& pos, const std::string& type) const;

    // Curve manipulation methods
    void setTruncationPoints(float start, float end);
    std::pair<float, float> getTruncationPoints() const;
    void toggleSplineClosure();
    bool isSplineClosed() const;

private:
    // Drawing state
    DrawingMode currentMode;
    bool isDrawing;
    bool isFirstClick;
    int clickCount;
    bool snapToGrid;
    
    // Transform state
    float zoomLevel;
    ImVec2 panOffset;
    float gridSpacing;
    
    // Shape collections
    std::vector<Point> points;
    std::vector<Line> lines;
    std::vector<Circle> circles;
    std::vector<Triangle> triangles;
    std::vector<Square> squares;
    std::vector<Rectangle> rectangles;
    std::vector<Spline> splines;
    std::vector<BezierCurve> bezierCurves;
    
    // Temporary drawing state
    ImVec2 startPoint;
    ImVec2 endPoint;
    std::array<ImVec2, 3> trianglePoints;
    std::vector<ImVec2> currentSplinePoints;
    
    // Curve editing state
    std::vector<ImVec2> currentCurvePoints;
    bool isEditingCurve;
    float curveStartT;
    float curveEndT;
    
    // Helper methods
    ImVec2 getSnappedPoint(const ImVec2& point) const;
    bool trySnapToExistingPoint(ImVec2& point) const;
    void renderPreview(ImDrawList* drawList, const ImVec2& currentPos);
    std::optional<ImVec2> findNearestPoint(const ImVec2& point, float threshold) const;
    
    // Drawing methods
    void handlePointDrawing(ImDrawList* drawList, const ImVec2& currentPos);
    void handleLineDrawing(ImDrawList* drawList, const ImVec2& currentPos);
    void handleCircleDrawing(ImDrawList* drawList, const ImVec2& currentPos);
    void handleTriangleDrawing(ImDrawList* drawList, const ImVec2& currentPos);
    void handleSquareDrawing(ImDrawList* drawList, const ImVec2& currentPos);
    void handleRectangleDrawing(ImDrawList* drawList, const ImVec2& currentPos);
    void handleSplineDrawing(ImDrawList* drawList, const ImVec2& currentPos);
    void handleBezierDrawing(ImDrawList* drawList, const ImVec2& currentPos);
    
    // Render methods
    void renderPoints(ImDrawList* drawList) const;
    void renderLines(ImDrawList* drawList) const;
    void renderCircles(ImDrawList* drawList) const;
    void renderTriangles(ImDrawList* drawList) const;
    void renderSquares(ImDrawList* drawList) const;
    void renderRectangles(ImDrawList* drawList) const;
    void renderSplines(ImDrawList* drawList) const;
    void renderBezierCurves(ImDrawList* drawList) const;
    
    // Preview methods
    void previewPoint(ImDrawList* drawList, const ImVec2& pos) const;
    void previewLine(ImDrawList* drawList, const ImVec2& start, const ImVec2& end) const;
    void previewCircle(ImDrawList* drawList, const ImVec2& center, float radius) const;
    void previewTriangle(ImDrawList* drawList, const std::array<ImVec2, 3>& points, int count) const;
    void previewSquare(ImDrawList* drawList, const ImVec2& start, const ImVec2& end) const;
    void previewRectangle(ImDrawList* drawList, const ImVec2& start, const ImVec2& end) const;
    void previewSpline(ImDrawList* drawList, const std::vector<ImVec2>& points) const;
    void previewBezier(ImDrawList* drawList, const std::vector<ImVec2>& points) const;
    void drawDashedLine(ImDrawList* drawList, const ImVec2& p1, const ImVec2& p2, 
                       ImU32 color, float thickness, float dash_length) const;

    float windowX = 0.0f;
    float windowY = 0.0f;
    float windowWidth = 0.0f;
    float windowHeight = 0.0f;

    // Curve calculation methods
    ImVec2 calculateBezierPoint(const std::vector<ImVec2>& points, float t) const;
    std::vector<ImVec2> calculateSplinePoints(const std::vector<ImVec2>& controlPoints, bool isClosed) const;
};

} // namespace Drawing 