#pragma once
#include "Command.hpp"
#include "shapes/BasicShapes.hpp"
#include <memory>
#include <vector>

namespace Core {
namespace Commands {

/**
 * @brief Command that duplicates a shape by cloning it into the shapes vector.
 *
 * On execute: pushes the cloned shape into the vector and selects it.
 * On undo:    removes the last-added clone from the vector.
 */
class DuplicateShapeCommand : public Command {
public:
    DuplicateShapeCommand(std::vector<std::unique_ptr<Drawing::Shape>>& shapesVec,
                          std::unique_ptr<Drawing::Shape> clonedShape)
        : shapes(shapesVec), clone(std::move(clonedShape)) {}

    void execute() override {
        if (clone) {
            shapes.push_back(std::move(clone));
            insertedIndex = shapes.size() - 1;
        }
    }

    void undo() override {
        if (insertedIndex < shapes.size()) {
            clone = std::move(shapes[insertedIndex]);
            shapes.erase(shapes.begin() + static_cast<std::ptrdiff_t>(insertedIndex));
        }
    }

    std::string getName() const override { return "Duplicate Shape"; }

    /// After execute(), returns pointer to the newly inserted shape (for selection).
    Drawing::Shape* getInsertedShape() const {
        if (insertedIndex < shapes.size()) return shapes[insertedIndex].get();
        return nullptr;
    }

private:
    std::vector<std::unique_ptr<Drawing::Shape>>& shapes;
    std::unique_ptr<Drawing::Shape> clone;
    size_t insertedIndex{0};
};

} // namespace Commands
} // namespace Core
