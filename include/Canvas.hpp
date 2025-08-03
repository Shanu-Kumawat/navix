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
    BezierCurve,
    Bellows,
    BallBearing,
    Spring2D
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
    void setShowGrid(bool show) { showGrid = show; }
    bool isGridVisible() const { return showGrid; }
    void setShowControlPoints(bool show) { showControlPoints = show; }
    bool getShowControlPoints() const { return showControlPoints; }

    // Accessors for current tool dimensions
    void setLineLength(float length) { lineLength = std::max(0.1f, length); }
    float getLineLength() const { return lineLength; }
    void setFixedLineLength(bool fixed) { fixedLineLength = fixed; }
    bool isFixedLineLength() const { return fixedLineLength; }
    
    void setCircleRadius(float radius) { circleRadius = std::max(0.1f, radius); }
    float getCircleRadius() const { return circleRadius; }
    void setFixedCircleRadius(bool fixed) { fixedCircleRadius = fixed; }
    bool isFixedCircleRadius() const { return fixedCircleRadius; }
    
    void setSquareSize(float size) { squareSize = std::max(0.1f, size); }
    float getSquareSize() const { return squareSize; }
    void setFixedSquareSize(bool fixed) { fixedSquareSize = fixed; }
    bool isFixedSquareSize() const { return fixedSquareSize; }
    
    void setRectangleWidth(float width) { rectangleWidth = std::max(0.1f, width); }
    float getRectangleWidth() const { return rectangleWidth; }
    void setRectangleHeight(float height) { rectangleHeight = std::max(0.1f, height); }
    float getRectangleHeight() const { return rectangleHeight; }
    void setFixedRectangleSize(bool fixed) { fixedRectangleSize = fixed; }
    bool isFixedRectangleSize() const { return fixedRectangleSize; }
    
    void setTriangleSide(float side) { triangleSide = std::max(0.1f, side); }
    float getTriangleSide() const { return triangleSide; }
    void setFixedTriangleSize(bool fixed) { fixedTriangleSize = fixed; }
    bool isFixedTriangleSize() const { return fixedTriangleSize; }

    // Selection methods
    void selectShape(Shape* shape) { selectedShape = shape; }
    void clearSelection();
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
        bool showGrid;
    };

    void setUnitSystem(Drawing::UnitSystem system) {
        currentUnits = system;
        updateConversionFactor();
    }

    float getDisplayValue(float pixels) const {
        using namespace Drawing;
        switch(currentUnits) {
            case UnitSystem::Millimeters: 
                return Math::pixelsToMillimeters(pixels, dpi);
            case UnitSystem::Centimeters: 
                return Math::pixelsToCentimeters(pixels, dpi);
            case UnitSystem::Inches: 
                return pixels / dpi;
            default: return pixels;
        }
    }

    float getInternalValue(float displayValue) const {
        using namespace Drawing;
        switch(currentUnits) {
            case UnitSystem::Millimeters: 
                return Math::millimetersToPixels(displayValue, dpi);
            case UnitSystem::Centimeters: 
                return Math::centimetersToPixels(displayValue, dpi);
            case UnitSystem::Inches: 
                return displayValue * dpi;
            default: return displayValue;
        }
    }

    // Unit system methods
    bool hasUnitSystem() const { return currentUnits != UnitSystem::Pixels; }
    UnitSystem getCurrentUnit() const { return currentUnits; }
    float getZoomLevel() const { return zoomLevel; }
    
    // Bellows-specific methods
    void fitBellowsToView();
    const Bellows* findOrCreateBellows() const;
    const BallBearing* findOrCreateBallBearing() const;

    void setSpring2DShape(std::unique_ptr<Shape> spring);

    // Spring2D parameters for click-to-place and preview
    float springOuterDiameter = 44.5f;
    float springWireDiameter = 7.25f;
    float springFreeLength = 68.0f;
    int springNumCoils = 6;

    void renderBellows(ImDrawList* drawList) const;
    void renderSprings2D(ImDrawList* drawList) const;

    // Viewport culling methods
    bool isPointInViewport(const ImVec2& point) const {
        ImVec2 transformed = transformCoordinates(point);
        return transformed.x >= 0 && transformed.x <= windowWidth &&
               transformed.y >= 0 && transformed.y <= windowHeight;
    }

    bool isRectInViewport(const ImVec2& min, const ImVec2& max) const {
        ImVec2 transformedMin = transformCoordinates(min);
        ImVec2 transformedMax = transformCoordinates(max);
        return !(transformedMax.x < 0 || transformedMin.x > windowWidth ||
                transformedMax.y < 0 || transformedMin.y > windowHeight);
    }

    void addShape(std::unique_ptr<Shape> shape) { shapes.push_back(std::move(shape)); }

    // Update all ShockAbsorberEnd2D shapes for a given parent spring
    void updateShockAbsorberEndsForSpring(const Drawing::Spring2D* spring);

    const std::vector<std::unique_ptr<Shape>>& getShapes() const { return shapes; }

    void setSpring2DShape(std::unique_ptr<Shape> spring);

    // Spring2D parameters for click-to-place and preview
    float springOuterDiameter = 44.5f;
    float springWireDiameter = 7.25f;
    float springFreeLength = 68.0f;
    int springNumCoils = 6;

    void renderBellows(ImDrawList* drawList) const;
    void renderSprings2D(ImDrawList* drawList) const;

    // Viewport culling methods
    bool isPointInViewport(const ImVec2& point) const {
        ImVec2 transformed = transformCoordinates(point);
        return transformed.x >= 0 && transformed.x <= windowWidth &&
               transformed.y >= 0 && transformed.y <= windowHeight;
    }

    bool isRectInViewport(const ImVec2& min, const ImVec2& max) const {
        ImVec2 transformedMin = transformCoordinates(min);
        ImVec2 transformedMax = transformCoordinates(max);
        return !(transformedMax.x < 0 || transformedMin.x > windowWidth ||
                transformedMax.y < 0 || transformedMin.y > windowHeight);
    }

    void addShape(std::unique_ptr<Shape> shape) { shapes.push_back(std::move(shape)); }

    // Update all ShockAbsorberEnd2D shapes for a given parent spring
    void updateShockAbsorberEndsForSpring(const Drawing::Spring2D* spring);

    const std::vector<std::unique_ptr<Shape>>& getShapes() const { return shapes; }

private:
    // Drawing state
    DrawingMode currentMode{DrawingMode::None};
    bool isDrawing{false};
    bool isFirstClick{true};
    int clickCount{0};
    bool snapToGrid{true};
    bool isDraggingCanvas{false};
    
    // Transform state
    float zoomLevel{1.0f};
    float scaleFactor{1.0f};
    ImVec2 panOffset{0.0f, 0.0f};
    ImVec2 zoomCenter{0.0f, 0.0f};
    
    // Grid & snapping
    float gridSpacing{Constants::DEFAULT_GRID_SPACING};
    bool showGrid{true};
    
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
    
    // Line specific state
    
    // Circle specific state
    
    // Square specific state
    
    // Rectangle specific state
    
    // Triangle specific state
    
    float windowX{0.0f};
    float windowY{0.0f};
    float windowWidth{0.0f};
    float windowHeight{0.0f};

    bool showControlPoints{true};
    Shape* selectedShape{nullptr};

    // Drawing data
    int selectedShapeIndex = -1;
    
    // History management
    std::vector<HistoryState> history;
    size_t currentHistoryIndex{0};
    static constexpr size_t MAX_HISTORY_SIZE = 50;
    
    ImVec2 mousePos;
    ImVec2 lastMousePos;
    std::vector<ImVec2> splinePoints;
    
    bool isDragging = false;
    bool isPanning = false;
    
    // Current dimensions - use fixed defaults
    float lineLength = 100.0f;  // Default line length
    bool fixedLineLength = false;  // Whether to use fixed or dynamic line length (set to false for variable by default)
    float circleRadius = 50.0f;  // Default circle radius
    bool fixedCircleRadius = false;  // Whether to use fixed or dynamic circle radius
    float squareSize = 100.0f;  // Default square size
    bool fixedSquareSize = false;  // Whether to use fixed or dynamic square size
    float rectangleWidth{150.0f};
    float rectangleHeight{100.0f};
    bool fixedRectangleSize = false;  // Whether to use fixed or dynamic rectangle size
    float triangleSide{100.0f};
    bool fixedTriangleSize = false;  // Whether to use fixed or dynamic triangle size
    
    // Helper methods
    void restoreHistoryState(const HistoryState& state);
    ImVec2 getSnappedPoint(const ImVec2& point) const;
    bool trySnapToExistingPoint(ImVec2& point) const;
    void renderPreview(ImDrawList* drawList, const ImVec2& currentPos) const;
    std::optional<ImVec2> findNearestPoint(const ImVec2& point, float threshold) const;
    ImVec2 findNearestSnapPoint(const ImVec2& pos) const;
    
    // Vector math helpers
    static ImVec2 normalizeVector(const ImVec2& vec) {
        float length = std::sqrt(vec.x * vec.x + vec.y * vec.y);
        if (length < 0.0001f) return ImVec2(1.0f, 0.0f); // Avoid division by zero
        return ImVec2(vec.x / length, vec.y / length);
    }
    
    // Drawing handlers
    void handlePointDrawing(ImDrawList* drawList, const ImVec2& currentPos);
    void handleLineDrawing(ImDrawList* drawList, const ImVec2& currentPos);
    void handleCircleDrawing(ImDrawList* drawList, const ImVec2& currentPos);
    void handleTriangleDrawing(ImDrawList* drawList, const ImVec2& currentPos);
    void handleSquareDrawing(ImDrawList* drawList, const ImVec2& currentPos);
    void handleRectangleDrawing(ImDrawList* drawList, const ImVec2& currentPos);
    void handleSplineDrawing(ImDrawList* drawList, const ImVec2& currentPos);
    void handleBezierDrawing(ImDrawList* drawList, const ImVec2& currentPos);
    void handleBellowsDrawing(ImDrawList* drawList, const ImVec2& currentPos);
    void handleBallBearingDrawing(ImDrawList* drawList, const ImVec2& currentPos);
    void handleSpring2DDrawing(ImDrawList* drawList, const ImVec2& currentPos);
    
    // Preview methods
    void previewPoint(ImDrawList* drawList, const ImVec2& pos) const;
    void previewLine(ImDrawList* drawList, const ImVec2& start, const ImVec2& end) const;
    void previewCircle(ImDrawList* drawList, const ImVec2& center, float radius) const;
    void previewTriangle(ImDrawList* drawList, const std::array<ImVec2, 3>& points, int count) const;
    void previewSquare(ImDrawList* drawList, const ImVec2& start, const ImVec2& end) const;
    void previewRectangle(ImDrawList* drawList, const ImVec2& start, const ImVec2& end) const;
    void previewSpline(ImDrawList* drawList, const std::vector<ImVec2>& points) const;
    void previewBezier(ImDrawList* drawList, const std::vector<ImVec2>& points) const;
    void previewBellows(ImDrawList* drawList, const ImVec2& start, const ImVec2& end) const;
    void previewBallBearing(ImDrawList* drawList, const ImVec2& center, float radius) const;
    void previewSpring2D(ImDrawList* drawList, const ImVec2& center) const;
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
    void renderBallBearings(ImDrawList* drawList) const;

    // Curve calculation methods
    ImVec2 calculateBezierPoint(const std::vector<ImVec2>& points, float t) const;
    std::vector<ImVec2> calculateSplinePoints(const std::vector<ImVec2>& controlPoints, bool isClosed) const;
    ImVec2 catmullRomPoint(const ImVec2& p0, const ImVec2& p1, 
                          const ImVec2& p2, const ImVec2& p3, float t) const;

    // Update helper methods
    void updateSelectedShape(const ImVec2& mousePos);
    void updateDrawingPreview(const ImVec2& mousePos);

    Drawing::UnitSystem currentUnits = Drawing::UnitSystem::Millimeters;
    float conversionFactor = 1.0f;
    float dpi = 96.0f;

    void updateConversionFactor() {
        switch(currentUnits) {
            case Drawing::UnitSystem::Millimeters: conversionFactor = dpi / 25.4f; break;
            case Drawing::UnitSystem::Centimeters: conversionFactor = dpi / 2.54f; break;
            case Drawing::UnitSystem::Inches: conversionFactor = dpi; break;
            default: conversionFactor = 1.0f;
        }
    }
};

} // namespace Drawing 