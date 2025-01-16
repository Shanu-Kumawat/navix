#include "utils/MathUtils.hpp"
#include <algorithm>

namespace Drawing {
namespace Math {

float calculateDistance(const ImVec2& p1, const ImVec2& p2) {
    return distance(p1, p2);
}

float calculateDistanceToLine(const ImVec2& point, const ImVec2& lineStart, const ImVec2& lineEnd) {
    float lineLength = distance(lineStart, lineEnd);
    if (lineLength < 0.0001f) return distance(point, lineStart);

    ImVec2 lineDir = normalize(lineEnd - lineStart);
    ImVec2 toPoint = point - lineStart;
    
    float projection = dot(toPoint, lineDir);
    
    if (projection <= 0.0f) return distance(point, lineStart);
    if (projection >= lineLength) return distance(point, lineEnd);
    
    ImVec2 closestPoint = lineStart + lineDir * projection;
    return distance(point, closestPoint);
}

ImVec2 calculateMidpoint(const ImVec2& p1, const ImVec2& p2) {
    return lerp(p1, p2, 0.5f);
}

float calculateAngle(const ImVec2& v1, const ImVec2& v2) {
    float d = dot(normalize(v1), normalize(v2));
    return std::acos(clamp(d, -1.0f, 1.0f));
}

bool isPointInRect(const ImVec2& point, const ImVec2& rectMin, const ImVec2& rectMax) {
    return point.x >= rectMin.x && point.x <= rectMax.x &&
           point.y >= rectMin.y && point.y <= rectMax.y;
}

float calculateTriangleArea(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3) {
    return std::abs(cross(p2 - p1, p3 - p1)) * 0.5f;
}

ImVec2 snapToGrid(const ImVec2& point, float gridSize) {
    return ImVec2(
        std::round(point.x / gridSize) * gridSize,
        std::round(point.y / gridSize) * gridSize
    );
}

bool isPointNearPoint(const ImVec2& p1, const ImVec2& p2, float threshold) {
    return distance(p1, p2) <= threshold;
}

ImVec2 normalizeVector(const ImVec2& v) {
    return normalize(v);
}

float dotProduct(const ImVec2& v1, const ImVec2& v2) {
    return dot(v1, v2);
}

ImVec2 rotatePoint(const ImVec2& point, const ImVec2& center, float angle) {
    float s = std::sin(angle);
    float c = std::cos(angle);
    
    ImVec2 translated = point - center;
    ImVec2 rotated(
        translated.x * c - translated.y * s,
        translated.x * s + translated.y * c
    );
    
    return rotated + center;
}

} // namespace Math
} // namespace Drawing 