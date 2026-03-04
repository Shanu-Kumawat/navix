#pragma once
#include "Command.hpp"
#include "shapes/BasicShapes.hpp"
#include <memory>
#include <vector>

namespace Core {
namespace Commands {

/**
 * @brief Command that adds a shape to the scene.
 * 
 * On execute: inserts the shape at the stored index.
 * On undo: removes the shape from that index and stores it.
 */
class AddShapeCommand : public Command {
public:
    /**
     * @brief Construct after the shape has already been added to the vector.
     * @param shapesRef Reference to the canonical shapes vector (SceneModel's).
     * @param index     The index where the shape was inserted (usually back).
     */
    AddShapeCommand(std::vector<std::unique_ptr<Drawing::Shape>>& shapesRef, size_t index)
        : shapes(shapesRef), insertIndex(index) {}

    void execute() override {
        // Re-insert the shape we previously removed during undo
        if (removedShape) {
            if (insertIndex <= shapes.size()) {
                shapes.insert(shapes.begin() + insertIndex, std::move(removedShape));
                removedShape = nullptr;
            }
        }
    }

    void undo() override {
        if (insertIndex < shapes.size()) {
            removedShape = std::move(shapes[insertIndex]);
            shapes.erase(shapes.begin() + insertIndex);
        }
    }

    std::string getName() const override { return "Add Shape"; }

private:
    std::vector<std::unique_ptr<Drawing::Shape>>& shapes;
    size_t insertIndex;
    std::unique_ptr<Drawing::Shape> removedShape; // held during undo state
};

} // namespace Commands
} // namespace Core
