#include <glm/glm.hpp>
#include "shapes/BasicShapes.hpp"
#include <vector>
#include <imgui.h>

namespace Drawing {

std::vector<glm::dvec2> Spring2D::generateProfile() const {
    std::vector<glm::dvec2> profile;
    float r = outerDiameter / 2.0f - wireDiameter / 2.0f;
    float y0 = centerY - freeLength / 2.0f;
    float y1 = centerY + freeLength / 2.0f;
    profile.push_back(glm::dvec2(r, y0));
    profile.push_back(glm::dvec2(r, y1));
    return profile;
}

std::vector<glm::dvec2> ShockAbsorberEnd2D::generateProfile() const {
    std::vector<glm::dvec2> profile;
    float x = baseCenter.x;
    float y = baseCenter.y - shaftLength / 2.0f;
    // Step 3 (top, smallest)
    profile.push_back(glm::dvec2(x + step3Diameter / 2, y));
    y += step3Length;
    profile.push_back(glm::dvec2(x + step3Diameter / 2, y));
    // Step 2
    profile.push_back(glm::dvec2(x + step2Diameter / 2, y));
    y += step2Length;
    profile.push_back(glm::dvec2(x + step2Diameter / 2, y));
    // Step 1
    profile.push_back(glm::dvec2(x + step1Diameter / 2, y));
    y += step1Length;
    profile.push_back(glm::dvec2(x + step1Diameter / 2, y));
    // Now go up the left side
    profile.push_back(glm::dvec2(x - step1Diameter / 2, y));
    y -= step1Length;
    profile.push_back(glm::dvec2(x - step1Diameter / 2, y));
    // Step 2
    profile.push_back(glm::dvec2(x - step2Diameter / 2, y));
    y -= step2Length;
    profile.push_back(glm::dvec2(x - step2Diameter / 2, y));
    // Step 3
    profile.push_back(glm::dvec2(x - step3Diameter / 2, y));
    y -= step3Length;
    profile.push_back(glm::dvec2(x - step3Diameter / 2, y));
    return profile;
}

} // namespace Drawing 