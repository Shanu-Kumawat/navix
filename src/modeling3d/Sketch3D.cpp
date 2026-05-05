#include "modeling3d/Sketch3D.hpp"
#include <algorithm>

namespace Modeling3D {

Sketch3D::Sketch3D()
    : workPlane(WorkPlane3D::XY())
{
}

Sketch3D::Sketch3D(const WorkPlane3D& plane)
    : workPlane(plane)
{
}

void Sketch3D::addShape(std::unique_ptr<Drawing::Shape> shape) {
    shapes.push_back(std::move(shape));
}

void Sketch3D::removeShape(Drawing::Shape* shape) {
    auto it = std::find_if(shapes.begin(), shapes.end(),
        [shape](const std::unique_ptr<Drawing::Shape>& s) { return s.get() == shape; });
    if (it != shapes.end()) {
        if (selectedShape == shape) selectedShape = nullptr;
        shapes.erase(it);
    }
}

void Sketch3D::clearAll() {
    selectedShape = nullptr;
    shapes.clear();
}

std::vector<glm::dvec3> Sketch3D::toWireProfile() const {
    std::vector<glm::dvec3> profile;

    // Collect all endpoints from line shapes to form a wire
    for (const auto& shape : shapes) {
        switch (shape->type) {
            case Drawing::ShapeType::LINE: {
                auto* line = static_cast<const Drawing::Line*>(shape.get());
                if (profile.empty()) {
                    profile.push_back(workPlane.to3D(line->start));
                }
                profile.push_back(workPlane.to3D(line->end));
                break;
            }
            case Drawing::ShapeType::CIRCLE: {
                auto* circle = static_cast<const Drawing::Circle*>(shape.get());
                // Generate circle points
                const int segments = 64;
                for (int i = 0; i <= segments; ++i) {
                    double angle = (2.0 * M_PI * i) / segments;
                    glm::dvec2 p2d(
                        circle->center.x + circle->radius * std::cos(angle),
                        circle->center.y + circle->radius * std::sin(angle)
                    );
                    profile.push_back(workPlane.to3D(p2d));
                }
                break;
            }
            case Drawing::ShapeType::RECTANGLE: {
                auto* rect = static_cast<const Drawing::Rectangle*>(shape.get());
                profile.push_back(workPlane.to3D(rect->topLeft));
                profile.push_back(workPlane.to3D(rect->topRight));
                profile.push_back(workPlane.to3D(rect->bottomRight));
                profile.push_back(workPlane.to3D(rect->bottomLeft));
                profile.push_back(workPlane.to3D(rect->topLeft)); // Close
                break;
            }
            case Drawing::ShapeType::SQUARE: {
                auto* sq = static_cast<const Drawing::Square*>(shape.get());
                auto tl = sq->getTopLeft();
                float size = sq->getSize();
                profile.push_back(workPlane.to3D(tl));
                profile.push_back(workPlane.to3D(glm::dvec2(tl.x + size, tl.y)));
                profile.push_back(workPlane.to3D(glm::dvec2(tl.x + size, tl.y + size)));
                profile.push_back(workPlane.to3D(glm::dvec2(tl.x, tl.y + size)));
                profile.push_back(workPlane.to3D(tl)); // Close
                break;
            }
            case Drawing::ShapeType::SPLINE: {
                auto* spline = static_cast<const Drawing::Spline*>(shape.get());
                auto points = spline->calculatePoints(0.02f);
                for (const auto& p : points) {
                    profile.push_back(workPlane.to3D(p));
                }
                break;
            }
            case Drawing::ShapeType::BEZIER: {
                auto* bezier = static_cast<const Drawing::BezierCurve*>(shape.get());
                auto points = bezier->calculatePoints(0.02f);
                for (const auto& p : points) {
                    profile.push_back(workPlane.to3D(p));
                }
                break;
            }
            default:
                break;
        }
    }

    return profile;
}

bool Sketch3D::isClosed() const {
    auto profile = toWireProfile();
    if (profile.size() < 3) return false;
    double dist = glm::length(profile.front() - profile.back());
    return dist < 1e-6;
}

} // namespace Modeling3D
