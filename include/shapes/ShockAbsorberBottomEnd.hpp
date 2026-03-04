#include <glm/glm.hpp>
#pragma once
#include "BasicShapes.hpp"

namespace Drawing {

struct ShockAbsorberBottomEnd : public Shape {
    const Spring2D* parentSpring;
    glm::dvec2 baseCenter; // Center of the mounting plate
    float width;
    float height;
    float thickness;
    float holeRadius;
    float plateThickness;

    ShockAbsorberBottomEnd(const Spring2D* spring, ImU32 color = IM_COL32(80, 80, 80, 255), float lineThickness = 2.0f);
    
    // Static method for template-based shape finding
    static ShapeType GetShapeType() { return ShapeType::SHOCK_ABSORBER_BOTTOM_END; }
    void updateGeometry();
    std::unique_ptr<Shape> clone() const override;
    bool isValid() const override;
    bool isPointNear(const glm::dvec2& point, float threshold) const override;
    void getBounds(glm::dvec2& min, glm::dvec2& max) const override;
    void draw(ImDrawList* drawList, const Canvas* canvas) const;
    std::vector<glm::dvec2> generateProfile() const;
};

} // namespace Drawing 