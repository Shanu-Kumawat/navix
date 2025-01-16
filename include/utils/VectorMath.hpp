#pragma once

#include <imgui.h>
#include <algorithm>
#include <cmath>

// Vector operations in global namespace for ImVec2
inline ImVec2 operator+(const ImVec2& a, const ImVec2& b) { return ImVec2(a.x + b.x, a.y + b.y); }
inline ImVec2 operator-(const ImVec2& a, const ImVec2& b) { return ImVec2(a.x - b.x, a.y - b.y); }
inline ImVec2 operator*(const ImVec2& a, float b) { return ImVec2(a.x * b, a.y * b); }
inline ImVec2 operator*(float b, const ImVec2& a) { return ImVec2(a.x * b, a.y * b); }
inline ImVec2 operator/(const ImVec2& a, float b) { return ImVec2(a.x / b, a.y / b); }

namespace Drawing {
namespace Math {

// Utility functions
inline float clamp(float value, float min, float max) {
    return std::min(std::max(value, min), max);
}

inline float length(const ImVec2& v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}

inline float distance(const ImVec2& a, const ImVec2& b) {
    ImVec2 d = b - a;
    return length(d);
}

inline float dot(const ImVec2& a, const ImVec2& b) {
    return a.x * b.x + a.y * b.y;
}

inline ImVec2 normalize(const ImVec2& v) {
    float len = length(v);
    return len > 0.0001f ? v / len : ImVec2(0, 0);
}

inline ImVec2 lerp(const ImVec2& a, const ImVec2& b, float t) {
    return a + (b - a) * t;
}

inline float cross(const ImVec2& a, const ImVec2& b) {
    return a.x * b.y - a.y * b.x;
}

} // namespace Math
} // namespace Drawing 