#include "utils/MathUtils.hpp"

namespace Drawing::Math {

float calculateDistance(const ImVec2& p1, const ImVec2& p2) {
    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    return std::sqrt(dx * dx + dy * dy);
}

ImVec2 calculateMidpoint(const ImVec2& p1, const ImVec2& p2) {
    return ImVec2(
        (p1.x + p2.x) * 0.5f,
        (p1.y + p2.y) * 0.5f
    );
}

float calculateAngle(const ImVec2& p1, const ImVec2& p2) {
    return std::atan2(p2.y - p1.y, p2.x - p1.x);
}

bool isPointInRect(const ImVec2& point, const ImVec2& rectMin, const ImVec2& rectMax) {
    return point.x >= rectMin.x && point.x <= rectMax.x &&
           point.y >= rectMin.y && point.y <= rectMax.y;
}

float calculateTriangleArea(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3) {
    return std::abs(
        (p1.x * (p2.y - p3.y) + p2.x * (p3.y - p1.y) + p3.x * (p1.y - p2.y)) / 2.0f
    );
}

ImVec2 snapToGrid(const ImVec2& point, float gridSpacing) {
    float x = std::round(point.x / gridSpacing) * gridSpacing;
    float y = std::round(point.y / gridSpacing) * gridSpacing;
    return ImVec2(x, y);
}

bool isPointNearPoint(const ImVec2& p1, const ImVec2& p2, float threshold) {
    return calculateDistance(p1, p2) <= threshold;
}

ImVec2 normalizeVector(const ImVec2& vec) {
    float length = std::sqrt(vec.x * vec.x + vec.y * vec.y);
    if (length < 1e-6f) return ImVec2(0.0f, 0.0f);
    return ImVec2(vec.x / length, vec.y / length);
}

float dotProduct(const ImVec2& v1, const ImVec2& v2) {
    return v1.x * v2.x + v1.y * v2.y;
}

ImVec2 rotatePoint(const ImVec2& point, const ImVec2& center, float angle) {
    float s = std::sin(angle);
    float c = std::cos(angle);

    float px = point.x - center.x;
    float py = point.y - center.y;

    float xnew = px * c - py * s;
    float ynew = px * s + py * c;

    return ImVec2(xnew + center.x, ynew + center.y);
}

} // namespace Drawing::Math 