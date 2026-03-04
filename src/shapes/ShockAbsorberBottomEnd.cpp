#include <glm/glm.hpp>
#include "shapes/ShockAbsorberBottomEnd.hpp"
#include "Canvas.hpp"
#include <cmath>
#include <iostream>

namespace Drawing {
using namespace Drawing::Math;

ShockAbsorberBottomEnd::ShockAbsorberBottomEnd(const Spring2D* spring, ImU32 color, float lineThickness)
    : Shape(ShapeType::SHOCK_ABSORBER_BOTTOM_END, color, lineThickness), parentSpring(spring) {
    updateGeometry();
}

void ShockAbsorberBottomEnd::updateGeometry() {
    float od = parentSpring->outerDiameter;
    float wd = parentSpring->wireDiameter;
    float fl = parentSpring->freeLength;
    width = od * 0.9f;
    height = wd * 2.5f;
    thickness = wd * 0.6f;
    plateThickness = wd * 0.7f;
    holeRadius = wd * 0.85f;
    // Place baseCenter at the center of the plate
    baseCenter = glm::dvec2(parentSpring->centerX, parentSpring->centerY + fl / 2 + plateThickness / 2 + height / 2);
}

std::unique_ptr<Shape> ShockAbsorberBottomEnd::clone() const {
    return std::make_unique<ShockAbsorberBottomEnd>(*this);
}

bool ShockAbsorberBottomEnd::isValid() const {
    return parentSpring != nullptr && width > 0 && height > 0 && thickness > 0 && holeRadius > 0 && plateThickness > 0;
}

bool ShockAbsorberBottomEnd::isPointNear(const glm::dvec2& point, float threshold) const {
    glm::dvec2 min, max;
    getBounds(min, max);
    return (point.x >= min.x - threshold && point.x <= max.x + threshold &&
            point.y >= min.y - threshold && point.y <= max.y + threshold);
}

void ShockAbsorberBottomEnd::getBounds(glm::dvec2& min, glm::dvec2& max) const {
    // Compute shaft height to touch step1 of ShockAbsorberEnd2D
    float fl = parentSpring->freeLength;
    float step1Length = fl * 0.50f;
    float plateTopY = baseCenter.y - plateThickness / 2;
    float shaftTopY = parentSpring->centerY - fl / 2 + step1Length; // bottom of step1
    min = glm::dvec2(baseCenter.x - width / 2, shaftTopY);
    max = glm::dvec2(baseCenter.x + width / 2, baseCenter.y + plateThickness / 2);
}

void ShockAbsorberBottomEnd::draw(ImDrawList* drawList, const Canvas* canvas) const {
    auto tx = [&](const glm::dvec2& p) { return canvas->transformCoordinates(p); };
    ImU32 c = color;
    float t = std::max(thickness * canvas->getZoomLevel(), 2.0f);
    
    // Define PI if not available
    const float PI = 3.14159265358979323846f;
    
    // Shaft
    float shaftWidth = thickness * 1.2f;
    float fl = parentSpring->freeLength;
    float step1Length = fl * 0.50f;
    float plateTopY = baseCenter.y - plateThickness / 2;
    float shaftTopY = parentSpring->centerY - fl / 2 + step1Length; // bottom of step1
    glm::dvec2 shaftTop(baseCenter.x - shaftWidth / 2, shaftTopY);
    glm::dvec2 shaftBot(baseCenter.x + shaftWidth / 2, plateTopY);
    ImVec2 shaftPts[4] = {Drawing::Math::toImVec2(tx(shaftTop)), Drawing::Math::toImVec2(tx(glm::dvec2(shaftBot.x, shaftTop.y))), Drawing::Math::toImVec2(tx(shaftBot)), Drawing::Math::toImVec2(tx(glm::dvec2(shaftTop.x, shaftBot.y)))};
    drawList->AddPolyline(shaftPts, 4, c, ImDrawFlags_Closed, t);
    // Plate
    glm::dvec2 plateMin(baseCenter.x - width / 2, plateTopY);
    glm::dvec2 plateMax(baseCenter.x + width / 2, plateTopY + plateThickness);
    glm::dvec2 p1 = tx(plateMin), p2 = tx(glm::dvec2(plateMax.x, plateMin.y)), p3 = tx(plateMax), p4 = tx(glm::dvec2(plateMin.x, plateMax.y));
    ImVec2 platePts[4] = {Drawing::Math::toImVec2(p1), Drawing::Math::toImVec2(p2), Drawing::Math::toImVec2(p3), Drawing::Math::toImVec2(p4)};
    drawList->AddPolyline(platePts, 4, c, ImDrawFlags_Closed, t);
    // U-bracket legs
    float legTop = plateTopY + plateThickness;
    float legBottom = legTop + height;
    // Left leg
    glm::dvec2 l1 = tx(glm::dvec2(plateMin.x, legTop)), l2 = tx(glm::dvec2(plateMin.x, legBottom));
    drawList->AddLine(Drawing::Math::toImVec2(l1), Drawing::Math::toImVec2(l2), c, t);
    // Right leg
    glm::dvec2 r1 = tx(glm::dvec2(plateMax.x, legTop)), r2 = tx(glm::dvec2(plateMax.x, legBottom));
    drawList->AddLine(Drawing::Math::toImVec2(r1), Drawing::Math::toImVec2(r2), c, t);
    // Bottom arc (semi-circle at the very bottom)
    glm::dvec2 arcCenter = tx(glm::dvec2(baseCenter.x, legBottom));
    float arcRadius = (width - 2 * thickness) / 2 * canvas->getZoomLevel();
    int arcSegments = 32;
    std::vector<ImVec2> arcPts;
    for (int i = 0; i <= arcSegments; ++i) {
        float theta = (float(i) / arcSegments) * PI;
        float x = arcCenter.x + arcRadius * cosf(theta);
        float y = arcCenter.y + arcRadius * sinf(theta);
    arcPts.push_back(ImVec2(x, y));
    }
    drawList->AddPolyline(arcPts.data(), arcPts.size(), c, 0, t);
    // Hole
    glm::dvec2 holeCenter = tx(glm::dvec2(baseCenter.x, legTop + height / 2));
    drawList->AddCircle(Drawing::Math::toImVec2(holeCenter), holeRadius * canvas->getZoomLevel(), c, 0, t);
}

std::vector<glm::dvec2> ShockAbsorberBottomEnd::generateProfile() const {
    std::vector<glm::dvec2> profile;
    
    // Define PI if not available
    const float PI = 3.14159265358979323846f;
    
    // Plate top
    float plateTopY = baseCenter.y - plateThickness / 2;
    float plateBottomY = plateTopY + plateThickness;
    float leftX = baseCenter.x - width / 2;
    float rightX = baseCenter.x + width / 2;
    // Shaft
    float shaftWidth = thickness * 1.2f;
    float shaftTopY = parentSpring->centerY - parentSpring->freeLength / 2 + parentSpring->freeLength * 0.50f; // bottom of step1
    float shaftLeftX = baseCenter.x - shaftWidth / 2;
    float shaftRightX = baseCenter.x + shaftWidth / 2;
    // U-bracket legs
    float legTop = plateBottomY;
    float legBottom = legTop + height;
    // Arc (semi-circle at the very bottom)
    float arcCenterY = legBottom;
    float arcRadius = (width - 2 * thickness) / 2;
    int arcSegments = 16;
    
    std::cout << "[ShockAbsorberBottomEnd] Generating profile with " << arcSegments << " arc segments" << std::endl;
    
    // Build profile (left side, down, arc, up right side)
    profile.push_back(glm::dvec2(shaftLeftX, shaftTopY));
    profile.push_back(glm::dvec2(shaftLeftX, plateTopY));
    profile.push_back(glm::dvec2(leftX, plateTopY));
    profile.push_back(glm::dvec2(leftX, plateBottomY));
    profile.push_back(glm::dvec2(leftX, legBottom));
    // Arc (bottom)
    for (int i = arcSegments; i >= 0; --i) {
        float theta = PI - (float(i) / arcSegments) * PI;
        float x = baseCenter.x + arcRadius * cosf(theta);
        float y = arcCenterY + arcRadius * sinf(theta);
        profile.push_back(glm::dvec2(x, y));
    }
    profile.push_back(glm::dvec2(rightX, legBottom));
    profile.push_back(glm::dvec2(rightX, plateBottomY));
    profile.push_back(glm::dvec2(rightX, plateTopY));
    profile.push_back(glm::dvec2(shaftRightX, plateTopY));
    profile.push_back(glm::dvec2(shaftRightX, shaftTopY));
    
    std::cout << "[ShockAbsorberBottomEnd] Generated profile with " << profile.size() << " points" << std::endl;
    
    return profile;
}

} // namespace Drawing 