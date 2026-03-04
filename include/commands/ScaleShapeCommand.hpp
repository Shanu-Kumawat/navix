#pragma once
#include "Command.hpp"
#include "shapes/BasicShapes.hpp"
#include "shapes/ComplexShapes.hpp"
#include <glm/glm.hpp>

namespace Core {
namespace Commands {

/**
 * @brief Command that scales a shape by a given factor around a center point.
 *
 * On execute: scales all geometry by `factor` relative to center.
 * On undo:    scales by 1/factor to restore original geometry.
 */
class ScaleShapeCommand : public Command {
public:
    ScaleShapeCommand(Drawing::Shape* targetShape, float scaleFactor, const glm::dvec2& scaleCenter)
        : shape(targetShape), factor(scaleFactor), center(scaleCenter) {}

    void execute() override {
        applyScale(factor);
    }

    void undo() override {
        if (factor != 0.0f) applyScale(1.0f / factor);
    }

    std::string getName() const override { return "Scale Shape"; }

private:
    static glm::dvec2 scalePointAroundCenter(const glm::dvec2& point, const glm::dvec2& c, float f) {
        return c + (point - c) * static_cast<double>(f);
    }

    void applyScale(float f) {
        if (!shape) return;

        switch (shape->type) {
            case Drawing::ShapeType::POINT: {
                auto& pt = static_cast<Drawing::Point&>(*shape);
                pt.position = scalePointAroundCenter(pt.position, center, f);
                pt.size *= f;
                break;
            }
            case Drawing::ShapeType::LINE: {
                auto& line = static_cast<Drawing::Line&>(*shape);
                line.start = scalePointAroundCenter(line.start, center, f);
                line.end   = scalePointAroundCenter(line.end, center, f);
                break;
            }
            case Drawing::ShapeType::CIRCLE: {
                auto& circle = static_cast<Drawing::Circle&>(*shape);
                circle.center = scalePointAroundCenter(circle.center, center, f);
                circle.radius *= f;
                break;
            }
            case Drawing::ShapeType::TRIANGLE: {
                auto& tri = static_cast<Drawing::Triangle&>(*shape);
                for (auto& pt : tri.points) pt = scalePointAroundCenter(pt, center, f);
                break;
            }
            case Drawing::ShapeType::SQUARE: {
                auto& sq = static_cast<Drawing::Square&>(*shape);
                sq.start = scalePointAroundCenter(sq.start, center, f);
                sq.end   = scalePointAroundCenter(sq.end, center, f);
                break;
            }
            case Drawing::ShapeType::RECTANGLE: {
                auto& rect = static_cast<Drawing::Rectangle&>(*shape);
                rect.topLeft     = scalePointAroundCenter(rect.topLeft, center, f);
                rect.topRight    = scalePointAroundCenter(rect.topRight, center, f);
                rect.bottomRight = scalePointAroundCenter(rect.bottomRight, center, f);
                rect.bottomLeft  = scalePointAroundCenter(rect.bottomLeft, center, f);
                break;
            }
            case Drawing::ShapeType::SPLINE: {
                auto& spline = static_cast<Drawing::Spline&>(*shape);
                for (auto& cp : spline.controlPoints) cp = scalePointAroundCenter(cp, center, f);
                break;
            }
            case Drawing::ShapeType::BEZIER: {
                auto& bezier = static_cast<Drawing::BezierCurve&>(*shape);
                for (auto& cp : bezier.controlPoints) cp = scalePointAroundCenter(cp, center, f);
                break;
            }
            case Drawing::ShapeType::DIMENSION: {
                auto& dim = static_cast<Drawing::Dimension&>(*shape);
                dim.start        = scalePointAroundCenter(dim.start, center, f);
                dim.end          = scalePointAroundCenter(dim.end, center, f);
                dim.textPosition = scalePointAroundCenter(dim.textPosition, center, f);
                dim.lengthInPixels *= f;
                break;
            }
            case Drawing::ShapeType::BALL_BEARING: {
                auto& bb = static_cast<Drawing::BallBearing&>(*shape);
                bb.position = scalePointAroundCenter(bb.position, center, f);
                bb.outerDiameter *= f;
                bb.innerDiameter *= f;
                bb.width *= f;
                bb.ballDiameter *= f;
                bb.raceRadius *= f;
                break;
            }
            case Drawing::ShapeType::SPRING2D: {
                auto& spring = static_cast<Drawing::Spring2D&>(*shape);
                glm::dvec2 pos(spring.centerX, spring.centerY);
                pos = scalePointAroundCenter(pos, center, f);
                spring.centerX = static_cast<float>(pos.x);
                spring.centerY = static_cast<float>(pos.y);
                spring.outerDiameter *= f;
                spring.wireDiameter *= f;
                spring.freeLength *= f;
                break;
            }
            case Drawing::ShapeType::BELLOWS: {
                auto& bellows = static_cast<Drawing::Bellows&>(*shape);
                bellows.position = scalePointAroundCenter(bellows.position, center, f);
                bellows.cuffAInnerDiameter *= f;
                bellows.cuffBInnerDiameter *= f;
                bellows.cuffALength *= f;
                bellows.cuffBLength *= f;
                bellows.baseConvolutionDiameter *= f;
                bellows.peakConvolutionDiameter *= f;
                bellows.convolutedSectionLength *= f;
                bellows.wallThickness *= f;
                bellows.invalidateCache();
                break;
            }
            default:
                break;
        }
    }

    Drawing::Shape* shape;
    float factor;
    glm::dvec2 center;
};

} // namespace Commands
} // namespace Core
