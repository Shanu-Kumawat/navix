#include <glm/glm.hpp>
#include "SceneModel.hpp"
#include <algorithm>

namespace Core {

SceneModel::SceneModel() : selectedShape(nullptr), currentHistoryIndex(0) {
    // Initial empty state will be saved by the Canvas/Controller
}

void SceneModel::addShape(std::unique_ptr<Drawing::Shape> shape) {
    shapes.push_back(std::move(shape));
}

void SceneModel::removeShape(Drawing::Shape* shape) {
    if (selectedShape == shape) {
        selectedShape = nullptr;
    }
    
    auto it = std::remove_if(shapes.begin(), shapes.end(),
                             [shape](const std::unique_ptr<Drawing::Shape>& s) {
                                 return s.get() == shape;
                             });
    if (it != shapes.end()) {
        shapes.erase(it, shapes.end());
    }
}

void SceneModel::clearAll() {
    shapes.clear();
    selectedShape = nullptr;
}

const std::vector<std::unique_ptr<Drawing::Shape>>& SceneModel::getShapes() const {
    return shapes;
}

std::vector<std::unique_ptr<Drawing::Shape>>& SceneModel::getShapesMutable() {
    return shapes;
}

void SceneModel::selectShape(Drawing::Shape* shape) {
    selectedShape = shape;
}

void SceneModel::clearSelection() {
    selectedShape = nullptr;
}

Drawing::Shape* SceneModel::getSelectedShape() const {
    return selectedShape;
}

void SceneModel::saveToHistory(const glm::dvec2& panOffset, float zoomLevel, bool showGrid) {
    // If we're not at the end of history, truncate the future
    if (currentHistoryIndex < history.size() - 1) {
        history.erase(history.begin() + currentHistoryIndex + 1, history.end());
    }

    HistoryState newState;
    newState.panOffset = panOffset;
    newState.zoomLevel = zoomLevel;
    newState.showGrid = showGrid;

    // Deep copy shapes
    for (const auto& shape : shapes) {
        newState.shapes.push_back(shape->clone());
    }

    history.push_back(std::move(newState));
    currentHistoryIndex = history.size() - 1;
}

bool SceneModel::undo(glm::dvec2& outPanOffset, float& outZoomLevel, bool& outShowGrid) {
    if (!canUndo()) return false;

    currentHistoryIndex--;
    const auto& state = history[currentHistoryIndex];

    // Restore shapes
    shapes.clear();
    for (const auto& shape : state.shapes) {
        shapes.push_back(shape->clone());
    }
    selectedShape = nullptr;

    // Restore view state
    outPanOffset = state.panOffset;
    outZoomLevel = state.zoomLevel;
    outShowGrid = state.showGrid;

    return true;
}

bool SceneModel::redo(glm::dvec2& outPanOffset, float& outZoomLevel, bool& outShowGrid) {
    if (!canRedo()) return false;

    currentHistoryIndex++;
    const auto& state = history[currentHistoryIndex];

    // Restore shapes
    shapes.clear();
    for (const auto& shape : state.shapes) {
        shapes.push_back(shape->clone());
    }
    selectedShape = nullptr;

    // Restore view state
    outPanOffset = state.panOffset;
    outZoomLevel = state.zoomLevel;
    outShowGrid = state.showGrid;

    return true;
}

bool SceneModel::canUndo() const {
    return currentHistoryIndex > 0;
}

bool SceneModel::canRedo() const {
    return currentHistoryIndex + 1 < history.size();
}

} // namespace Core
