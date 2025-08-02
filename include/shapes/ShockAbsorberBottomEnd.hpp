#pragma once
#include "BasicShapes.hpp"

namespace Drawing {

struct ShockAbsorberBottomEnd : public Shape {
    const Spring2D* parentSpring;
    ImVec2 baseCenter; // Center of the mounting plate
    float width;
    float height;
    float thickness;
    float holeRadius;
    float plateThickness;

    ShockAbsorberBottomEnd(const Spring2D* spring, ImU32 color = IM_COL32(80, 80, 80, 255), float lineThickness = 2.0f);
    void updateGeometry();
    std::unique_ptr<Shape> clone() const override;
    bool isValid() const override;
    bool isPointNear(const ImVec2& point, float threshold) const override;
    void getBounds(ImVec2& min, ImVec2& max) const override;
    void draw(ImDrawList* drawList, const Canvas* canvas) const;
    std::vector<ImVec2> generateProfile() const;
};

} // namespace Drawing 