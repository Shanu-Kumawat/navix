#include <glm/glm.hpp>
#include "utils/MathUtils.hpp"
#include <algorithm>

namespace Drawing {
using namespace Drawing::Math;
namespace Math {

float calculateDistance(const glm::dvec2& p1, const glm::dvec2& p2) {
    return distance(p1, p2);
}

double calculateDistanceToLine(const glm::dvec2& point, const glm::dvec2& lineStart, const glm::dvec2& lineEnd) {
    float lineLength = distance(lineStart, lineEnd);
    if (lineLength < 0.0001f) return distance(point, lineStart);

    glm::dvec2 lineDir = normalize(lineEnd - lineStart);
    glm::dvec2 toPoint = point - lineStart;
    
    double projection = dot(toPoint, lineDir);
    
    if (projection <= 0.0f) return distance(point, lineStart);
    if (projection >= lineLength) return distance(point, lineEnd);
    
    glm::dvec2 closestPoint = lineStart + lineDir * projection;
    return distance(point, closestPoint);
}

glm::dvec2 calculateMidpoint(const glm::dvec2& p1, const glm::dvec2& p2) {
    return glm::mix(p1, p2, 0.5f);
}

float calculateAngle(const glm::dvec2& v1, const glm::dvec2& v2) {
    double d = dot(normalize(v1), normalize(v2));
    return std::acos(glm::clamp(d, -1.0, 1.0));
}

bool isPointInRect(const glm::dvec2& point, const glm::dvec2& rectMin, const glm::dvec2& rectMax) {
    return point.x >= rectMin.x && point.x <= rectMax.x &&
           point.y >= rectMin.y && point.y <= rectMax.y;
}

float calculateTriangleArea(const glm::dvec2& p1, const glm::dvec2& p2, const glm::dvec2& p3) {
    return std::abs(cross(p2 - p1, p3 - p1)) * 0.5f;
}

glm::dvec2 snapToGrid(const glm::dvec2& point, float gridSize) {
    return glm::dvec2(
        std::round(point.x / gridSize) * gridSize,
        std::round(point.y / gridSize) * gridSize
    );
}

bool isPointNearPoint(const glm::dvec2& p1, const glm::dvec2& p2, float threshold) {
    return distance(p1, p2) <= threshold;
}

glm::dvec2 normalizeVector(const glm::dvec2& v) {
    return normalize(v);
}

float dotProduct(const glm::dvec2& v1, const glm::dvec2& v2) {
    return dot(v1, v2);
}

glm::dvec2 rotatePoint(const glm::dvec2& point, const glm::dvec2& center, float angle) {
    float s = std::sin(angle);
    float c = std::cos(angle);
    
    glm::dvec2 translated = point - center;
    glm::dvec2 rotated(
        translated.x * c - translated.y * s,
        translated.x * s + translated.y * c
    );
    
    return rotated + center;
}

} // namespace Math
} // namespace Drawing 