#pragma once

#include <vector>
#include <memory>
#include <imgui.h>
#include "shapes/BasicShapes.hpp"
#include "shapes/ComplexShapes.hpp"

namespace Core {

/**
 * @brief Represents the state of the drawing canvas to support Undo/Redo.
 */
struct HistoryState {
    std::vector<std::unique_ptr<Drawing::Shape>> shapes;
    ImVec2 panOffset;
    float zoomLevel;
    bool showGrid;
};

/**
 * @brief The Model in the MVC architecture for the Canvas.
 * 
 * SceneModel is responsible for managing the collection of geometric shapes,
 * the current selection state, and the history (Undo/Redo) of the scene.
 * It is completely independent of rendering and input handling.
 */
class SceneModel {
public:
    SceneModel();
    ~SceneModel() = default;

    // Shape management
    void addShape(std::unique_ptr<Drawing::Shape> shape);
    void removeShape(Drawing::Shape* shape);
    void clearAll();
    const std::vector<std::unique_ptr<Drawing::Shape>>& getShapes() const;
    std::vector<std::unique_ptr<Drawing::Shape>>& getShapesMutable();

    // Selection management
    void selectShape(Drawing::Shape* shape);
    void clearSelection();
    Drawing::Shape* getSelectedShape() const;

    // History management
    void saveToHistory(const ImVec2& panOffset, float zoomLevel, bool showGrid);
    bool undo(ImVec2& outPanOffset, float& outZoomLevel, bool& outShowGrid);
    bool redo(ImVec2& outPanOffset, float& outZoomLevel, bool& outShowGrid);
    bool canUndo() const;
    bool canRedo() const;

    // Shape finding utilities
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

private:
    std::vector<std::unique_ptr<Drawing::Shape>> shapes;
    Drawing::Shape* selectedShape;

    std::vector<HistoryState> history;
    size_t currentHistoryIndex;
};

} // namespace Core
