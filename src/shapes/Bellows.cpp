#include <glm/glm.hpp>
#include "shapes/ComplexShapes.hpp"
#include "Canvas.hpp"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace Drawing {

bool Bellows::isPointNear(const glm::dvec2& point, float threshold) const {
    // Get the cached profile instead of regenerating
    const std::vector<glm::dvec2>& profile = getCachedProfile();
    
    // Calculate sin and cos for rotation
    float s = sin(angle);
    float c = cos(angle);
    
    // Transform all profile points to account for position and rotation
    std::vector<glm::dvec2> transformedProfile;
    transformedProfile.reserve(profile.size());
    
    for (const auto& p : profile) {
        // Rotate and translate
        float rotatedX = p.x * c - p.y * s;
        float rotatedY = p.x * s + p.y * c;
        
        transformedProfile.push_back(glm::dvec2(
            position.x + rotatedX,
            position.y + rotatedY
        ));
    }
    
    // Use a much larger threshold for bellows to make them easier to select
    float adjustedThreshold = threshold * 10.0f;
    
    // Check if point is near any segment of the transformed profile
    for (size_t i = 0; i < transformedProfile.size() - 1; ++i) {
        const glm::dvec2& p1 = transformedProfile[i];
        const glm::dvec2& p2 = transformedProfile[i + 1];
        
        float lineLength = std::sqrt(
            (p2.x - p1.x) * (p2.x - p1.x) + 
            (p2.y - p1.y) * (p2.y - p1.y)
        );
        
        if (lineLength < 0.0001f) continue;
        
        float t = ((point.x - p1.x) * (p2.x - p1.x) + 
                  (point.y - p1.y) * (p2.y - p1.y)) / (lineLength * lineLength);
        
        if (t >= 0.0f && t <= 1.0f) {
            float closestX = p1.x + t * (p2.x - p1.x);
            float closestY = p1.y + t * (p2.y - p1.y);
            
            float distance = std::sqrt(
                (point.x - closestX) * (point.x - closestX) + 
                (point.y - closestY) * (point.y - closestY)
            );
            
            if (distance <= adjustedThreshold) return true;
        }
    }
    
    // Also check if the point is inside the bellows profile (for easier selection)
    bool inside = false;
    for (size_t i = 0, j = transformedProfile.size() - 1; i < transformedProfile.size(); j = i++) {
        if (((transformedProfile[i].y > point.y) != (transformedProfile[j].y > point.y)) &&
            (point.x < (transformedProfile[j].x - transformedProfile[i].x) * (point.y - transformedProfile[i].y) / 
             (transformedProfile[j].y - transformedProfile[i].y) + transformedProfile[i].x)) {
            inside = !inside;
        }
    }
    
    return inside;
}

std::vector<glm::dvec2> Bellows::generateProfile() const {
    std::vector<glm::dvec2> points;
    
    // Calculate derived parameters
    float convolutionDistance = convolutedSectionLength / static_cast<float>(numConvolutions);
    float convolutionRadius = (peakConvolutionDiameter - baseConvolutionDiameter) / 4.0f;
    
    // Starting position (left side of bellows)
    float x = 0.0f;
    
    // Generate inner profile (bottom side first, left to right)
    
    // First cuff (A) - inner surface
    float innerYA = cuffAInnerDiameter / 2.0f;
    points.push_back(glm::dvec2(x, innerYA));
    x += cuffALength;
    points.push_back(glm::dvec2(x, innerYA));
    
    // Convoluted section - inner surface
    float valleyY = baseConvolutionDiameter / 2.0f;
    float peakY = peakConvolutionDiameter / 2.0f - wallThickness;
    
    for (int i = 0; i < numConvolutions; ++i) {
        // Generate valley to peak curve (quarter circle)
        for (int j = 0; j <= 10; ++j) {
            float angle = static_cast<float>(j) / 10.0f * M_PI / 2.0f;
            float dx = convolutionRadius * (1.0f - std::cos(angle));
            float dy = convolutionRadius * std::sin(angle);
            points.push_back(glm::dvec2(x + dx, valleyY + dy));
        }
        
        x += convolutionRadius;
        
        // Generate peak
        points.push_back(glm::dvec2(x, peakY));
        x += convolutionDistance - 2 * convolutionRadius;
        points.push_back(glm::dvec2(x, peakY));
        
        // Generate peak to valley curve (quarter circle)
        for (int j = 0; j <= 10; ++j) {
            float angle = static_cast<float>(j) / 10.0f * M_PI / 2.0f;
            float dx = convolutionRadius * std::sin(angle);
            float dy = convolutionRadius * (1.0f - std::cos(angle));
            points.push_back(glm::dvec2(x + dx, peakY - dy));
        }
        
        x += convolutionRadius;
    }
    
    // Second cuff (B) - inner surface
    float innerYB = cuffBInnerDiameter / 2.0f;
    points.push_back(glm::dvec2(x, innerYB));
    x += cuffBLength;
    points.push_back(glm::dvec2(x, innerYB));
    
    // Total length points (for reference)
    float totalLength = x;
    
    // Generate outer profile (top side, right to left)
    // Second cuff (B) - outer surface
    points.push_back(glm::dvec2(x, innerYB + wallThickness));
    x -= cuffBLength;
    points.push_back(glm::dvec2(x, innerYB + wallThickness));
    
    // Convoluted section - outer surface (right to left)
    for (int i = numConvolutions - 1; i >= 0; --i) {
        // Determine position for this convolution
        float convStartX = cuffALength + i * convolutionDistance;
        
        // Generate valley to peak curve (quarter circle)
        x = convStartX + convolutionDistance - convolutionRadius;
        for (int j = 10; j >= 0; --j) {
            float angle = static_cast<float>(j) / 10.0f * M_PI / 2.0f;
            float dx = convolutionRadius * std::sin(angle);
            float dy = convolutionRadius * (1.0f - std::cos(angle));
            points.push_back(glm::dvec2(x + dx, valleyY + wallThickness + dy));
        }
        
        // Generate peak
        x = convStartX + convolutionRadius;
        points.push_back(glm::dvec2(x, peakY + 2 * wallThickness));
        x -= convolutionRadius;
        
        // Generate peak to valley curve (quarter circle)
        for (int j = 10; j >= 0; --j) {
            float angle = static_cast<float>(j) / 10.0f * M_PI / 2.0f;
            float dx = convolutionRadius * (1.0f - std::cos(angle));
            float dy = convolutionRadius * std::sin(angle);
            points.push_back(glm::dvec2(x + dx, valleyY + wallThickness + convolutionRadius - dy));
        }
        
        x = convStartX;
    }
    
    // First cuff (A) - outer surface
    points.push_back(glm::dvec2(x, innerYA + wallThickness));
    x -= cuffALength;
    points.push_back(glm::dvec2(x, innerYA + wallThickness));
    
    // Close the profile
    points.push_back(points[0]);
    
    return points;
}

std::vector<std::pair<glm::dvec2, glm::dvec2>> Bellows::generateDimensionLines() const {
    std::vector<std::pair<glm::dvec2, glm::dvec2>> dimensions;
    
    // Calculate total length
    float totalLength = calculateOverallLength();
    
    // Calculate heights
    float innerHeightA = cuffAInnerDiameter;
    float innerHeightB = cuffBInnerDiameter;
    float baseHeight = baseConvolutionDiameter;
    float peakHeight = peakConvolutionDiameter;
    
    // Overall length dimension (below)
    dimensions.push_back({
        glm::dvec2(0, -10), 
        glm::dvec2(totalLength, -10)
    });
    
    // Cuff A length
    dimensions.push_back({
        glm::dvec2(0, -30),
        glm::dvec2(cuffALength, -30)
    });
    
    // Convoluted section length
    dimensions.push_back({
        glm::dvec2(cuffALength, -50),
        glm::dvec2(totalLength - cuffBLength, -50)
    });
    
    // Cuff B length
    dimensions.push_back({
        glm::dvec2(totalLength - cuffBLength, -30),
        glm::dvec2(totalLength, -30)
    });
    
    // Inner diameter dimensions (vertical)
    dimensions.push_back({
        glm::dvec2(-20, -innerHeightA/2),
        glm::dvec2(-20, innerHeightA/2)
    });
    
    dimensions.push_back({
        glm::dvec2(totalLength + 20, -innerHeightB/2),
        glm::dvec2(totalLength + 20, innerHeightB/2)
    });
    
    // Convolution base diameter
    dimensions.push_back({
        glm::dvec2(totalLength/2, -baseHeight/2),
        glm::dvec2(totalLength/2, baseHeight/2)
    });
    
    // Convolution peak diameter
    dimensions.push_back({
        glm::dvec2(totalLength/2 + 20, -peakHeight/2),
        glm::dvec2(totalLength/2 + 20, peakHeight/2)
    });
    
    return dimensions;
}

const std::vector<glm::dvec2>& Bellows::getCachedProfile() const {
    if (!profileCached || !isCacheValid()) {
        // Cache is invalid, regenerate profile
        cachedProfile = generateProfile();
        profileCached = true;
        
        // Update cached parameters
        cachedCuffAInnerDiameter = cuffAInnerDiameter;
        cachedCuffBInnerDiameter = cuffBInnerDiameter;
        cachedCuffALength = cuffALength;
        cachedCuffBLength = cuffBLength;
        cachedBaseConvolutionDiameter = baseConvolutionDiameter;
        cachedPeakConvolutionDiameter = peakConvolutionDiameter;
        cachedConvolutedSectionLength = convolutedSectionLength;
        cachedNumConvolutions = numConvolutions;
        cachedWallThickness = wallThickness;
        cachedPosition = position;
        cachedAngle = angle;
    }
    
    return cachedProfile;
}

bool Bellows::isCacheValid() const {
    const float epsilon = 1e-6f;
    
    return (std::abs(cachedCuffAInnerDiameter - cuffAInnerDiameter) < epsilon &&
            std::abs(cachedCuffBInnerDiameter - cuffBInnerDiameter) < epsilon &&
            std::abs(cachedCuffALength - cuffALength) < epsilon &&
            std::abs(cachedCuffBLength - cuffBLength) < epsilon &&
            std::abs(cachedBaseConvolutionDiameter - baseConvolutionDiameter) < epsilon &&
            std::abs(cachedPeakConvolutionDiameter - peakConvolutionDiameter) < epsilon &&
            std::abs(cachedConvolutedSectionLength - convolutedSectionLength) < epsilon &&
            cachedNumConvolutions == numConvolutions &&
            std::abs(cachedWallThickness - wallThickness) < epsilon &&
            std::abs(cachedPosition.x - position.x) < epsilon &&
            std::abs(cachedPosition.y - position.y) < epsilon &&
            std::abs(cachedAngle - angle) < epsilon);
}

} // namespace Drawing
