#include <glm/glm.hpp>
#pragma once

#include <imgui.h>
#include <cmath>
#include "utils/VectorMath.hpp"

namespace Drawing {
using namespace Drawing::Math;
namespace Math {

/**
 * @brief Calculate the Euclidean distance between two points
 */
float calculateDistance(const glm::dvec2& p1, const glm::dvec2& p2);

/**
 * @brief Calculate the distance from a point to a line
 */
double calculateDistanceToLine(const glm::dvec2& point, const glm::dvec2& lineStart, const glm::dvec2& lineEnd);

/**
 * @brief Calculate the midpoint between two points
 */
glm::dvec2 calculateMidpoint(const glm::dvec2& p1, const glm::dvec2& p2);

/**
 * @brief Calculate the angle between two points (in radians)
 */
float calculateAngle(const glm::dvec2& v1, const glm::dvec2& v2);

/**
 * @brief Check if a point lies within a rectangle
 */
bool isPointInRect(const glm::dvec2& point, const glm::dvec2& rectMin, const glm::dvec2& rectMax);

/**
 * @brief Calculate the area of a triangle defined by three points
 */
float calculateTriangleArea(const glm::dvec2& p1, const glm::dvec2& p2, const glm::dvec2& p3);

/**
 * @brief Snap a point to the nearest grid intersection
 */
glm::dvec2 snapToGrid(const glm::dvec2& point, float gridSize);

/**
 * @brief Check if two points are within a threshold distance of each other
 */
bool isPointNearPoint(const glm::dvec2& p1, const glm::dvec2& p2, float threshold);

/**
 * @brief Normalize a vector to unit length
 */
glm::dvec2 normalizeVector(const glm::dvec2& v);

/**
 * @brief Calculate the dot product of two vectors
 */
float dotProduct(const glm::dvec2& v1, const glm::dvec2& v2);

/**
 * @brief Rotate a point around a center by a given angle
 */
glm::dvec2 rotatePoint(const glm::dvec2& point, const glm::dvec2& center, float angle);

constexpr float MM_PER_INCH = 25.4f;
constexpr float CM_PER_INCH = 2.54f;

inline float pixelsToMillimeters(float pixels, float dpi = 96.0f) {
    return pixels * MM_PER_INCH / dpi;
}

inline float millimetersToPixels(float mm, float dpi = 96.0f) {
    return mm * dpi / MM_PER_INCH;
}

inline float pixelsToCentimeters(float pixels, float dpi = 96.0f) {
    return pixels * CM_PER_INCH / dpi;
}

inline float centimetersToPixels(float cm, float dpi = 96.0f) {
    return cm * dpi / CM_PER_INCH;
}

} // namespace Math
} // namespace Drawing 