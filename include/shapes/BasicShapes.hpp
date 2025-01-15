#pragma once

#include <imgui.h>
#include <array>
#include <vector>
#include "utils/MathUtils.hpp"
#include "Constants.hpp"

namespace Drawing {

// Shape structures with validation methods
struct Point {
    ImVec2 position;
    ImU32 color;
    float size;

    bool isValid() const { return size > 0.0f; }
};

struct Line {
    ImVec2 start;
    ImVec2 end;
    ImU32 color;
    float thickness;

    bool isValid() const {
        return thickness > 0.0f && 
               Math::calculateDistance(start, end) > Constants::MIN_SHAPE_SIZE;
    }
};

struct Circle {
    ImVec2 center;
    float radius;
    ImU32 color;
    float thickness;

    bool isValid() const {
        return thickness > 0.0f && radius > Constants::MIN_SHAPE_SIZE;
    }
};

struct Triangle {
    std::array<ImVec2, 3> points;
    ImU32 color;
    float thickness;

    bool isValid() const {
        return thickness > 0.0f && 
               Math::calculateTriangleArea(points[0], points[1], points[2]) > Constants::MIN_SHAPE_SIZE;
    }
};

struct Square {
    ImVec2 start;
    ImVec2 end;
    ImU32 color;
    float thickness;

    bool isValid() const {
        return thickness > 0.0f && 
               Math::calculateDistance(start, end) > Constants::MIN_SHAPE_SIZE;
    }
};

struct Rectangle {
    ImVec2 start;
    ImVec2 end;
    ImU32 color;
    float thickness;

    bool isValid() const {
        return thickness > 0.0f && 
               std::fabs(end.x - start.x) > Constants::MIN_SHAPE_SIZE &&
               std::fabs(end.y - start.y) > Constants::MIN_SHAPE_SIZE;
    }
};

enum class DrawingMode {
    None,
    Point,
    Line,
    Circle,
    Triangle,
    Square,
    Rectangle,
    Spline,
    BezierCurve
};

} // namespace Drawing 