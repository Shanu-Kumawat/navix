#pragma once
#include "Command.hpp"
#include "shapes/BasicShapes.hpp"
#include <memory>
#include <vector>

namespace Core {
namespace Commands {

/**
 * @brief Command that deletes a shape from the scene.
 * 
 * On execute: removes the shape at the stored index.
 * On undo: re-inserts the shape at the stored index.
 */
class DeleteShapeCommand : public Command {
public:
    /**
     * @brief Construct before the shape is actually removed.
     * @param shapesRef Reference to the canonical shapes vector.
     * @param index     The index of the shape to delete.
     */
    DeleteShapeCommand(std::vector<std::unique_ptr<Drawing::Shape>>& shapesRef, size_t index)
        : shapes(shapesRef), deleteIndex(index) {}

    void execute() override {
        if (deleteIndex < shapes.size()) {
            removedShape = std::move(shapes[deleteIndex]);
            shapes.erase(shapes.begin() + deleteIndex);
        }
    }

    void undo() override {
        if (removedShape && deleteIndex <= shapes.size()) {
            shapes.insert(shapes.begin() + deleteIndex, std::move(removedShape));
            removedShape = nullptr;
        }
    }

    std::string getName() const override { return "Delete Shape"; }

private:
    std::vector<std::unique_ptr<Drawing::Shape>>& shapes;
    size_t deleteIndex;
    std::unique_ptr<Drawing::Shape> removedShape;
};

} // namespace Commands
} // namespace Core
