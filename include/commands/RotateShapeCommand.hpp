#pragma once
#include "Command.hpp"
#include "shapes/BasicShapes.hpp"
#include "shapes/ComplexShapes.hpp"
#include <glm/glm.hpp>
#include <cmath>

namespace Core {
namespace Commands {

/**
 * @brief Command that rotates a shape around a given center by a specified angle.
 *
 * On execute: rotates all geometry points by +angle around center.
 * On undo:    rotates all geometry points by -angle around center.
 */
class RotateShapeCommand : public Command {
public:
    RotateShapeCommand(Drawing::Shape* targetShape, float angleDegrees, const glm::dvec2& rotationCenter)
        : shape(targetShape), angle(angleDegrees), center(rotationCenter) {}

    void execute() override {
        applyRotation(angle);
    }

    void undo() override {
        applyRotation(-angle);
    }

    std::string getName() const override { return "Rotate Shape"; }

private:
    static glm::dvec2 rotatePointAroundCenter(const glm::dvec2& point, const glm::dvec2& c, float degrees) {
        float rad = degrees * static_cast<float>(M_PI) / 180.0f;
        float cosA = std::cos(rad);
        float sinA = std::sin(rad);
        double dx = point.x - c.x;
        double dy = point.y - c.y;
        return glm::dvec2(c.x + dx * cosA - dy * sinA,
                          c.y + dx * sinA + dy * cosA);
    }

    void applyRotation(float deg) {
        if (!shape) return;

        switch (shape->type) {
            case Drawing::ShapeType::POINT: {
                auto& pt = static_cast<Drawing::Point&>(*shape);
                pt.position = rotatePointAroundCenter(pt.position, center, deg);
                break;
            }
            case Drawing::ShapeType::LINE: {
                auto& line = static_cast<Drawing::Line&>(*shape);
                line.start = rotatePointAroundCenter(line.start, center, deg);
                line.end   = rotatePointAroundCenter(line.end, center, deg);
                break;
            }
            case Drawing::ShapeType::CIRCLE: {
                auto& circle = static_cast<Drawing::Circle&>(*shape);
                circle.center = rotatePointAroundCenter(circle.center, center, deg);
                break;
            }
            case Drawing::ShapeType::TRIANGLE: {
                auto& tri = static_cast<Drawing::Triangle&>(*shape);
                for (auto& pt : tri.points) pt = rotatePointAroundCenter(pt, center, deg);
                break;
            }
            case Drawing::ShapeType::SQUARE: {
                auto& sq = static_cast<Drawing::Square&>(*shape);
                sq.start = rotatePointAroundCenter(sq.start, center, deg);
                sq.end   = rotatePointAroundCenter(sq.end, center, deg);
                break;
            }
            case Drawing::ShapeType::RECTANGLE: {
                auto& rect = static_cast<Drawing::Rectangle&>(*shape);
                rect.topLeft     = rotatePointAroundCenter(rect.topLeft, center, deg);
                rect.topRight    = rotatePointAroundCenter(rect.topRight, center, deg);
                rect.bottomRight = rotatePointAroundCenter(rect.bottomRight, center, deg);
                rect.bottomLeft  = rotatePointAroundCenter(rect.bottomLeft, center, deg);
                break;
            }
            case Drawing::ShapeType::SPLINE: {
                auto& spline = static_cast<Drawing::Spline&>(*shape);
                for (auto& cp : spline.controlPoints) cp = rotatePointAroundCenter(cp, center, deg);
                break;
            }
            case Drawing::ShapeType::BEZIER: {
                auto& bezier = static_cast<Drawing::BezierCurve&>(*shape);
                for (auto& cp : bezier.controlPoints) cp = rotatePointAroundCenter(cp, center, deg);
                break;
            }
            case Drawing::ShapeType::DIMENSION: {
                auto& dim = static_cast<Drawing::Dimension&>(*shape);
                dim.start        = rotatePointAroundCenter(dim.start, center, deg);
                dim.end          = rotatePointAroundCenter(dim.end, center, deg);
                dim.textPosition = rotatePointAroundCenter(dim.textPosition, center, deg);
                break;
            }
            case Drawing::ShapeType::BALL_BEARING: {
                auto& bb = static_cast<Drawing::BallBearing&>(*shape);
                bb.position = rotatePointAroundCenter(bb.position, center, deg);
                bb.angle += deg;
                break;
            }
            case Drawing::ShapeType::SPRING2D: {
                auto& spring = static_cast<Drawing::Spring2D&>(*shape);
                glm::dvec2 pos(spring.centerX, spring.centerY);
                pos = rotatePointAroundCenter(pos, center, deg);
                spring.centerX = static_cast<float>(pos.x);
                spring.centerY = static_cast<float>(pos.y);
                break;
            }
            case Drawing::ShapeType::BELLOWS: {
                auto& bellows = static_cast<Drawing::Bellows&>(*shape);
                bellows.position = rotatePointAroundCenter(bellows.position, center, deg);
                bellows.angle += deg;
                bellows.invalidateCache();
                break;
            }
            default:
                break;
        }
    }

    Drawing::Shape* shape;
    float angle;
    glm::dvec2 center;
};

} // namespace Commands
} // namespace Core
