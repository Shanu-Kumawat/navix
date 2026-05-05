#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>
#include "modeling3d/WorkPlane3D.hpp"
#include "shapes/BasicShapes.hpp"
#include "shapes/ComplexShapes.hpp"

namespace Modeling3D {

/**
 * @brief A 2D sketch constrained to a WorkPlane3D.
 *
 * Reuses existing Drawing::Shape types (Line, Circle, Spline, etc.)
 * projected onto a work-plane. Provides conversion to wire profiles
 * for extrude/revolve operations.
 */
class Sketch3D {
public:
    Sketch3D();
    explicit Sketch3D(const WorkPlane3D& plane);
    ~Sketch3D() = default;

    // Work plane
    void setWorkPlane(const WorkPlane3D& plane) { workPlane = plane; }
    const WorkPlane3D& getWorkPlane() const { return workPlane; }

    // Shape management (shapes are stored in 2D local coordinates)
    void addShape(std::unique_ptr<Drawing::Shape> shape);
    void removeShape(Drawing::Shape* shape);
    void clearAll();
    const std::vector<std::unique_ptr<Drawing::Shape>>& getShapes() const { return shapes; }
    std::vector<std::unique_ptr<Drawing::Shape>>& getShapesMutable() { return shapes; }

    // Selection
    Drawing::Shape* getSelectedShape() const { return selectedShape; }
    void selectShape(Drawing::Shape* shape) { selectedShape = shape; }
    void clearSelection() { selectedShape = nullptr; }

    // Convert sketch geometry to a closed wire profile (list of 3D points)
    // Used by extrude/revolve operations
    std::vector<glm::dvec3> toWireProfile() const;

    // Check if the sketch forms a closed profile
    bool isClosed() const;

    // State
    bool isActive() const { return active; }
    void setActive(bool a) { active = a; }

    // Name
    const std::string& getName() const { return name; }
    void setName(const std::string& n) { name = n; }

    // Visibility
    bool isVisible() const { return visible; }
    void setVisible(bool v) { visible = v; }

private:
    WorkPlane3D workPlane;
    std::vector<std::unique_ptr<Drawing::Shape>> shapes;
    Drawing::Shape* selectedShape{nullptr};
    bool active{false};
    bool visible{true};
    std::string name{"Sketch"};
};

} // namespace Modeling3D
