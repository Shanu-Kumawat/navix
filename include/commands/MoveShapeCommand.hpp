#pragma once
#include "Command.hpp"
#include "shapes/BasicShapes.hpp"
#include "shapes/ComplexShapes.hpp"
#include <glm/glm.hpp>
#include <memory>

namespace Core {
namespace Commands {

/**
 * @brief Command that moves a shape by a delta offset.
 * 
 * On execute: applies +delta to the shape's position.
 * On undo: applies -delta to restore the original position.
 * 
 * Handles all shape types that currently support movement
 * (mirrors the logic in Canvas::moveSelectedShape).
 */
class MoveShapeCommand : public Command {
public:
    MoveShapeCommand(Drawing::Shape* targetShape, const glm::dvec2& moveDelta)
        : shape(targetShape), delta(moveDelta) {}

    void execute() override {
        applyDelta(delta);
    }

    void undo() override {
        applyDelta(-delta);
    }

    std::string getName() const override { return "Move Shape"; }

private:
    void applyDelta(const glm::dvec2& d) {
        if (!shape) return;
        
        switch (shape->type) {
            case Drawing::ShapeType::POINT: {
                auto& pt = static_cast<Drawing::Point&>(*shape);
                pt.position += d;
                break;
            }
            case Drawing::ShapeType::LINE: {
                auto& line = static_cast<Drawing::Line&>(*shape);
                line.start += d;
                line.end += d;
                break;
            }
            case Drawing::ShapeType::CIRCLE: {
                auto& circle = static_cast<Drawing::Circle&>(*shape);
                circle.center += d;
                break;
            }
            case Drawing::ShapeType::TRIANGLE: {
                auto& tri = static_cast<Drawing::Triangle&>(*shape);
                for (auto& pt : tri.points) { pt += d; }
                break;
            }
            case Drawing::ShapeType::SQUARE: {
                auto& sq = static_cast<Drawing::Square&>(*shape);
                sq.start += d;
                sq.end += d;
                break;
            }
            case Drawing::ShapeType::RECTANGLE: {
                auto& rect = static_cast<Drawing::Rectangle&>(*shape);
                rect.topLeft += d;
                rect.topRight += d;
                rect.bottomRight += d;
                rect.bottomLeft += d;
                break;
            }
            case Drawing::ShapeType::SPLINE: {
                auto& spline = static_cast<Drawing::Spline&>(*shape);
                spline.moveEntireSpline(d);
                break;
            }
            case Drawing::ShapeType::BEZIER: {
                auto& bezier = static_cast<Drawing::BezierCurve&>(*shape);
                bezier.moveEntireCurve(d);
                break;
            }
            case Drawing::ShapeType::BALL_BEARING: {
                auto& bb = static_cast<Drawing::BallBearing&>(*shape);
                bb.position.x += d.x;
                bb.position.y += d.y;
                break;
            }
            case Drawing::ShapeType::SPRING2D: {
                auto& spring = static_cast<Drawing::Spring2D&>(*shape);
                spring.centerX += static_cast<float>(d.x);
                spring.centerY += static_cast<float>(d.y);
                break;
            }
            default:
                // Bellows, Dimension, ShockAbsorberEnd etc. — not yet movable
                break;
        }
    }

    Drawing::Shape* shape;
    glm::dvec2 delta;
};

} // namespace Commands
} // namespace Core
