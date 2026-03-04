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
#include "commands/AddShapeCommand.hpp"
#include "commands/DeleteShapeCommand.hpp"
#include "commands/MoveShapeCommand.hpp"
#include "commands/DuplicateShapeCommand.hpp"
#include "commands/RotateShapeCommand.hpp"
#include "commands/ScaleShapeCommand.hpp"
#include "topology/TopologyManager.hpp"
#include "fem/MaterialManager.hpp"
#include "shapes/ShockAbsorberBottomEnd.hpp"
#include "utils/MathUtils.hpp"
#include "Constants.hpp"
#include "Renderer2D.hpp"
#include "core/ImGuiRenderer.hpp"
#include "meshing/GmshTranslator.hpp"
#include "meshing/Mesh.hpp"
#include "InputController.hpp"
#include "SceneModel.hpp"

namespace Drawing {
using namespace Drawing::Math;



class CanvasSnapshotCommand;

class Canvas {
public:
    friend class CanvasSnapshotCommand;
    void render(ImDrawList* drawList) {
        if (renderer) {
            // Render grid via the renderer
            if (showGrid) renderer->renderGrid(drawList);
            // Render shapes from SceneModel (the canonical source of truth)
            // Canvas::shapes is a reference to sceneModel->getShapesMutable()
            renderer->renderShapes(drawList, shapes);
        }
        // Render topology overlay (nodes/edges) via ImGuiRenderer — hidden by default
        if (showTopologyOverlay && topoRenderer && topologyManager) {
            topoRenderer->beginFrame(drawList);
            // Use (0,0) for windowPos because shapes use transformCoordinates() which
            // doesn't include window position — keep coordinate systems consistent
            topoRenderer->setTransform(panOffset, zoomLevel, glm::dvec2(0, 0));
            for (const auto& [id, edge] : topologyManager->getEdges()) {
                auto n1 = topologyManager->getNode(edge->getStartNodeId());
                auto n2 = topologyManager->getNode(edge->getEndNodeId());
                topoRenderer->drawEdge(edge.get(), n1.get(), n2.get());
            }
            for (const auto& [id, node] : topologyManager->getNodes()) {
                topoRenderer->drawNode(node.get());
            }
        }
        // Render mesh overlay if available
        if (showMesh && !currentMesh.isEmpty() && renderer) {
            renderer->beginFrame(drawList);
            // Use (0,0) for windowPos because shapes use transformCoordinates() which
            // doesn't include window position — keep coordinate systems consistent
            renderer->setTransform(panOffset, zoomLevel, glm::dvec2(0, 0));
            renderer->drawMesh(currentMesh, glm::dvec3(0.0, 0.5, 1.0));
        }
    }
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
    bool getShowControlPoints() const { return showControlPoints; }
    Shape* getSelectedShape() const { return selectedShape; } // Synced with SceneModel
    DrawingMode getCurrentMode() const { return currentMode; }

    // Drawing state — forwarded to InputController (canonical owner)
    bool getIsDrawing() const;
    glm::dvec2 getStartPoint() const;
    int getClickCount() const;
    const std::array<glm::dvec2, 3>& getTrianglePoints() const;
    const std::vector<glm::dvec2>& getCurrentSplinePoints() const;
    const std::vector<glm::dvec2>& getCurrentCurvePoints() const;
    
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

    // Selection methods — synced with SceneModel
    void selectShape(Shape* shape) { selectedShape = shape; if (sceneModel) sceneModel->selectShape(shape); }
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

    // Shape creation (used by InputController)
    void addShapeWithCommand(std::unique_ptr<Shape> shape);
    void createTopologyForShape(Shape* shape);
    void removeTopologyForShape(Shape* shape);
    void updateTopologyPositions(Shape* shape);

    // Snapping (used by InputController)
    glm::dvec2 getSnappedPoint(const glm::dvec2& point) const;
    glm::dvec2 findNearestSnapPoint(const glm::dvec2& pos) const;

    void setWindowInfo(float x, float y, float width, float height) {
        windowX = x;
        windowY = y;
        windowWidth = width;
        windowHeight = height;
    }

    void clearAll();

    // Meshing
    bool generateMesh(double elementSize = 10.0);
    void clearMesh();
    bool hasMesh() const;
    const Core::Meshing::Mesh& getMesh() const { return currentMesh; }
    void setMeshElementSize(double size) { meshElementSize = size; }
    double getMeshElementSize() const { return meshElementSize; }
    bool isMeshVisible() const { return showMesh; }
    void setMeshVisible(bool visible) { showMesh = visible; }

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
    
    // Sub-system access
    Core::Topology::TopologyManager* getTopologyManager() const { return topologyManager.get(); }
    Core::FEM::MaterialManager* getMaterialManager() const { return materialManager.get(); }

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

    void addShape(std::unique_ptr<Shape> shape) { shapes.push_back(std::move(shape)); } // Pushes to SceneModel via reference

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
    // SceneModel must be declared first since shapes is a reference into it
    std::unique_ptr<Core::SceneModel> sceneModel;
    std::unique_ptr<Core::Renderer2D> renderer;
    std::unique_ptr<Core::Graphics::ImGuiRenderer> topoRenderer; // For topology overlay
    std::unique_ptr<Core::InputController> inputController;

    // Drawing state (currentMode stays here; drawing interaction state is in InputController)
    DrawingMode currentMode{DrawingMode::None};
    bool snapToGrid{true};
    
    // Transform state
    float zoomLevel{1.0f};
    float scaleFactor{1.0f};
    glm::dvec2 panOffset{0.0f, 0.0f};
    glm::dvec2 zoomCenter{0.0f, 0.0f};
    
    // Grid & snapping
    float gridSpacing{Constants::DEFAULT_GRID_SPACING};
    bool showGrid{true};
    
    // Shape collection — reference to SceneModel's canonical shapes vector
    std::vector<std::unique_ptr<Shape>>& shapes;
    
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
    bool showTopologyOverlay{false};
    Shape* selectedShape{nullptr};

    // Drawing data
    int selectedShapeIndex = -1;
    
    // History management
    Core::Commands::CommandManager commandManager;
    std::unique_ptr<Core::Topology::TopologyManager> topologyManager;
    std::unique_ptr<Core::FEM::MaterialManager> materialManager;

    // Shape ↔ Topology mapping — tracks which topology IDs belong to each shape
    struct ShapeTopology {
        std::vector<uint64_t> nodeIds;
        std::vector<uint64_t> edgeIds;
        std::vector<uint64_t> faceIds;
    };
    std::unordered_map<Drawing::Shape*, ShapeTopology> shapeTopoMap;

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
    bool trySnapToExistingPoint(glm::dvec2& point) const;
    void renderPreview(ImDrawList* drawList, const glm::dvec2& currentPos) const;
    std::optional<glm::dvec2> findNearestPoint(const glm::dvec2& point, float threshold) const;
    

    
    // Drawing handlers — moved to InputController
    
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

    // Meshing state
    Core::Meshing::GmshTranslator gmshTranslator;
    Core::Meshing::Mesh currentMesh;
    double meshElementSize{10.0};
    bool showMesh{true};

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