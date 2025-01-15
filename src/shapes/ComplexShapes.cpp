#include "shapes/ComplexShapes.hpp"

namespace Drawing {

std::vector<ImVec2> Spline::calculatePoints(float resolution) const {
    if (!isValid()) return {};

    std::vector<ImVec2> points;
    const size_t numPoints = static_cast<size_t>(1.0f / resolution) + 1;
    points.reserve(numPoints);

    for (float t = 0; t <= 1.0f; t += resolution) {
        // Catmull-Rom spline calculation
        size_t n = controlPoints.size();
        if (isClosed) {
            // Handle closed spline
            for (size_t i = 0; i < n; i++) {
                size_t p0 = (i - 1 + n) % n;
                size_t p1 = i;
                size_t p2 = (i + 1) % n;
                size_t p3 = (i + 2) % n;

                float t2 = t * t;
                float t3 = t2 * t;

                float x = 0.5f * ((2.0f * controlPoints[p1].x) +
                    (-controlPoints[p0].x + controlPoints[p2].x) * t +
                    (2.0f * controlPoints[p0].x - 5.0f * controlPoints[p1].x + 4.0f * controlPoints[p2].x - controlPoints[p3].x) * t2 +
                    (-controlPoints[p0].x + 3.0f * controlPoints[p1].x - 3.0f * controlPoints[p2].x + controlPoints[p3].x) * t3);

                float y = 0.5f * ((2.0f * controlPoints[p1].y) +
                    (-controlPoints[p0].y + controlPoints[p2].y) * t +
                    (2.0f * controlPoints[p0].y - 5.0f * controlPoints[p1].y + 4.0f * controlPoints[p2].y - controlPoints[p3].y) * t2 +
                    (-controlPoints[p0].y + 3.0f * controlPoints[p1].y - 3.0f * controlPoints[p2].y + controlPoints[p3].y) * t3);

                points.push_back(ImVec2(x, y));
            }
        } else {
            // Handle open spline
            for (size_t i = 1; i < n - 2; i++) {
                float t2 = t * t;
                float t3 = t2 * t;

                float x = 0.5f * ((2.0f * controlPoints[i].x) +
                    (-controlPoints[i-1].x + controlPoints[i+1].x) * t +
                    (2.0f * controlPoints[i-1].x - 5.0f * controlPoints[i].x + 4.0f * controlPoints[i+1].x - controlPoints[i+2].x) * t2 +
                    (-controlPoints[i-1].x + 3.0f * controlPoints[i].x - 3.0f * controlPoints[i+1].x + controlPoints[i+2].x) * t3);

                float y = 0.5f * ((2.0f * controlPoints[i].y) +
                    (-controlPoints[i-1].y + controlPoints[i+1].y) * t +
                    (2.0f * controlPoints[i-1].y - 5.0f * controlPoints[i].y + 4.0f * controlPoints[i+1].y - controlPoints[i+2].y) * t2 +
                    (-controlPoints[i-1].y + 3.0f * controlPoints[i].y - 3.0f * controlPoints[i+1].y + controlPoints[i+2].y) * t3);

                points.push_back(ImVec2(x, y));
            }
        }
    }

    return points;
}

ImVec2 BezierCurve::calculatePoint(float t) const {
    if (!isValid()) return ImVec2(0, 0);

    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    ImVec2 point = ImVec2(
        uuu * controlPoints[0].x +
        3 * uu * t * controlPoints[1].x +
        3 * u * tt * controlPoints[2].x +
        ttt * controlPoints[3].x,
        
        uuu * controlPoints[0].y +
        3 * uu * t * controlPoints[1].y +
        3 * u * tt * controlPoints[2].y +
        ttt * controlPoints[3].y
    );

    return point;
}

std::vector<ImVec2> BezierCurve::calculatePoints(float resolution) const {
    if (!isValid()) return {};

    std::vector<ImVec2> points;
    const size_t numPoints = static_cast<size_t>((endT - startT) / resolution) + 1;
    points.reserve(numPoints);

    for (float t = startT; t <= endT; t += resolution) {
        points.push_back(calculatePoint(t));
    }

    // Ensure the end point is included
    if (!points.empty() && endT > startT) {
        points.push_back(calculatePoint(endT));
    }

    return points;
}

} // namespace Drawing 