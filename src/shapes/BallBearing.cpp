#include <glm/glm.hpp>
#include "shapes/ComplexShapes.hpp"
#include "Canvas.hpp"
#include <cmath>
#include <sstream>
#include <iomanip>

namespace Drawing {

bool BallBearing::isPointNear(const glm::dvec2& point, float threshold) const {
    // Transform point relative to bearing position
    glm::dvec2 localPoint = glm::dvec2(point.x - position.x, point.y - position.y);
    
    // Rotate point by negative bearing angle
    float s = sin(-angle);
    float c = cos(-angle);
    glm::dvec2 rotatedPoint = glm::dvec2(
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

std::vector<glm::dvec2> BallBearing::generateProfile() const {
    std::vector<glm::dvec2> points;
    
    float outerRadius = outerDiameter / 2.0f;
    float innerRadius = innerDiameter / 2.0f;
    float halfWidth = width / 2.0f;
    
    // Generate outer race profile (top view - cross section)
    // Outer circle
    int segments = 64;
    for (int i = 0; i <= segments; ++i) {
        float angle = (float)i / segments * 2.0f * M_PI;
        points.push_back(glm::dvec2(
            outerRadius * cos(angle),
            outerRadius * sin(angle)
        ));
    }
    
    // Inner circle (reverse direction to create hole)
    for (int i = segments; i >= 0; --i) {
        float angle = (float)i / segments * 2.0f * M_PI;
        points.push_back(glm::dvec2(
            innerRadius * cos(angle),
            innerRadius * sin(angle)
        ));
    }
    
    return points;
}

std::vector<glm::dvec2> BallBearing::generateBallPositions() const {
    std::vector<glm::dvec2> ballPositions;
    
    float pitchRadius = calculatePitchCircleDiameter() / 2.0f;
    
    for (int i = 0; i < numBalls; ++i) {
        float ballAngle = (float)i / numBalls * 2.0f * M_PI;
        ballPositions.push_back(glm::dvec2(
            pitchRadius * cos(ballAngle),
            pitchRadius * sin(ballAngle)
        ));
    }
    
    return ballPositions;
}

std::vector<std::pair<glm::dvec2, glm::dvec2>> BallBearing::generateDimensionLines() const {
    std::vector<std::pair<glm::dvec2, glm::dvec2>> dimensions;
    
    float outerRadius = outerDiameter / 2.0f;
    float innerRadius = innerDiameter / 2.0f;
    float pitchRadius = calculatePitchCircleDiameter() / 2.0f;
    
    // Outer diameter dimension (horizontal)
    dimensions.push_back({
        glm::dvec2(-outerRadius, outerRadius + 15.0f),
        glm::dvec2(outerRadius, outerRadius + 15.0f)
    });
    
    // Inner diameter dimension (horizontal)
    dimensions.push_back({
        glm::dvec2(-innerRadius, -outerRadius - 15.0f),
        glm::dvec2(innerRadius, -outerRadius - 15.0f)
    });
    
    // Pitch circle diameter dimension (vertical)
    dimensions.push_back({
        glm::dvec2(-outerRadius - 15.0f, -pitchRadius),
        glm::dvec2(-outerRadius - 15.0f, pitchRadius)
    });
    
    // Ball diameter dimension (if balls are shown)
    if (showBalls && numBalls > 0) {
        float ballRadius = ballDiameter / 2.0f;
        float firstBallAngle = 0.0f;
        glm::dvec2 ballCenter = glm::dvec2(pitchRadius, 0.0f);
        
        dimensions.push_back({
            glm::dvec2(ballCenter.x - ballRadius, ballCenter.y + ballRadius + 10.0f),
            glm::dvec2(ballCenter.x + ballRadius, ballCenter.y + ballRadius + 10.0f)
        });
    }
    
    return dimensions;
}

} // namespace Drawing
