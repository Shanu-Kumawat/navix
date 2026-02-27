#include <glm/glm.hpp>
#pragma once

#include <imgui.h>
#include <memory>
#include <vector>
#include <optional>
#include <string>
#include <algorithm>  
#include "shapes/BasicShapes.hpp"
#include "shapes/ComplexShapes.hpp"
#include "commands/CommandManager.hpp"
#include "shapes/ShockAbsorberBottomEnd.hpp"
#include "utils/MathUtils.hpp"
#include "Constants.hpp"
#include "Renderer2D.hpp"
#include "InputController.hpp"
#include "SceneModel.hpp"

namespace Drawing {
using namespace Drawing::Math;



class CanvasSnapshotCommand;

class Canvas {
public:
    friend class CanvasSnapshotCommand;
    void render(ImDrawList* drawList) { if (renderer) renderer->render(drawList, *sceneModel); }
    std::vector<glm::dvec2> calculateSplinePoints(const std::vector<glm::dvec2>& controlPoints, bool isClosed) const;

    // Getters for Renderer2D
    float getZoomLevel() const { return zoomLevel; }
    glm::dvec2 getPanOffset() const { return panOffset; }
    bool isGridVisible() const { return showGrid; }
    float getGridSpacing() const { return gridSpacing; }
    float getWindowWidth() const { return windowWidth; }
    float getWindowHeight() const { return windowHeight; }
    float getWindowX() const { return windowX; }
    float getWindowY() const { return windowY; }
    bool getIsDrawing() const { return isDrawing; }
    glm::dvec2 getStartPoint() const { return startPoint; }
    int getClickCount() const { return clickCount; }
    const std::array<glm::dvec2, 3>& getTrianglePoints() const { return trianglePoints; }
    const std::vector<glm::dvec2>& getCurrentSplinePoints() const { return currentSplinePoints; }
    const std::vector<glm::dvec2>& getCurrentCurvePoints() const { return currentCurvePoints; }
    bool getShowControlPoints() const { return showControlPoints; }
    Shape* getSelectedShape() const { return selectedShape; }
    
    // Make these public for now
    

    Canvas();
    ~Canvas() = default;

    // Main loop methods
    void handleInput();
    void update(const glm::dvec2& mousePos);
    
    // Input handling methods
    void handleSelection(const glm::dvec2& mousePos);
    void handleDrawing(const glm::dvec2& mousePos);
    void handleCurveManipulation(const glm::dvec2& mousePos);
    
    // Transformation methods
    glm::dvec2 transformCoordinates(const glm::dvec2& point) const;
    glm::dvec2 inverseTransformCoordinates(const glm::dvec2& point) const;
    
    // Drawing state methods
    void setDrawingMode(DrawingMode mode);
    void reset();
    
    // Grid methods
    void renderGrid(ImDrawList* drawList);
    void updateZoom(float delta);
    void updatePan(const glm::dvec2& delta);

    // Settings
    void setSnapToGrid(bool enabled) { snapToGrid = enabled; }
    bool isSnapToGridEnabled() const { return snapToGrid; }
    void setGridSpacing(float spacing) { gridSpacing = spacing; }
    void setShowGrid(bool show) { showGrid = show; }
    void setShowControlPoints(bool show) { showControlPoints = show; }

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

    // Shape manipulation methods
    void deleteSelectedShape();
    void duplicateSelectedShape();
    void moveSelectedShape(const glm::dvec2& delta);
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
        glm::dvec2 point;
        std::string type;
    };

    // Snapping methods
    std::optional<SnapPoint> findSnapPoint(const glm::dvec2& mousePos) const;
    std::optional<glm::dvec2> findMidPoint(const glm::dvec2& mousePos) const;
    std::optional<glm::dvec2> findIntersection(const glm::dvec2& mousePos) const;
    std::optional<glm::dvec2> findPerpendicular(const glm::dvec2& mousePos) const;
    std::optional<glm::dvec2> findCenter(const glm::dvec2& mousePos) const;
    void renderSnapIndicator(ImDrawList* drawList, const glm::dvec2& pos, const std::string& type) const;

    // Make HistoryState public since it's used in method signatures
    struct HistoryState {
        std::vector<std::unique_ptr<Shape>> shapes;
        glm::dvec2 panOffset;
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
    
    // Unified complex shape management
    void fitBellowsToView();
    
    // Unified shape finding (replaces inconsistent findOrCreate methods)
    template<typename T>
    const T* findSelectedShapeOfType() const {
        if (selectedShape && selectedShape->type == T::GetShapeType()) {
            return static_cast<const T*>(selectedShape);
        }
        return nullptr;
    }
    
    template<typename T>
    const T* findFirstShapeOfType() const {
        for (const auto& shape : shapes) {
            if (shape->type == T::GetShapeType()) {
                return static_cast<const T*>(shape.get());
            }
        }
        return nullptr;
    }
    
    template<typename T>
    std::vector<const T*> findAllShapesOfType() const {
        std::vector<const T*> result;
        for (const auto& shape : shapes) {
            if (shape->type == T::GetShapeType()) {
                result.push_back(static_cast<const T*>(shape.get()));
            }
        }
        return result;
    }
    
    // Legacy methods for backward compatibility
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
    bool isPointInViewport(const glm::dvec2& point) const {
        glm::dvec2 transformed = transformCoordinates(point);
        return transformed.x >= 0 && transformed.x <= windowWidth &&
               transformed.y >= 0 && transformed.y <= windowHeight;
    }

    bool isRectInViewport(const glm::dvec2& min, const glm::dvec2& max) const {
        glm::dvec2 transformedMin = transformCoordinates(min);
        glm::dvec2 transformedMax = transformCoordinates(max);
        return !(transformedMax.x < 0 || transformedMin.x > windowWidth ||
                transformedMax.y < 0 || transformedMin.y > windowHeight);
    }

    void addShape(std::unique_ptr<Shape> shape) { shapes.push_back(std::move(shape)); }

    // Update all ShockAbsorberEnd2D shapes for a given parent spring
    void updateShockAbsorberEndsForSpring(const Drawing::Spring2D* spring);

    const std::vector<std::unique_ptr<Shape>>& getShapes() const { return shapes; }
    
    // Shock absorber assembly detection
    struct ShockAbsorberAssembly {
        const Spring2D* spring;
        const ShockAbsorberEnd2D* topEnd;
        const ShockAbsorberBottomEnd* bottomEnd;
        // Assembly is complete if we have spring and both end components for 3D viewing
        bool isComplete() const { return spring && topEnd && bottomEnd; }
    };
    
    // Find complete shock absorber assemblies
    std::vector<ShockAbsorberAssembly> findShockAbsorberAssemblies() const;
    bool hasCompleteShockAbsorberAssembly() const;

private:
    std::unique_ptr<Core::Renderer2D> renderer;
    std::unique_ptr<Core::InputController> inputController;
    std::unique_ptr<Core::SceneModel> sceneModel;

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
    glm::dvec2 panOffset{0.0f, 0.0f};
    glm::dvec2 zoomCenter{0.0f, 0.0f};
    
    // Grid & snapping
    float gridSpacing{Constants::DEFAULT_GRID_SPACING};
    bool showGrid{true};
    
    // Shape collections
    std::vector<std::unique_ptr<Shape>> shapes;
    
    // Temporary drawing state
    glm::dvec2 startPoint{0.0f, 0.0f};
    glm::dvec2 endPoint{0.0f, 0.0f};
    std::array<glm::dvec2, 3> trianglePoints{};
    std::vector<glm::dvec2> currentSplinePoints;
    
    // Curve editing state
    std::vector<glm::dvec2> currentCurvePoints;
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
    Core::Commands::CommandManager commandManager;
    
    glm::dvec2 mousePos;
    glm::dvec2 lastMousePos;
    std::vector<glm::dvec2> splinePoints;
    
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
    glm::dvec2 getSnappedPoint(const glm::dvec2& point) const;
    bool trySnapToExistingPoint(glm::dvec2& point) const;
    void renderPreview(ImDrawList* drawList, const glm::dvec2& currentPos) const;
    std::optional<glm::dvec2> findNearestPoint(const glm::dvec2& point, float threshold) const;
    glm::dvec2 findNearestSnapPoint(const glm::dvec2& pos) const;
    
    // Vector math helpers
    static glm::dvec2 normalizeVector(const glm::dvec2& vec) {
        float length = std::sqrt(vec.x * vec.x + vec.y * vec.y);
        if (length < 0.0001f) return glm::dvec2(1.0f, 0.0f); // Avoid division by zero
        return glm::dvec2(vec.x / length, vec.y / length);
    }
    
    // Drawing handlers
    void handlePointDrawing(ImDrawList* drawList, const glm::dvec2& currentPos);
    void handleLineDrawing(ImDrawList* drawList, const glm::dvec2& currentPos);
    void handleCircleDrawing(ImDrawList* drawList, const glm::dvec2& currentPos);
    void handleTriangleDrawing(ImDrawList* drawList, const glm::dvec2& currentPos);
    void handleSquareDrawing(ImDrawList* drawList, const glm::dvec2& currentPos);
    void handleRectangleDrawing(ImDrawList* drawList, const glm::dvec2& currentPos);
    void handleSplineDrawing(ImDrawList* drawList, const glm::dvec2& currentPos);
    void handleBezierDrawing(ImDrawList* drawList, const glm::dvec2& currentPos);
    void handleBellowsDrawing(ImDrawList* drawList, const glm::dvec2& currentPos);
    void handleBallBearingDrawing(ImDrawList* drawList, const glm::dvec2& currentPos);
    void handleSpring2DDrawing(ImDrawList* drawList, const glm::dvec2& currentPos);
    
    // Preview methods
    void previewPoint(ImDrawList* drawList, const glm::dvec2& pos) const;
    void previewLine(ImDrawList* drawList, const glm::dvec2& start, const glm::dvec2& end) const;
    void previewCircle(ImDrawList* drawList, const glm::dvec2& center, float radius) const;
    void previewTriangle(ImDrawList* drawList, const std::array<glm::dvec2, 3>& points, int count) const;
    void previewSquare(ImDrawList* drawList, const glm::dvec2& start, const glm::dvec2& end) const;
    void previewRectangle(ImDrawList* drawList, const glm::dvec2& start, const glm::dvec2& end) const;
    void previewSpline(ImDrawList* drawList, const std::vector<glm::dvec2>& points) const;
    void previewBezier(ImDrawList* drawList, const std::vector<glm::dvec2>& points) const;
    void previewBellows(ImDrawList* drawList, const glm::dvec2& start, const glm::dvec2& end) const;
    void previewBallBearing(ImDrawList* drawList, const glm::dvec2& center, float radius) const;
    void previewSpring2D(ImDrawList* drawList, const glm::dvec2& center) const;
    void drawDashedLine(ImDrawList* drawList, const glm::dvec2& p1, const glm::dvec2& p2, 
                       ImU32 color, float thickness, float dash_length) const;

    // Shape rendering
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
    glm::dvec2 calculateBezierPoint(const std::vector<glm::dvec2>& points, float t) const;
    
    glm::dvec2 catmullRomPoint(const glm::dvec2& p0, const glm::dvec2& p1, 
                          const glm::dvec2& p2, const glm::dvec2& p3, float t) const;

    // Update helper methods
    void updateSelectedShape(const glm::dvec2& mousePos);
    void updateDrawingPreview(const glm::dvec2& mousePos);

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