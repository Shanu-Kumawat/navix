#pragma once

#include <imgui.h>
#include <memory>
#include <vector>
#include <optional>
#include <string>
#include <algorithm>  
#include "shapes/BasicShapes.hpp"
#include "shapes/ComplexShapes.hpp"
#include "utils/MathUtils.hpp"
#include "Constants.hpp"

namespace Drawing {

enum class DrawingMode {
    None,
    Select,
    Point,
    Line,
    Circle,
    Triangle,
    Square,
    Rectangle,
    Spline,
    BezierCurve
};

class Canvas {
public:
    Canvas();
    ~Canvas() = default;

    // Main loop methods
    void handleInput();
    void update(const ImVec2& mousePos);
    void render(ImDrawList* drawList);
    
    // Input handling methods
    void handleSelection(const ImVec2& mousePos);
    void handleDrawing(const ImVec2& mousePos);
    void handleCurveManipulation(const ImVec2& mousePos);
    
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
    void setShowControlPoints(bool show) { showControlPoints = show; }
    bool getShowControlPoints() const { return showControlPoints; }

    // Selection methods
    void selectShape(Shape* shape) { selectedShape = shape; }
    void clearSelection() { selectedShape = nullptr; }
    Shape* getSelectedShape() const { return selectedShape; }

    // Shape manipulation methods
    void deleteSelectedShape();
    void duplicateSelectedShape();
    void moveSelectedShape(const ImVec2& delta);
    void rotateSelectedShape(float angle);
    void scaleSelectedShape(float factor);

    // History management
    void undo();
    void redo();
    void saveToHistory();

    void setWindowInfo(float x, float y, float width, float height) {
        windowX = x;
        windowY = y;
        windowWidth = width;
        windowHeight = height;
    }

    void clearAll();

    struct SnapPoint {
        ImVec2 point;
        std::string type;
    };

    // Snapping methods
    std::optional<SnapPoint> findSnapPoint(const ImVec2& mousePos) const;
    std::optional<ImVec2> findMidPoint(const ImVec2& mousePos) const;
    std::optional<ImVec2> findIntersection(const ImVec2& mousePos) const;
    std::optional<ImVec2> findPerpendicular(const ImVec2& mousePos) const;
    std::optional<ImVec2> findCenter(const ImVec2& mousePos) const;
    void renderSnapIndicator(ImDrawList* drawList, const ImVec2& pos, const std::string& type) const;

    // Make HistoryState public since it's used in method signatures
    struct HistoryState {
        std::vector<std::unique_ptr<Shape>> shapes;
        ImVec2 panOffset;
        float zoomLevel;
    };

private:
    // Drawing state
    DrawingMode currentMode{DrawingMode::None};
    bool isDrawing{false};
    bool isFirstClick{true};
    int clickCount{0};
    bool snapToGrid{true};
    
    // Transform state
    float zoomLevel{1.0f};
    ImVec2 panOffset{0.0f, 0.0f};
    float gridSpacing{Constants::DEFAULT_GRID_SPACING};
    
    // Shape collections
    std::vector<std::unique_ptr<Shape>> shapes;
    
    // Temporary drawing state
    ImVec2 startPoint{0.0f, 0.0f};
    ImVec2 endPoint{0.0f, 0.0f};
    std::array<ImVec2, 3> trianglePoints{};
    std::vector<ImVec2> currentSplinePoints;
    
    // Curve editing state
    std::vector<ImVec2> currentCurvePoints;
    bool isEditingCurve{false};
    float curveStartT{0.0f};
    float curveEndT{1.0f};

    // History management
    std::vector<HistoryState> history;
    size_t currentHistoryIndex{0};
    static constexpr size_t MAX_HISTORY_SIZE = 50;

    float windowX{0.0f};
    float windowY{0.0f};
    float windowWidth{0.0f};
    float windowHeight{0.0f};

    bool showControlPoints{true};
    Shape* selectedShape{nullptr};

    // Helper methods
    void restoreHistoryState(const HistoryState& state);
    ImVec2 getSnappedPoint(const ImVec2& point) const;
    bool trySnapToExistingPoint(ImVec2& point) const;
    void renderPreview(ImDrawList* drawList, const ImVec2& currentPos);
    std::optional<ImVec2> findNearestPoint(const ImVec2& point, float threshold) const;
    ImVec2 findNearestSnapPoint(const ImVec2& pos) const;
    
    // Drawing handlers
    void handlePointDrawing(ImDrawList* drawList, const ImVec2& currentPos);
    void handleLineDrawing(ImDrawList* drawList, const ImVec2& currentPos);
    void handleCircleDrawing(ImDrawList* drawList, const ImVec2& currentPos);
    void handleTriangleDrawing(ImDrawList* drawList, const ImVec2& currentPos);
    void handleSquareDrawing(ImDrawList* drawList, const ImVec2& currentPos);
    void handleRectangleDrawing(ImDrawList* drawList, const ImVec2& currentPos);
    void handleSplineDrawing(ImDrawList* drawList, const ImVec2& currentPos);
    void handleBezierDrawing(ImDrawList* drawList, const ImVec2& currentPos);
    
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

    // Shape rendering
    void drawShape(const Shape& shape) const;
    void renderShapes(ImDrawList* drawList) const;
    void renderPoints(ImDrawList* drawList) const;
    void renderLines(ImDrawList* drawList) const;
    void renderCircles(ImDrawList* drawList) const;
    void renderTriangles(ImDrawList* drawList) const;
    void renderSquares(ImDrawList* drawList) const;
    void renderRectangles(ImDrawList* drawList) const;
    void renderSplines(ImDrawList* drawList) const;
    void renderBezierCurves(ImDrawList* drawList) const;

    // Curve calculation methods
    ImVec2 calculateBezierPoint(const std::vector<ImVec2>& points, float t) const;
    std::vector<ImVec2> calculateSplinePoints(const std::vector<ImVec2>& controlPoints, bool isClosed) const;
    ImVec2 catmullRomPoint(const ImVec2& p0, const ImVec2& p1, 
                          const ImVec2& p2, const ImVec2& p3, float t) const;

    // Update helper methods
    void updateSelectedShape(const ImVec2& mousePos);
    void updateDrawingPreview(const ImVec2& mousePos);
};

} // namespace Drawing 