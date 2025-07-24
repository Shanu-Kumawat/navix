#include "shapes/ComplexShapes.hpp"
#include "Canvas.hpp"
#include <cmath>
#include <sstream>
#include <iomanip>

namespace Drawing {

bool BallBearing::isPointNear(const ImVec2& point, float threshold) const {
    // Transform point relative to bearing position
    ImVec2 localPoint = ImVec2(point.x - position.x, point.y - position.y);
    
    // Rotate point by negative bearing angle
    float s = sin(-angle);
    float c = cos(-angle);
    ImVec2 rotatedPoint = ImVec2(
        localPoint.x * c - localPoint.y * s,
        localPoint.x * s + localPoint.y * c
    );
    
    // Check if point is near the bearing outline
    float distanceFromCenter = sqrt(rotatedPoint.x * rotatedPoint.x + rotatedPoint.y * rotatedPoint.y);
    float outerRadius = outerDiameter / 2.0f;
    float innerRadius = innerDiameter / 2.0f;
    
    // Check if point is within the bearing ring area with threshold
    return (distanceFromCenter <= outerRadius + threshold && 
            distanceFromCenter >= innerRadius - threshold);
}

std::vector<ImVec2> BallBearing::generateProfile() const {
    std::vector<ImVec2> points;
    
    float outerRadius = outerDiameter / 2.0f;
    float innerRadius = innerDiameter / 2.0f;
    float halfWidth = width / 2.0f;
    
    // Generate outer race profile (top view - cross section)
    // Outer circle
    int segments = 64;
    for (int i = 0; i <= segments; ++i) {
        float angle = (float)i / segments * 2.0f * M_PI;
        points.push_back(ImVec2(
            outerRadius * cos(angle),
            outerRadius * sin(angle)
        ));
    }
    
    // Inner circle (reverse direction to create hole)
    for (int i = segments; i >= 0; --i) {
        float angle = (float)i / segments * 2.0f * M_PI;
        points.push_back(ImVec2(
            innerRadius * cos(angle),
            innerRadius * sin(angle)
        ));
    }
    
    return points;
}

std::vector<ImVec2> BallBearing::generateBallPositions() const {
    std::vector<ImVec2> ballPositions;
    
    float pitchRadius = calculatePitchCircleDiameter() / 2.0f;
    
    for (int i = 0; i < numBalls; ++i) {
        float ballAngle = (float)i / numBalls * 2.0f * M_PI;
        ballPositions.push_back(ImVec2(
            pitchRadius * cos(ballAngle),
            pitchRadius * sin(ballAngle)
        ));
    }
    
    return ballPositions;
}

std::vector<std::pair<ImVec2, ImVec2>> BallBearing::generateDimensionLines() const {
    std::vector<std::pair<ImVec2, ImVec2>> dimensions;
    
    float outerRadius = outerDiameter / 2.0f;
    float innerRadius = innerDiameter / 2.0f;
    float pitchRadius = calculatePitchCircleDiameter() / 2.0f;
    
    // Outer diameter dimension (horizontal)
    dimensions.push_back({
        ImVec2(-outerRadius, outerRadius + 15.0f),
        ImVec2(outerRadius, outerRadius + 15.0f)
    });
    
    // Inner diameter dimension (horizontal)
    dimensions.push_back({
        ImVec2(-innerRadius, -outerRadius - 15.0f),
        ImVec2(innerRadius, -outerRadius - 15.0f)
    });
    
    // Pitch circle diameter dimension (vertical)
    dimensions.push_back({
        ImVec2(-outerRadius - 15.0f, -pitchRadius),
        ImVec2(-outerRadius - 15.0f, pitchRadius)
    });
    
    // Ball diameter dimension (if balls are shown)
    if (showBalls && numBalls > 0) {
        float ballRadius = ballDiameter / 2.0f;
        float firstBallAngle = 0.0f;
        ImVec2 ballCenter = ImVec2(pitchRadius, 0.0f);
        
        dimensions.push_back({
            ImVec2(ballCenter.x - ballRadius, ballCenter.y + ballRadius + 10.0f),
            ImVec2(ballCenter.x + ballRadius, ballCenter.y + ballRadius + 10.0f)
        });
    }
    
    return dimensions;
}

} // namespace Drawing
