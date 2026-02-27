#include <glm/glm.hpp>
#include "shapes/ComplexShapes.hpp"
#include <algorithm>
#include <cmath>

namespace Drawing {
using namespace Drawing::Math;

// Spline methods
void Spline::addControlPoint(const glm::dvec2& point) {
    controlPoints.push_back(point);
}

void Spline::removeControlPoint(size_t index) {
    if (index < controlPoints.size()) {
        controlPoints.erase(controlPoints.begin() + index);
    }
}

void Spline::moveControlPoint(size_t index, const glm::dvec2& newPos) {
    if (index < controlPoints.size()) {
        controlPoints[index] = newPos;
    }
}

void Spline::moveEntireSpline(const glm::dvec2& delta) {
    for (auto& point : controlPoints) {
        point.x += delta.x;
        point.y += delta.y;
    }
}

void Spline::smoothen(float factor) {
    if (controlPoints.size() < 3) return;

    std::vector<glm::dvec2> newPoints = controlPoints;
    for (size_t i = 1; i < controlPoints.size() - 1; ++i) {
        glm::dvec2& prev = controlPoints[i - 1];
        glm::dvec2& curr = controlPoints[i];
        glm::dvec2& next = controlPoints[i + 1];

        newPoints[i].x = curr.x + (prev.x + next.x - 2 * curr.x) * factor;
        newPoints[i].y = curr.y + (prev.y + next.y - 2 * curr.y) * factor;
    }
    controlPoints = newPoints;
}

void Spline::subdivide(float threshold) {
    if (controlPoints.size() < 2) return;

    std::vector<glm::dvec2> newPoints;
    newPoints.push_back(controlPoints[0]);

    for (size_t i = 0; i < controlPoints.size() - 1; ++i) {
        const glm::dvec2& p1 = controlPoints[i];
        const glm::dvec2& p2 = controlPoints[i + 1];
        float dist = Math::calculateDistance(p1, p2);

        if (dist > threshold) {
            glm::dvec2 mid = Math::calculateMidpoint(p1, p2);
            newPoints.push_back(mid);
        }
        newPoints.push_back(p2);
    }

    controlPoints = newPoints;
}

bool Spline::isPointNear(const glm::dvec2& point, float threshold) const {
    if (!isValid()) return false;

    const float thresholdSquared = threshold * threshold;

    // Check if point is near any control points with improved precision
    for (const auto& controlPoint : controlPoints) {
        const float dx = point.x - controlPoint.x;
        const float dy = point.y - controlPoint.y;
        const float distSquared = dx * dx + dy * dy;
        if (distSquared <= thresholdSquared) {
            return true;
        }
    }

    // Check if point is near the spline curve with improved accuracy
    const float smallStep = 0.01f;  // Smaller step for more accurate curve checking
    std::vector<glm::dvec2> curvePoints = calculatePoints(smallStep);
    
    for (size_t i = 1; i < curvePoints.size(); ++i) {
        const glm::dvec2& start = curvePoints[i - 1];
        const glm::dvec2& end = curvePoints[i];
        
        // Calculate squared length of line segment
        const float segmentDx = end.x - start.x;
        const float segmentDy = end.y - start.y;
        const float segmentLengthSquared = segmentDx * segmentDx + segmentDy * segmentDy;
        
        if (segmentLengthSquared < 1e-6f) continue;  // Skip degenerate segments
        
        // Calculate projection of point onto line segment
        const float t = std::clamp<double>(
            ((point.x - start.x) * segmentDx + (point.y - start.y) * segmentDy) / segmentLengthSquared,
            0.0, 1.0
        );
        
        // Calculate closest point on line segment
        const float closestX = start.x + t * segmentDx;
        const float closestY = start.y + t * segmentDy;
        
        // Calculate squared distance to closest point
        const float dx = point.x - closestX;
        const float dy = point.y - closestY;
        const float distSquared = dx * dx + dy * dy;
        
        if (distSquared <= thresholdSquared) {
            return true;
        }
    }
    return false;
}

int Spline::findNearestControlPoint(const glm::dvec2& point, float threshold) const {
    const float thresholdSquared = threshold * threshold;
    float minDistSquared = thresholdSquared;
    int nearest = -1;

    for (size_t i = 0; i < controlPoints.size(); ++i) {
        const float dx = point.x - controlPoints[i].x;
        const float dy = point.y - controlPoints[i].y;
        const float distSquared = dx * dx + dy * dy;
        
        if (distSquared < minDistSquared) {
            minDistSquared = distSquared;
            nearest = static_cast<int>(i);
        }
    }

    return nearest;
}

std::vector<glm::dvec2> Spline::calculatePoints(float resolution) const {
    if (!isValid()) return {};

    std::vector<glm::dvec2> points;
    const size_t numPoints = static_cast<size_t>(1.0f / resolution) + 1;
    points.reserve(numPoints);

    auto catmullRom = [this](const glm::dvec2& p0, const glm::dvec2& p1, const glm::dvec2& p2, const glm::dvec2& p3, float t) {
        // Ensure t is in [0,1]
        t = std::clamp<double>(t, 0.0, 1.0);
        float t2 = t * t;
        float t3 = t2 * t;

        // Apply tension parameter with bounds checking
        float alpha = std::clamp<double>((1.0f - tension) * 0.5f, 0.0, 1.0);

        // Improved numerical stability by grouping terms
        float c0 = -alpha * t3 + 2.0f * alpha * t2 - alpha * t;
        float c1 = (2.0f - alpha) * t3 + (alpha - 3.0f) * t2 + 1.0f;
        float c2 = (alpha - 2.0f) * t3 + (3.0f - 2.0f * alpha) * t2 + alpha * t;
        float c3 = alpha * t3 - alpha * t2;

        glm::dvec2 result;
        result.x = c0 * p0.x + c1 * p1.x + c2 * p2.x + c3 * p3.x;
        result.y = c0 * p0.y + c1 * p1.y + c2 * p2.y + c3 * p3.y;
        return result;
    };

    size_t n = controlPoints.size();
    if (n < 4) {
        // Handle special cases for fewer points
        if (n == 2) {
            // Linear interpolation between two points
            for (float t = 0; t <= 1.0f; t += resolution) {
                glm::dvec2 point;
                point.x = controlPoints[0].x + t * (controlPoints[1].x - controlPoints[0].x);
                point.y = controlPoints[0].y + t * (controlPoints[1].y - controlPoints[0].y);
                points.push_back(point);
            }
        } else if (n == 3) {
            // Quadratic interpolation between three points
            for (float t = 0; t <= 1.0f; t += resolution) {
                float t2 = t * t;
                float mt = 1.0f - t;
                float mt2 = mt * mt;
                glm::dvec2 point;
                point.x = mt2 * controlPoints[0].x + 2.0f * mt * t * controlPoints[1].x + t2 * controlPoints[2].x;
                point.y = mt2 * controlPoints[0].y + 2.0f * mt * t * controlPoints[1].y + t2 * controlPoints[2].y;
                points.push_back(point);
            }
        }
        return points;
    }

    if (isClosed) {
        // Handle closed spline with proper wrapping
        for (size_t i = 0; i < n; i++) {
            for (float t = 0; t <= 1.0f; t += resolution) {
                size_t p0 = (i + n - 1) % n;
                size_t p1 = i;
                size_t p2 = (i + 1) % n;
                size_t p3 = (i + 2) % n;
                points.push_back(catmullRom(
                    controlPoints[p0], 
                    controlPoints[p1],
                    controlPoints[p2], 
                    controlPoints[p3], 
                    t
                ));
            }
        }
    } else {
        // Handle open spline with proper boundary conditions
        // Use extrapolated points for the ends
        glm::dvec2 startExtrapolated = controlPoints[0] * 2.0 - controlPoints[1];
        glm::dvec2 endExtrapolated = controlPoints[n-1] * 2.0 - controlPoints[n-2];

        // First segment
        for (float t = 0; t <= 1.0f; t += resolution) {
            points.push_back(catmullRom(
                startExtrapolated,
                controlPoints[0],
                controlPoints[1],
                controlPoints[2],
                t
            ));
        }

        // Middle segments
        for (size_t i = 1; i < n - 2; i++) {
            for (float t = 0; t <= 1.0f; t += resolution) {
                points.push_back(catmullRom(
                    controlPoints[i-1],
                    controlPoints[i],
                    controlPoints[i+1],
                    controlPoints[i+2],
                    t
                ));
            }
        }

        // Last segment
        for (float t = 0; t <= 1.0f; t += resolution) {
            points.push_back(catmullRom(
                controlPoints[n-3],
                controlPoints[n-2],
                controlPoints[n-1],
                endExtrapolated,
                t
            ));
        }
    }

    // Remove duplicate points
    points.erase(
        std::unique(points.begin(), points.end(), 
            [](const glm::dvec2& a, const glm::dvec2& b) {
                const float EPSILON = 1e-5f;
                return std::abs(a.x - b.x) < EPSILON && std::abs(a.y - b.y) < EPSILON;
            }
        ),
        points.end()
    );

    return points;
}

// BezierCurve methods
void BezierCurve::moveControlPoint(size_t index, const glm::dvec2& newPos) {
    if (index < controlPoints.size()) {
        glm::dvec2 oldPos = controlPoints[index];
        controlPoints[index] = newPos;

        if (isSymmetrical && (index == 1 || index == 2)) {
            // Adjust the opposite control point symmetrically
            size_t oppositeIndex = (index == 1) ? 2 : 1;
            glm::dvec2 center = controlPoints[index == 1 ? 0 : 3];
            glm::dvec2 delta;
            delta.x = newPos.x - oldPos.x;
            delta.y = newPos.y - oldPos.y;
            controlPoints[oppositeIndex].x -= delta.x;
            controlPoints[oppositeIndex].y -= delta.y;
        }
    }
}

void BezierCurve::moveEntireCurve(const glm::dvec2& delta) {
    for (auto& point : controlPoints) {
        point.x += delta.x;
        point.y += delta.y;
    }
}

bool BezierCurve::isPointNear(const glm::dvec2& point, float threshold) const {
    if (!isValid()) return false;

    const float thresholdSquared = threshold * threshold;

    // Check if point is near any control points with improved precision
    for (const auto& controlPoint : controlPoints) {
        const float dx = point.x - controlPoint.x;
        const float dy = point.y - controlPoint.y;
        const float distSquared = dx * dx + dy * dy;
        if (distSquared <= thresholdSquared) {
            return true;
        }
    }

    // Check if point is near the curve with improved accuracy
    const float smallStep = 0.01f;  // Smaller step for more accurate curve checking
    std::vector<glm::dvec2> curvePoints = calculatePoints(smallStep);
    
    for (size_t i = 1; i < curvePoints.size(); ++i) {
        const glm::dvec2& start = curvePoints[i - 1];
        const glm::dvec2& end = curvePoints[i];
        
        // Calculate squared length of line segment
        const float segmentDx = end.x - start.x;
        const float segmentDy = end.y - start.y;
        const float segmentLengthSquared = segmentDx * segmentDx + segmentDy * segmentDy;
        
        if (segmentLengthSquared < 1e-6f) continue;  // Skip degenerate segments
        
        // Calculate projection of point onto line segment
        const float t = std::clamp<double>(
            ((point.x - start.x) * segmentDx + (point.y - start.y) * segmentDy) / segmentLengthSquared,
            0.0, 1.0
        );
        
        // Calculate closest point on line segment
        const float closestX = start.x + t * segmentDx;
        const float closestY = start.y + t * segmentDy;
        
        // Calculate squared distance to closest point
        const float dx = point.x - closestX;
        const float dy = point.y - closestY;
        const float distSquared = dx * dx + dy * dy;
        
        if (distSquared <= thresholdSquared) {
            return true;
        }
    }
    return false;
}

int BezierCurve::findNearestControlPoint(const glm::dvec2& point, float threshold) const {
    const float thresholdSquared = threshold * threshold;
    float minDistSquared = thresholdSquared;
    int nearest = -1;

    for (size_t i = 0; i < controlPoints.size(); ++i) {
        const float dx = point.x - controlPoints[i].x;
        const float dy = point.y - controlPoints[i].y;
        const float distSquared = dx * dx + dy * dy;
        
        if (distSquared < minDistSquared) {
            minDistSquared = distSquared;
            nearest = static_cast<int>(i);
        }
    }

    return nearest;
}

void BezierCurve::adjustSymmetrically(size_t index, const glm::dvec2& newPos) {
    if (index >= controlPoints.size()) return;

    glm::dvec2 oldPos = controlPoints[index];
    moveControlPoint(index, newPos);

    if (isSymmetrical && (index == 1 || index == 2)) {
        size_t oppositeIndex = (index == 1) ? 2 : 1;
        size_t anchorIndex = (index == 1) ? 0 : 3;
        
        glm::dvec2 anchor = controlPoints[anchorIndex];
        float oldDist = Math::calculateDistance(oldPos, anchor);
        float newDist = Math::calculateDistance(newPos, anchor);
        float ratio = newDist / oldDist;

        glm::dvec2 oppositeVec;
        oppositeVec.x = controlPoints[oppositeIndex].x - anchor.x;
        oppositeVec.y = controlPoints[oppositeIndex].y - anchor.y;
        oppositeVec.x *= ratio;
        oppositeVec.y *= ratio;
        controlPoints[oppositeIndex].x = anchor.x + oppositeVec.x;
        controlPoints[oppositeIndex].y = anchor.y + oppositeVec.y;
    }
}

void BezierCurve::splitCurve(float t) {
    if (!isValid() || t <= 0.0f || t >= 1.0f) return;

    glm::dvec2 p0 = controlPoints[0];
    glm::dvec2 p1 = controlPoints[1];
    glm::dvec2 p2 = controlPoints[2];
    glm::dvec2 p3 = controlPoints[3];

    // De Casteljau's algorithm
    glm::dvec2 p01 = Math::calculateMidpoint(p0, p1);
    glm::dvec2 p12 = Math::calculateMidpoint(p1, p2);
    glm::dvec2 p23 = Math::calculateMidpoint(p2, p3);

    glm::dvec2 p012 = Math::calculateMidpoint(p01, p12);
    glm::dvec2 p123 = Math::calculateMidpoint(p12, p23);

    glm::dvec2 p0123 = Math::calculateMidpoint(p012, p123);

    // Create two new curves
    std::vector<glm::dvec2> firstHalf = {p0, p01, p012, p0123};
    std::vector<glm::dvec2> secondHalf = {p0123, p123, p23, p3};

    // Update current curve to be the first half
    controlPoints = firstHalf;
}

void BezierCurve::elevateOrder() {
    // Degree elevation for cubic Bezier curve (not commonly needed but provided for completeness)
    if (!isValid()) return;

    std::vector<glm::dvec2> newPoints;
    newPoints.resize(5);  // Quartic curve has 5 control points

    // Degree elevation formulas
    newPoints[0] = controlPoints[0];
    
    newPoints[1].x = controlPoints[0].x * 0.25f + controlPoints[1].x * 0.75f;
    newPoints[1].y = controlPoints[0].y * 0.25f + controlPoints[1].y * 0.75f;
    
    newPoints[2].x = controlPoints[1].x * 0.5f + controlPoints[2].x * 0.5f;
    newPoints[2].y = controlPoints[1].y * 0.5f + controlPoints[2].y * 0.5f;
    
    newPoints[3].x = controlPoints[2].x * 0.75f + controlPoints[3].x * 0.25f;
    newPoints[3].y = controlPoints[2].y * 0.75f + controlPoints[3].y * 0.25f;
    
    newPoints[4] = controlPoints[3];

    controlPoints = newPoints;
}

void BezierCurve::reduceOrder() {
    // Degree reduction for cubic Bezier curve (approximate)
    if (!isValid()) return;

    std::vector<glm::dvec2> newPoints;
    newPoints.resize(3);  // Quadratic curve has 3 control points

    // Simple degree reduction (approximate)
    newPoints[0] = controlPoints[0];
    newPoints[1].x = (controlPoints[1].x + controlPoints[2].x) * 0.5f;
    newPoints[1].y = (controlPoints[1].y + controlPoints[2].y) * 0.5f;
    newPoints[2] = controlPoints[3];

    controlPoints = newPoints;
}

// UI Helper Functions
namespace CurveUI {

void drawControlPoint(ImDrawList* drawList, const glm::dvec2& pos, bool isSelected, float size) {
    ImU32 color = isSelected ? IM_COL32(255, 255, 0, 255) : IM_COL32(255, 255, 255, 255);
    drawList->AddCircleFilled(Drawing::Math::toglm::dvec2(pos), size, color);
    drawList->AddCircle(Drawing::Math::toglm::dvec2(pos), size + 1.0f, IM_COL32(0, 0, 0, 255));
}

void drawControlPolygon(ImDrawList* drawList, const std::vector<glm::dvec2>& points, bool isSelected) {
    ImU32 color = isSelected ? IM_COL32(255, 255, 0, 128) : IM_COL32(128, 128, 128, 128);
    for (size_t i = 1; i < points.size(); ++i) {
        drawList->AddLine(Drawing::Math::toglm::dvec2(points[i - 1]), Drawing::Math::toglm::dvec2(points[i]), color, 1.0f);
    }
}

void drawTangentHandles(ImDrawList* drawList, const glm::dvec2& point, const glm::dvec2& tangent, bool isSelected) {
    ImU32 color = isSelected ? IM_COL32(255, 255, 0, 128) : IM_COL32(128, 128, 128, 128);
    drawList->AddLine(Drawing::Math::toglm::dvec2(point), Drawing::Math::toglm::dvec2(tangent), color, 1.0f);
    drawList->AddCircleFilled(Drawing::Math::toglm::dvec2(tangent), 3.0f, color);
}

void drawCurveManipulator(ImDrawList* drawList, const glm::dvec2& pos, float size, bool isSelected) {
    ImU32 color = isSelected ? IM_COL32(255, 255, 0, 255) : IM_COL32(255, 255, 255, 255);
    drawList->AddRect(
        glm::dvec2(pos.x - size, pos.y - size),
        glm::dvec2(pos.x + size, pos.y + size),
        color
    );
}

} // namespace CurveUI

// New Spline methods
void Spline::reverseDirection() {
    std::reverse(controlPoints.begin(), controlPoints.end());
}

void Spline::makeUniform() {
    if (controlPoints.size() < 2) return;
    
    float totalLength = 0.0f;
    for (size_t i = 1; i < controlPoints.size(); ++i) {
        totalLength += Math::calculateDistance(controlPoints[i], controlPoints[i-1]);
    }
    
    float segmentLength = totalLength / (controlPoints.size() - 1);
    std::vector<glm::dvec2> newPoints;
    newPoints.push_back(controlPoints.front());
    
    for (size_t i = 1; i < controlPoints.size() - 1; ++i) {
        float t = static_cast<float>(i) / (controlPoints.size() - 1);
        glm::dvec2 point = calculatePoint(t);
        newPoints.push_back(point);
    }
    
    newPoints.push_back(controlPoints.back());
    controlPoints = newPoints;
}

void Spline::adjustTension(float t) {
    tension = std::clamp<double>(t, 0.0, 1.0);
}

void Spline::optimizeControlPoints() {
    if (controlPoints.size() < 4) return;
    
    std::vector<glm::dvec2> optimized;
    optimized.push_back(controlPoints.front());
    
    for (size_t i = 1; i < controlPoints.size() - 1; ++i) {
        glm::dvec2 prev = controlPoints[i - 1];
        glm::dvec2 curr = controlPoints[i];
        glm::dvec2 next = controlPoints[i + 1];
        
        // Calculate angle between segments
        float angle = Math::calculateAngle(
            glm::dvec2(curr.x - prev.x, curr.y - prev.y),
            glm::dvec2(next.x - curr.x, next.y - curr.y)
        );
        
        // Keep point if angle is significant
        if (std::abs(angle) > 0.1f) {  // ~5.7 degrees
            optimized.push_back(curr);
        }
    }
    
    optimized.push_back(controlPoints.back());
    controlPoints = optimized;
}

float Spline::calculateLength() const {
    std::vector<glm::dvec2> points = calculatePoints(0.01f);
    float length = 0.0f;
    
    for (size_t i = 1; i < points.size(); ++i) {
        length += Math::calculateDistance(points[i], points[i-1]);
    }
    
    return length;
}

float Spline::calculateCurvature(float t) const {
    if (controlPoints.size() < 3) return 0.0f;
    
    // Get point and its derivatives
    glm::dvec2 p = calculatePoint(t);
    glm::dvec2 dp = calculateDerivative(t);
    glm::dvec2 ddp = calculateSecondDerivative(t);
    
    // Calculate curvature using the formula: |x'y'' - y'x''| / (x'² + y'²)^(3/2)
    float num = std::abs(dp.x * ddp.y - dp.y * ddp.x);
    float den = std::pow(dp.x * dp.x + dp.y * dp.y, 1.5f);
    
    return den > 0.0001f ? num / den : 0.0f;
}

std::vector<glm::dvec2> Spline::getTangents() const {
    std::vector<glm::dvec2> tangents;
    if (controlPoints.size() < 2) return tangents;
    
    for (size_t i = 0; i < controlPoints.size(); ++i) {
        float t = static_cast<float>(i) / (controlPoints.size() - 1);
        glm::dvec2 derivative = calculateDerivative(t);
        float len = std::sqrt(derivative.x * derivative.x + derivative.y * derivative.y);
        if (len > 0.0001f) {
            derivative.x /= len;
            derivative.y /= len;
        }
        tangents.push_back(derivative);
    }
    
    return tangents;
}

void Spline::insertKnot(float t) {
    if (t <= 0.0f || t >= 1.0f) return;
    
    glm::dvec2 newPoint = calculatePoint(t);
    
    // Find insertion point
    auto it = controlPoints.begin();
    float accumLength = 0.0f;
    float totalLength = calculateLength();
    
    for (; it != controlPoints.end() - 1; ++it) {
        float segLen = Math::calculateDistance(*it, *(it + 1));
        if (accumLength + segLen > t * totalLength) {
            break;
        }
        accumLength += segLen;
    }
    
    controlPoints.insert(it + 1, newPoint);
}

void Spline::removeKnot(size_t index, float tolerance) {
    if (index >= controlPoints.size() || controlPoints.size() <= 2) return;
    
    // Store original curve points for comparison
    std::vector<glm::dvec2> originalCurve = calculatePoints(0.01f);
    
    // Temporarily remove the point
    glm::dvec2 removedPoint = controlPoints[index];
    controlPoints.erase(controlPoints.begin() + index);
    
    // Check if the curve still maintains its shape within tolerance
    std::vector<glm::dvec2> newCurve = calculatePoints(0.01f);
    
    float maxError = 0.0f;
    for (size_t i = 0; i < std::min(originalCurve.size(), newCurve.size()); ++i) {
        float error = Math::calculateDistance(originalCurve[i], newCurve[i]);
        maxError = std::max(maxError, error);
    }
    
    // Restore point if error is too large
    if (maxError > tolerance) {
        controlPoints.insert(controlPoints.begin() + index, removedPoint);
    }
}

// Helper methods for curvature calculation
glm::dvec2 Spline::calculatePoint(float t) const {
    if (controlPoints.size() < 2) return glm::dvec2(0, 0);
    
    size_t n = controlPoints.size();
    size_t i = static_cast<size_t>(t * (n - 1));
    i = std::min(i, n - 2);
    
    float localT = (t * (n - 1)) - i;
    
    glm::dvec2 p0 = controlPoints[i];
    glm::dvec2 p1 = controlPoints[i + 1];
    glm::dvec2 p2 = i + 2 < n ? controlPoints[i + 2] : p1;
    glm::dvec2 p3 = i > 0 ? controlPoints[i - 1] : p0;
    
    return catmullRomPoint(p3, p0, p1, p2, localT);
}

glm::dvec2 Spline::calculateDerivative(float t) const {
    if (controlPoints.size() < 2) return glm::dvec2(0, 0);
    
    const float h = 0.0001f;
    glm::dvec2 p1 = calculatePoint(t - h);
    glm::dvec2 p2 = calculatePoint(t + h);
    
    return glm::dvec2((p2.x - p1.x) / (2 * h), (p2.y - p1.y) / (2 * h));
}

glm::dvec2 Spline::calculateSecondDerivative(float t) const {
    if (controlPoints.size() < 2) return glm::dvec2(0, 0);
    
    const float h = 0.0001f;
    glm::dvec2 p1 = calculatePoint(t - h);
    glm::dvec2 p2 = calculatePoint(t);
    glm::dvec2 p3 = calculatePoint(t + h);
    
    return glm::dvec2(
        (p3.x - 2 * p2.x + p1.x) / (h * h),
        (p3.y - 2 * p2.y + p1.y) / (h * h)
    );
}

glm::dvec2 Spline::catmullRomPoint(const glm::dvec2& p0, const glm::dvec2& p1, 
                              const glm::dvec2& p2, const glm::dvec2& p3, float t) const {
    float t2 = t * t;
    float t3 = t2 * t;
    
    float alpha = (1.0f - tension) * 0.5f;
    
    glm::dvec2 result;
    result.x = 0.5f * (
        (2.0f * p1.x) +
        (-p0.x + p2.x) * alpha * t +
        (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * alpha * t2 +
        (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * alpha * t3
    );
    result.y = 0.5f * (
        (2.0f * p1.y) +
        (-p0.y + p2.y) * alpha * t +
        (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * alpha * t2 +
        (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * alpha * t3
    );
    return result;
}

// New Bezier methods
void BezierCurve::decompose() {
    if (!isValid()) return;
    
    // Split curve at t = 0.5
    glm::dvec2 p0 = controlPoints[0];
    glm::dvec2 p1 = controlPoints[1];
    glm::dvec2 p2 = controlPoints[2];
    glm::dvec2 p3 = controlPoints[3];
    
    // De Casteljau's algorithm
    glm::dvec2 p01 = Math::calculateMidpoint(p0, p1);
    glm::dvec2 p12 = Math::calculateMidpoint(p1, p2);
    glm::dvec2 p23 = Math::calculateMidpoint(p2, p3);
    
    glm::dvec2 p012 = Math::calculateMidpoint(p01, p12);
    glm::dvec2 p123 = Math::calculateMidpoint(p12, p23);
    
    glm::dvec2 p0123 = Math::calculateMidpoint(p012, p123);
    
    // Update control points to first half of curve
    controlPoints = {p0, p01, p012, p0123};
}

void BezierCurve::approximate(float tolerance) {
    if (!isValid()) return;
    
    std::vector<glm::dvec2> points = calculatePoints(0.01f);
    std::vector<glm::dvec2> simplified;
    simplified.push_back(points.front());
    
    size_t start = 0;
    while (start < points.size()) {
        size_t end = start + 1;
        while (end < points.size()) {
            // Check if line approximation is good enough
            bool canApproximate = true;
            for (size_t i = start + 1; i < end; ++i) {
                float dist = Math::calculateDistanceToLine(points[i], points[start], points[end]);
                if (dist > tolerance) {
                    canApproximate = false;
                    break;
                }
            }
            
            if (!canApproximate) break;
            end++;
        }
        
        if (end >= points.size()) break;
        simplified.push_back(points[end - 1]);
        start = end - 1;
    }
    
    simplified.push_back(points.back());
    
    // Convert back to Bezier control points
    fitToCurve(simplified);
}

float BezierCurve::calculateLength() const {
    if (!isValid()) return 0.0f;
    
    // Gaussian quadrature for accurate length calculation
    const float w[] = {0.2955242247147529f, 0.2955242247147529f,
                      0.2692667193099963f, 0.2692667193099963f,
                      0.2190863625159820f, 0.2190863625159820f,
                      0.1494513491505806f, 0.1494513491505806f,
                      0.0666713443086881f, 0.0666713443086881f};
    const float x[] = {-0.1488743389816312f, 0.1488743389816312f,
                      -0.4333953941292472f, 0.4333953941292472f,
                      -0.6794095682990244f, 0.6794095682990244f,
                      -0.8650633666889845f, 0.8650633666889845f,
                      -0.9739065285171717f, 0.9739065285171717f};
    
    float length = 0.0f;
    for (int i = 0; i < 10; ++i) {
        float t = (x[i] + 1.0f) * 0.5f;
        glm::dvec2 derivative = calculateDerivative(t);
        float speed = std::sqrt(derivative.x * derivative.x + derivative.y * derivative.y);
        length += w[i] * speed;
    }
    
    return length * 0.5f;
}

std::vector<glm::dvec2> BezierCurve::findExtrema() const {
    if (!isValid()) return {};
    
    std::vector<glm::dvec2> extrema;
    
    // Find roots of derivative
    // Calculate coefficients for derivative polynomial
    glm::dvec2 p0 = controlPoints[0];
    glm::dvec2 p1 = controlPoints[1];
    glm::dvec2 p2 = controlPoints[2];
    glm::dvec2 p3 = controlPoints[3];
    
    // For cubic Bezier, derivative coefficients are:
    // a = 3(p3 - 3p2 + 3p1 - p0)
    // b = 2(3p2 - 6p1 + 3p0)
    // c = 3(p1 - p0)
    glm::dvec2 a = 3.0f * (p3 - 3.0f * p2 + 3.0f * p1 - p0);
    glm::dvec2 b = 6.0f * (p2 - 2.0f * p1 + p0);
    glm::dvec2 c = 3.0f * (p1 - p0);
    
    // Solve quadratic equation for x and y components
    auto solveQuadratic = [](float a, float b, float c) -> std::vector<float> {
        std::vector<float> roots;
        if (std::abs(a) < 0.0001f) {
            if (std::abs(b) > 0.0001f) {
                float t = -c / b;
                if (t >= 0.0f && t <= 1.0f) roots.push_back(t);
            }
            return roots;
        }
        
        float discriminant = b * b - 4.0f * a * c;
        if (discriminant < 0.0f) return roots;
        
        float sqrtDisc = std::sqrt(discriminant);
        float t1 = (-b + sqrtDisc) / (2.0f * a);
        float t2 = (-b - sqrtDisc) / (2.0f * a);
        
        if (t1 >= 0.0f && t1 <= 1.0f) roots.push_back(t1);
        if (t2 >= 0.0f && t2 <= 1.0f) roots.push_back(t2);
        return roots;
    };
    
    auto xRoots = solveQuadratic(a.x, b.x, c.x);
    auto yRoots = solveQuadratic(a.y, b.y, c.y);
    
    // Add extrema points
    for (float t : xRoots) extrema.push_back(calculatePoint(t));
    for (float t : yRoots) extrema.push_back(calculatePoint(t));
    
    return extrema;
}

std::vector<glm::dvec2> BezierCurve::findInflections() const {
    if (!isValid()) return {};
    
    std::vector<glm::dvec2> inflections;
    
    // Calculate second derivative coefficients
    glm::dvec2 p0 = controlPoints[0];
    glm::dvec2 p1 = controlPoints[1];
    glm::dvec2 p2 = controlPoints[2];
    glm::dvec2 p3 = controlPoints[3];
    
    // For cubic Bezier, second derivative coefficients are:
    // a = 6(p3 - 3p2 + 3p1 - p0)
    // b = 6(p2 - 2p1 + p0)
    glm::dvec2 a = 6.0f * (p3 - 3.0f * p2 + 3.0f * p1 - p0);
    glm::dvec2 b = 6.0f * (p2 - 2.0f * p1 + p0);
    
    // Find points where curvature changes sign
    for (float t = 0.0f; t <= 1.0f; t += 0.01f) {
        float curvature = calculateCurvature(t);
        float nextCurvature = calculateCurvature(t + 0.01f);
        
        if (curvature * nextCurvature < 0.0f) {
            inflections.push_back(calculatePoint(t));
        }
    }
    
    return inflections;
}

glm::dvec2 BezierCurve::findNearestPoint(const glm::dvec2& point) const {
    if (!isValid()) return point;
    
    float minDist = std::numeric_limits<float>::max();
    glm::dvec2 nearest = point;
    
    // Binary search for closest point
    const int MAX_ITERATIONS = 20;
    float tLeft = 0.0f;
    float tRight = 1.0f;
    
    for (int i = 0; i < MAX_ITERATIONS; ++i) {
        float t1 = tLeft + (tRight - tLeft) / 3.0f;
        float t2 = tLeft + 2.0f * (tRight - tLeft) / 3.0f;
        
        glm::dvec2 p1 = calculatePoint(t1);
        glm::dvec2 p2 = calculatePoint(t2);
        
        float dist1 = Math::calculateDistance(point, p1);
        float dist2 = Math::calculateDistance(point, p2);
        
        if (dist1 < minDist) {
            minDist = dist1;
            nearest = p1;
        }
        if (dist2 < minDist) {
            minDist = dist2;
            nearest = p2;
        }
        
        if (dist1 < dist2) {
            tRight = t2;
        } else {
            tLeft = t1;
        }
    }
    
    return nearest;
}

void BezierCurve::generateOffsetCurve(float distance) {
    if (!isValid()) return;
    
    std::vector<glm::dvec2> offsetPoints;
    const int SAMPLES = 100;
    
    for (int i = 0; i <= SAMPLES; ++i) {
        float t = static_cast<float>(i) / SAMPLES;
        glm::dvec2 point = calculatePoint(t);
        glm::dvec2 derivative = calculateDerivative(t);
        
        // Calculate normal vector
        float len = std::sqrt(derivative.x * derivative.x + derivative.y * derivative.y);
        if (len > 0.0001f) {
            glm::dvec2 normal(-derivative.y / len, derivative.x / len);
            offsetPoints.push_back(glm::dvec2(
                point.x + normal.x * distance,
                point.y + normal.y * distance
            ));
        }
    }
    
    // Fit new Bezier curve to offset points
    fitToCurve(offsetPoints);
}

void BezierCurve::fitToCurve(const std::vector<glm::dvec2>& points) {
    if (points.size() < 2) return;
    
    // Simple least squares fitting for cubic Bezier
    glm::dvec2 p0 = points.front();
    glm::dvec2 p3 = points.back();
    
    // Estimate control points using chord length parameterization
    float totalLength = 0.0f;
    std::vector<float> chordLengths;
    chordLengths.push_back(0.0f);
    
    for (size_t i = 1; i < points.size(); ++i) {
        float length = Math::calculateDistance(points[i], points[i-1]);
        totalLength += length;
        chordLengths.push_back(totalLength);
    }
    
    // Normalize chord lengths
    for (float& t : chordLengths) {
        t /= totalLength;
    }
    
    // Estimate control points
    glm::dvec2 p1, p2;
    float alpha = 1.0f / 3.0f;
    
    p1.x = p0.x + (points[points.size()/3].x - p0.x) / alpha;
    p1.y = p0.y + (points[points.size()/3].y - p0.y) / alpha;
    
    p2.x = p3.x - (points[2*points.size()/3].x - p3.x) / alpha;
    p2.y = p3.y - (points[2*points.size()/3].y - p3.y) / alpha;
    
    controlPoints = {p0, p1, p2, p3};
}

// Helper methods for Bezier calculations
glm::dvec2 BezierCurve::calculateDerivative(float t) const {
    if (!isValid()) return glm::dvec2(0, 0);
    
    float mt = 1.0f - t;
    float mt2 = mt * mt;
    float t2 = t * t;
    
    glm::dvec2 result;
    result.x = 3.0f * (
        mt2 * (controlPoints[1].x - controlPoints[0].x) +
        2.0f * mt * t * (controlPoints[2].x - controlPoints[1].x) +
        t2 * (controlPoints[3].x - controlPoints[2].x)
    );
    result.y = 3.0f * (
        mt2 * (controlPoints[1].y - controlPoints[0].y) +
        2.0f * mt * t * (controlPoints[2].y - controlPoints[1].y) +
        t2 * (controlPoints[3].y - controlPoints[2].y)
    );
    
    return result;
}

float BezierCurve::calculateCurvature(float t) const {
    glm::dvec2 d1 = calculateDerivative(t);
    
    // Calculate second derivative
    float mt = 1.0f - t;
    glm::dvec2 d2;
    d2.x = 6.0f * (
        mt * (controlPoints[2].x - 2.0f * controlPoints[1].x + controlPoints[0].x) +
        t * (controlPoints[3].x - 2.0f * controlPoints[2].x + controlPoints[1].x)
    );
    d2.y = 6.0f * (
        mt * (controlPoints[2].y - 2.0f * controlPoints[1].y + controlPoints[0].y) +
        t * (controlPoints[3].y - 2.0f * controlPoints[2].y + controlPoints[1].y)
    );
    
    // Calculate curvature
    float num = d1.x * d2.y - d1.y * d2.x;
    float den = std::pow(d1.x * d1.x + d1.y * d1.y, 1.5f);
    
    return den > 0.0001f ? num / den : 0.0f;
}

glm::dvec2 BezierCurve::calculatePoint(float t) const {
    if (!isValid()) return glm::dvec2(0, 0);
    
    float mt = 1.0f - t;
    float mt2 = mt * mt;
    float mt3 = mt2 * mt;
    float t2 = t * t;
    float t3 = t2 * t;
    
    glm::dvec2 result;
    result.x = mt3 * controlPoints[0].x +
               3.0f * mt2 * t * controlPoints[1].x +
               3.0f * mt * t2 * controlPoints[2].x +
               t3 * controlPoints[3].x;
               
    result.y = mt3 * controlPoints[0].y +
               3.0f * mt2 * t * controlPoints[1].y +
               3.0f * mt * t2 * controlPoints[2].y +
               t3 * controlPoints[3].y;
               
    return result;
}

std::vector<glm::dvec2> BezierCurve::calculatePoints(float step) const {
    if (!isValid()) return {};
    
    std::vector<glm::dvec2> points;
    if (adaptiveRendering) {
        // Use adaptive sampling based on curvature
        float t = 0.0f;
        while (t <= 1.0f) {
            points.push_back(calculatePoint(t));
            
            // Adjust step size based on curvature
            float curvature = std::abs(calculateCurvature(t));
            float adaptiveStep = step / (1.0f + curvature * 10.0f);
            t += std::max(adaptiveStep, step * 0.1f); // Don't let step get too small
        }
        points.push_back(calculatePoint(1.0f)); // Ensure end point is included
    } else {
        // Use uniform sampling
        for (float t = 0.0f; t <= 1.0f; t += step) {
            points.push_back(calculatePoint(t));
        }
        points.push_back(calculatePoint(1.0f)); // Ensure end point is included
    }
    
    return points;
}

} // namespace Drawing 