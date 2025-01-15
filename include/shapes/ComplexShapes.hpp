#pragma once

#include <imgui.h>
#include <vector>
#include "shapes/BasicShapes.hpp"
#include "utils/MathUtils.hpp"

namespace Drawing {

struct Spline {
    std::vector<ImVec2> controlPoints;
    ImU32 color;
    float thickness;
    bool isClosed;

    bool isValid() const {
        return controlPoints.size() >= 2;
    }

    // Helper method to calculate points along the spline
    std::vector<ImVec2> calculatePoints(float resolution = 0.1f) const;
};

struct BezierCurve {
    std::vector<ImVec2> controlPoints;  // For cubic bezier, needs 4 points
    ImU32 color;
    float thickness;
    float startT;  // For truncation (0.0 to 1.0)
    float endT;    // For truncation (0.0 to 1.0)

    bool isValid() const {
        return controlPoints.size() == 4;  // Cubic bezier requires exactly 4 control points
    }

    // Helper method to calculate a point along the curve at parameter t
    ImVec2 calculatePoint(float t) const;
    
    // Helper method to calculate points along the curve
    std::vector<ImVec2> calculatePoints(float resolution = 0.01f) const;
};

} // namespace Drawing 