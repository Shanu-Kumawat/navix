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
using glm::normalize;
using glm::dot;
using Math::rotatePoint;

// ... existing code ...

Canvas::Canvas() : 
    sceneModel(std::make_unique<Core::SceneModel>()),
    renderer(std::make_unique<Core::Renderer2D>(this)),
    topoRenderer(std::make_unique<Core::Graphics::ImGuiRenderer>()),
    inputController(std::make_unique<Core::InputController>(this, sceneModel.get(), renderer.get(), nullptr)),
    currentMode(DrawingMode::None),
    snapToGrid(true),
    zoomLevel(1.0f),
    panOffset(0.0f, 0.0f),
    gridSpacing(Constants::DEFAULT_GRID_SPACING),
    showControlPoints(true),
    selectedShape(nullptr),
    topologyManager(std::make_unique<Core::Topology::TopologyManager>()),
    materialManager(std::make_unique<Core::FEM::MaterialManager>()),
    showGrid(true),
    shapes(sceneModel->getShapesMutable())
{
    // SceneModel is now the canonical owner of shapes.
    // Canvas::shapes is a reference to sceneModel->getShapesMutable().
}

// Forwarding getters — drawing state lives in InputController
bool Canvas::getIsDrawing() const { return inputController->getIsDrawing(); }
glm::dvec2 Canvas::getStartPoint() const { return inputController->getStartPoint(); }
int Canvas::getClickCount() const { return inputController->getClickCount(); }
const std::array<glm::dvec2, 3>& Canvas::getTrianglePoints() const { return inputController->getTrianglePoints(); }
const std::vector<glm::dvec2>& Canvas::getCurrentSplinePoints() const { return inputController->getCurrentSplinePoints(); }
const std::vector<glm::dvec2>& Canvas::getCurrentCurvePoints() const { return inputController->getCurrentCurvePoints(); }

void Canvas::setDrawingMode(DrawingMode mode) {
    currentMode = mode;
    inputController->setDrawingMode(mode);
}

void Canvas::clearAll() {
    saveToHistory();
    shapes.clear();
    selectedShape = nullptr;
    sceneModel->clearSelection();
    inputController->resetDrawingState();
}

void Canvas::handleInput() {
    inputController->handleInput();
}

void Canvas::handleSelection(const glm::dvec2& mousePos) {
    inputController->handleSelection(mousePos);
}


void Canvas::handleDrawing(const glm::dvec2& mousePos) {
    inputController->handleDrawing(mousePos);
}

void Canvas::update(const glm::dvec2& mousePos) {
    inputController->handleKeyboardShortcuts();
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
        size_t idx = static_cast<size_t>(std::distance(shapes.begin(), it));
        // Remove associated topology before deleting the shape
        removeTopologyForShape(selectedShape);
        // Remove from mesh exclusion set
        meshExcludedShapes.erase(selectedShape);
        clearSelection();
        // Use granular DeleteShapeCommand — executeCommand() calls execute() which removes the shape
        commandManager.executeCommand(std::make_unique<Core::Commands::DeleteShapeCommand>(shapes, idx));
        std::cout << "Shape deleted, remaining shapes: " << shapes.size() << "\n";
    }
}

void Canvas::duplicateSelectedShape() {
    if (!selectedShape) return;
    
    // Clone via the virtual clone() method — works for all shape types
    auto cloned = selectedShape->clone();
    
    // Offset the clone slightly so it's visible
    glm::dvec2 offset(20.0, 20.0);
    // Apply offset using a temporary MoveShapeCommand logic inline
    Core::Commands::MoveShapeCommand tempMove(cloned.get(), offset);
    tempMove.execute();
    
    // Execute via DuplicateShapeCommand for undo/redo
    auto cmd = std::make_unique<Core::Commands::DuplicateShapeCommand>(shapes, std::move(cloned));
    commandManager.executeCommand(std::move(cmd));
    
    // Select the newly duplicated shape
    if (!shapes.empty()) {
        selectedShape = shapes.back().get();
        sceneModel->selectShape(selectedShape);
        createTopologyForShape(selectedShape);
    }
}

void Canvas::moveSelectedShape(const glm::dvec2& delta) {
    if (!selectedShape) return;
    
    // Use granular MoveShapeCommand — executeCommand() calls execute() which applies the delta
    commandManager.executeCommand(std::make_unique<Core::Commands::MoveShapeCommand>(selectedShape, delta));
    // Sync topology node positions after the move
    updateTopologyPositions(selectedShape);
}

void Canvas::rotateSelectedShape(float angle) {
    if (!selectedShape) return;
    
    // Compute shape center from its bounding box
    glm::dvec2 bmin, bmax;
    selectedShape->getBounds(bmin, bmax);
    glm::dvec2 center = (bmin + bmax) * 0.5;
    
    commandManager.executeCommand(
        std::make_unique<Core::Commands::RotateShapeCommand>(selectedShape, angle, center));
    // Sync topology node positions after the rotation
    updateTopologyPositions(selectedShape);
}

void Canvas::scaleSelectedShape(float factor) {
    if (!selectedShape) return;
    
    // Compute shape center from its bounding box
    glm::dvec2 bmin, bmax;
    selectedShape->getBounds(bmin, bmax);
    glm::dvec2 center = (bmin + bmax) * 0.5;
    
    commandManager.executeCommand(
        std::make_unique<Core::Commands::ScaleShapeCommand>(selectedShape, factor, center));
    // Sync topology node positions after the scale
    updateTopologyPositions(selectedShape);
}


class CanvasSnapshotCommand : public Core::Commands::Command {
public:
    CanvasSnapshotCommand(Canvas* activeCanvas, Canvas::HistoryState state)
        : canvas(activeCanvas), savedState(std::move(state)) {}

    void execute() override {
        swapState();
    }

    void undo() override {
        swapState();
    }

    std::string getName() const override {
        return "Canvas Action";
    }

private:
    void swapState() {
        Canvas::HistoryState currentState;
        for (const auto& shape : canvas->shapes) {
            currentState.shapes.push_back(shape->clone());
        }
        currentState.panOffset = canvas->panOffset;
        currentState.zoomLevel = canvas->zoomLevel;
        currentState.showGrid = canvas->showGrid;

        canvas->restoreHistoryState(savedState);
        savedState = std::move(currentState);
    }

    Canvas* canvas;
    Canvas::HistoryState savedState;
};

void Canvas::saveToHistory() {
    Canvas::HistoryState state;
    for (const auto& shape : shapes) {
        state.shapes.push_back(shape->clone());
    }
    state.panOffset = panOffset;
    state.zoomLevel = zoomLevel;
    state.showGrid = showGrid;
    
    commandManager.executeCommand(std::make_unique<CanvasSnapshotCommand>(this, std::move(state)));
}

void Canvas::undo() {
    commandManager.undo();
}

void Canvas::redo() {
    commandManager.redo();
}

void Canvas::restoreHistoryState(const HistoryState& state) {
    shapes.clear();
    for (const auto& shape : state.shapes) {
        shapes.push_back(shape->clone());
    }
    selectedShape = nullptr;
    sceneModel->clearSelection();
    
    panOffset = state.panOffset;
    zoomLevel = state.zoomLevel;
    showGrid = state.showGrid;
}

void Canvas::addShapeWithCommand(std::unique_ptr<Shape> shape) {
    shapes.push_back(std::move(shape));
    size_t idx = shapes.size() - 1;
    // Populate topology for the new shape
    createTopologyForShape(shapes[idx].get());
    // Register as a granular command for undo/redo (shape already added, so use addCommand)
    commandManager.addCommand(std::make_unique<Core::Commands::AddShapeCommand>(shapes, idx));
}

// ============================================================================
// Meshing
// ============================================================================

bool Canvas::generateMesh(double elementSize) {
    if (!topologyManager) {
        std::cerr << "[Canvas] No topology manager — cannot mesh." << std::endl;
        return false;
    }

    meshElementSize = elementSize;
    currentMesh.clear();

    // Temporarily remove topology for excluded shapes
    struct SavedTopo { Shape* shape; ShapeTopology topo; };
    std::vector<SavedTopo> saved;
    for (auto* shape : meshExcludedShapes) {
        auto it = shapeTopoMap.find(shape);
        if (it != shapeTopoMap.end()) {
            saved.push_back({shape, it->second});
            // Remove from TopologyManager (and shapeTopoMap)
            removeTopologyForShape(shape);
        }
    }

    if (topologyManager->getNodes().empty()) {
        // Restore excluded shapes' topology before returning
        for (auto& s : saved) createTopologyForShape(s.shape);
        std::cerr << "[Canvas] No meshable topology nodes after exclusions." << std::endl;
        return false;
    }

    if (!gmshTranslator.initialize()) {
        for (auto& s : saved) createTopologyForShape(s.shape);
        std::cerr << "[Canvas] Failed to initialize Gmsh." << std::endl;
        return false;
    }

    if (!gmshTranslator.translateTopologyToGmsh(*topologyManager)) {
        std::cerr << "[Canvas] Failed to translate topology to Gmsh." << std::endl;
        gmshTranslator.finalize();
        for (auto& s : saved) createTopologyForShape(s.shape);
        return false;
    }

    gmshTranslator.generateMesh(2, elementSize);

    bool ok = gmshTranslator.extractGeneratedMesh(currentMesh, 2);
    gmshTranslator.finalize();

    // Restore excluded shapes' topology
    for (auto& s : saved) createTopologyForShape(s.shape);

    if (ok) {
        showMesh = true;
        std::cout << "[Canvas] Mesh generated: " << currentMesh.getNodes().size()
                  << " nodes, " << currentMesh.getElements().size() << " elements." << std::endl;
    } else {
        std::cerr << "[Canvas] Mesh extraction failed." << std::endl;
    }
    return ok;
}

void Canvas::clearMesh() {
    currentMesh.clear();
    meshExcludedShapes.clear();
    showMesh = false;
    std::cout << "[Canvas] All meshes cleared." << std::endl;
}

void Canvas::clearMeshForShape(Shape* shape) {
    if (!shape) return;
    meshExcludedShapes.insert(shape);
    // Regenerate mesh without the excluded shape
    if (!currentMesh.isEmpty()) {
        generateMesh(meshElementSize);
    }
    std::cout << "[Canvas] Mesh cleared for shape, excluded from future generation." << std::endl;
}

void Canvas::generateMeshIncludingShape(Shape* shape, double elementSize) {
    if (!shape) return;
    meshExcludedShapes.erase(shape);
    generateMesh(elementSize);
}

bool Canvas::hasMesh() const {
    return !currentMesh.isEmpty();
}

void Canvas::createTopologyForShape(Shape* shape) {
    if (!topologyManager || !shape) return;

    ShapeTopology topo;

    switch (shape->type) {
        case ShapeType::POINT: {
            auto& pt = static_cast<Point&>(*shape);
            auto n = topologyManager->createNode(pt.position.x, pt.position.y);
            if (n) topo.nodeIds.push_back(n->getId());
            break;
        }
        case ShapeType::LINE: {
            auto& line = static_cast<Line&>(*shape);
            auto n1 = topologyManager->createNode(line.start.x, line.start.y);
            auto n2 = topologyManager->createNode(line.end.x, line.end.y);
            if (n1) topo.nodeIds.push_back(n1->getId());
            if (n2) topo.nodeIds.push_back(n2->getId());
            if (n1 && n2) {
                auto e = topologyManager->createEdge(n1->getId(), n2->getId());
                if (e) topo.edgeIds.push_back(e->getId());
            }
            break;
        }
        case ShapeType::CIRCLE: {
            auto& circle = static_cast<Circle&>(*shape);
            // Approximate circle boundary as a polygon (32 segments) for 2D meshing
            const int numSegments = 32;
            std::vector<std::shared_ptr<Core::Topology::Node>> circleNodes;
            circleNodes.reserve(numSegments);
            for (int i = 0; i < numSegments; ++i) {
                double angle = 2.0 * M_PI * i / numSegments;
                double px = circle.center.x + circle.radius * std::cos(angle);
                double py = circle.center.y + circle.radius * std::sin(angle);
                auto n = topologyManager->createNode(px, py);
                if (n) {
                    circleNodes.push_back(n);
                    topo.nodeIds.push_back(n->getId());
                }
            }
            // Create edges between consecutive nodes, closing the loop
            if (circleNodes.size() == static_cast<size_t>(numSegments)) {
                std::vector<uint64_t> edgeIds;
                for (int i = 0; i < numSegments; ++i) {
                    int next = (i + 1) % numSegments;
                    auto e = topologyManager->createEdge(circleNodes[i]->getId(), circleNodes[next]->getId());
                    if (e) {
                        topo.edgeIds.push_back(e->getId());
                        edgeIds.push_back(e->getId());
                    }
                }
                // Create a face from the closed edge loop
                if (edgeIds.size() == static_cast<size_t>(numSegments)) {
                    auto f = topologyManager->createFace(edgeIds);
                    if (f) topo.faceIds.push_back(f->getId());
                }
            }
            break;
        }
        case ShapeType::TRIANGLE: {
            auto& tri = static_cast<Triangle&>(*shape);
            auto n1 = topologyManager->createNode(tri.points[0].x, tri.points[0].y);
            auto n2 = topologyManager->createNode(tri.points[1].x, tri.points[1].y);
            auto n3 = topologyManager->createNode(tri.points[2].x, tri.points[2].y);
            if (n1) topo.nodeIds.push_back(n1->getId());
            if (n2) topo.nodeIds.push_back(n2->getId());
            if (n3) topo.nodeIds.push_back(n3->getId());
            if (n1 && n2 && n3) {
                auto e1 = topologyManager->createEdge(n1->getId(), n2->getId());
                auto e2 = topologyManager->createEdge(n2->getId(), n3->getId());
                auto e3 = topologyManager->createEdge(n3->getId(), n1->getId());
                if (e1) topo.edgeIds.push_back(e1->getId());
                if (e2) topo.edgeIds.push_back(e2->getId());
                if (e3) topo.edgeIds.push_back(e3->getId());
                if (e1 && e2 && e3) {
                    auto f = topologyManager->createFace({e1->getId(), e2->getId(), e3->getId()});
                    if (f) topo.faceIds.push_back(f->getId());
                }
            }
            break;
        }
        case ShapeType::SQUARE: {
            auto& sq = static_cast<Square&>(*shape);
            auto tl = sq.getTopLeft();
            float size = sq.getSize();
            auto n1 = topologyManager->createNode(tl.x, tl.y);
            auto n2 = topologyManager->createNode(tl.x + size, tl.y);
            auto n3 = topologyManager->createNode(tl.x + size, tl.y + size);
            auto n4 = topologyManager->createNode(tl.x, tl.y + size);
            if (n1) topo.nodeIds.push_back(n1->getId());
            if (n2) topo.nodeIds.push_back(n2->getId());
            if (n3) topo.nodeIds.push_back(n3->getId());
            if (n4) topo.nodeIds.push_back(n4->getId());
            if (n1 && n2 && n3 && n4) {
                auto e1 = topologyManager->createEdge(n1->getId(), n2->getId());
                auto e2 = topologyManager->createEdge(n2->getId(), n3->getId());
                auto e3 = topologyManager->createEdge(n3->getId(), n4->getId());
                auto e4 = topologyManager->createEdge(n4->getId(), n1->getId());
                if (e1) topo.edgeIds.push_back(e1->getId());
                if (e2) topo.edgeIds.push_back(e2->getId());
                if (e3) topo.edgeIds.push_back(e3->getId());
                if (e4) topo.edgeIds.push_back(e4->getId());
                if (e1 && e2 && e3 && e4) {
                    auto f = topologyManager->createFace({e1->getId(), e2->getId(), e3->getId(), e4->getId()});
                    if (f) topo.faceIds.push_back(f->getId());
                }
            }
            break;
        }
        case ShapeType::RECTANGLE: {
            auto& rect = static_cast<Rectangle&>(*shape);
            auto n1 = topologyManager->createNode(rect.topLeft.x, rect.topLeft.y);
            auto n2 = topologyManager->createNode(rect.topRight.x, rect.topRight.y);
            auto n3 = topologyManager->createNode(rect.bottomRight.x, rect.bottomRight.y);
            auto n4 = topologyManager->createNode(rect.bottomLeft.x, rect.bottomLeft.y);
            if (n1) topo.nodeIds.push_back(n1->getId());
            if (n2) topo.nodeIds.push_back(n2->getId());
            if (n3) topo.nodeIds.push_back(n3->getId());
            if (n4) topo.nodeIds.push_back(n4->getId());
            if (n1 && n2 && n3 && n4) {
                auto e1 = topologyManager->createEdge(n1->getId(), n2->getId());
                auto e2 = topologyManager->createEdge(n2->getId(), n3->getId());
                auto e3 = topologyManager->createEdge(n3->getId(), n4->getId());
                auto e4 = topologyManager->createEdge(n4->getId(), n1->getId());
                if (e1) topo.edgeIds.push_back(e1->getId());
                if (e2) topo.edgeIds.push_back(e2->getId());
                if (e3) topo.edgeIds.push_back(e3->getId());
                if (e4) topo.edgeIds.push_back(e4->getId());
                if (e1 && e2 && e3 && e4) {
                    auto f = topologyManager->createFace({e1->getId(), e2->getId(), e3->getId(), e4->getId()});
                    if (f) topo.faceIds.push_back(f->getId());
                }
            }
            break;
        }
        case ShapeType::SPLINE: {
            auto& spline = static_cast<Spline&>(*shape);
            // Create nodes at control points and edges between consecutive ones
            std::shared_ptr<Core::Topology::Node> prev = nullptr;
            for (const auto& cp : spline.controlPoints) {
                auto n = topologyManager->createNode(cp.x, cp.y);
                if (n) {
                    topo.nodeIds.push_back(n->getId());
                    if (prev) {
                        auto e = topologyManager->createEdge(prev->getId(), n->getId());
                        if (e) topo.edgeIds.push_back(e->getId());
                    }
                    prev = n;
                }
            }
            break;
        }
        case ShapeType::BEZIER: {
            auto& bezier = static_cast<BezierCurve&>(*shape);
            std::shared_ptr<Core::Topology::Node> prev = nullptr;
            for (const auto& cp : bezier.controlPoints) {
                auto n = topologyManager->createNode(cp.x, cp.y);
                if (n) {
                    topo.nodeIds.push_back(n->getId());
                    if (prev) {
                        auto e = topologyManager->createEdge(prev->getId(), n->getId());
                        if (e) topo.edgeIds.push_back(e->getId());
                    }
                    prev = n;
                }
            }
            break;
        }
        default:
            // Bellows, BallBearing, Spring2D, ShockAbsorber — complex topology TBD
            break;
    }

    // Record the mapping
    if (!topo.nodeIds.empty() || !topo.edgeIds.empty() || !topo.faceIds.empty()) {
        shapeTopoMap[shape] = std::move(topo);
    }
}

void Canvas::removeTopologyForShape(Shape* shape) {
    auto it = shapeTopoMap.find(shape);
    if (it == shapeTopoMap.end()) return;

    const auto& topo = it->second;
    // Remove in reverse order: faces → edges → nodes
    for (auto fid : topo.faceIds) topologyManager->removeFace(fid);
    for (auto eid : topo.edgeIds) topologyManager->removeEdge(eid);
    for (auto nid : topo.nodeIds) topologyManager->removeNode(nid);
    shapeTopoMap.erase(it);
}

void Canvas::updateTopologyPositions(Shape* shape) {
    auto it = shapeTopoMap.find(shape);
    if (it == shapeTopoMap.end()) return;

    const auto& nodeIds = it->second.nodeIds;

    // Collect the current geometry positions according to shape type
    std::vector<glm::dvec2> positions;
    switch (shape->type) {
        case ShapeType::POINT: {
            auto& pt = static_cast<Point&>(*shape);
            positions.push_back(pt.position);
            break;
        }
        case ShapeType::LINE: {
            auto& line = static_cast<Line&>(*shape);
            positions.push_back(line.start);
            positions.push_back(line.end);
            break;
        }
        case ShapeType::CIRCLE: {
            auto& circle = static_cast<Circle&>(*shape);
            positions.push_back(circle.center);
            break;
        }
        case ShapeType::TRIANGLE: {
            auto& tri = static_cast<Triangle&>(*shape);
            for (const auto& p : tri.points) positions.push_back(p);
            break;
        }
        case ShapeType::SQUARE: {
            auto& sq = static_cast<Square&>(*shape);
            auto tl = sq.getTopLeft();
            float size = sq.getSize();
            positions.push_back(tl);
            positions.push_back(glm::dvec2(tl.x + size, tl.y));
            positions.push_back(glm::dvec2(tl.x + size, tl.y + size));
            positions.push_back(glm::dvec2(tl.x, tl.y + size));
            break;
        }
        case ShapeType::RECTANGLE: {
            auto& rect = static_cast<Rectangle&>(*shape);
            positions.push_back(rect.topLeft);
            positions.push_back(rect.topRight);
            positions.push_back(rect.bottomRight);
            positions.push_back(rect.bottomLeft);
            break;
        }
        case ShapeType::SPLINE: {
            auto& spline = static_cast<Spline&>(*shape);
            positions = spline.controlPoints;
            break;
        }
        case ShapeType::BEZIER: {
            auto& bezier = static_cast<BezierCurve&>(*shape);
            positions = bezier.controlPoints;
            break;
        }
        default:
            break;
    }

    // Update node positions — match by index
    size_t count = std::min(nodeIds.size(), positions.size());
    for (size_t i = 0; i < count; ++i) {
        auto node = topologyManager->getNode(nodeIds[i]);
        if (node) {
            node->setPosition(glm::dvec3(positions[i].x, positions[i].y, 0.0));
        }
    }
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
    inputController->handleCurveManipulation(mousePos);
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
    inputController->resetDrawingState();
    selectedShape = nullptr;
    sceneModel->clearSelection();
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
        
        // Clear the main selection pointer and sync with SceneModel
        selectedShape = nullptr;
        sceneModel->clearSelection();
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




void Canvas::setSpring2DShape(std::unique_ptr<Shape> spring) {
    // Remove any existing Spring2D shape
    shapes.erase(std::remove_if(shapes.begin(), shapes.end(),
        [](const std::unique_ptr<Shape>& s) {
            return s->type == ShapeType::SPRING2D;
        }), shapes.end());
    // Add the new spring via granular command
    addShapeWithCommand(std::move(spring));
    // Select the new spring
    selectShape(shapes.back().get());
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