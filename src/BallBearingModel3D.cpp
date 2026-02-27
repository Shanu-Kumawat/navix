#include <glm/glm.hpp>
#include "BallBearingModel3D.hpp"
#include <iostream>
#include <cmath>

BallBearingModel3D::BallBearingModel3D() : Base3DModel(), currentBallBearing(nullptr) {
    // Initialize ball bearing-specific default material properties (metallic)
    objectColor = glm::vec3(0.9f, 0.9f, 0.95f);
    ambientStrength = 0.2f;
    diffuseStrength = 0.8f;
    specularStrength = 0.9f;
    shininess = 128.0f;
}

void BallBearingModel3D::generateMesh() {
    if (currentBallBearing) {
        generateMesh(currentBallBearing);
    }
}

void BallBearingModel3D::generateMesh(const Drawing::BallBearing* ballBearing) {
    if (!ballBearing) return;
    
    // Store reference for potential regeneration
    currentBallBearing = ballBearing;
    
    // Clear existing mesh data
    clearMesh();
    
    // Generate ball bearing-specific geometry
    generateRaceGeometry(ballBearing);
    
    // Generate ball geometry if enabled
    if (ballBearing->showBalls) {
        std::vector<glm::dvec2> ballPositions = ballBearing->generateBallPositions();
        if (!ballPositions.empty()) {
            // Normalize ball size to work with our scaled coordinate system
            float outerRadius = ballBearing->outerDiameter / 2.0f;
            float maxDimension = outerRadius;
            float scale = 1.0f / maxDimension;
            float ballRadius = ballBearing->ballDiameter / 2.0f * scale * 1.5f; // Make balls 50% larger for visibility
            
            generateBallGeometry(ballPositions, ballRadius, scale);
        }
    }
    
    // Calculate normals
    generateNormals();
    
    // Setup OpenGL buffers
    setupMesh();
    
    // Reset view to default
    resetView();
}

void BallBearingModel3D::generateRaceGeometry(const Drawing::BallBearing* ballBearing) {
    // Get parameters and normalize to reasonable scale
    float outerRadius = ballBearing->outerDiameter / 2.0f;
    float innerRadius = ballBearing->innerDiameter / 2.0f;
    float width = ballBearing->width;
    
    // Normalize to [-1, 1] range for consistent OpenGL rendering
    float maxDimension = outerRadius;
    float scale = 1.0f / maxDimension;
    
    // Generate race geometry using profile revolution (higher quality)
    generateSeparateRaces(innerRadius * scale, outerRadius * scale, width * scale, 72);
}

void BallBearingModel3D::generateSeparateRaces(float innerRadius, float outerRadius, float width, int segments) {
    float halfWidth = width / 2.0f;
    
    // Calculate race dimensions
    float raceWidth = (outerRadius - innerRadius) * 0.18f;  // 18% of gap for race thickness
    float innerRaceOuter = innerRadius + raceWidth;
    float outerRaceInner = outerRadius - raceWidth;
    
    // Race height and groove details
    float raceHeight = halfWidth * 0.7f;  // Race height is 70% of total width
    float grooveDepth = raceWidth * 0.15f; // Groove depth for ball contact
    
    // Generate INNER RACE profile (hollow design)
    std::vector<glm::dvec2> innerRaceProfile;
    
    // Inner race with ball groove - create a proper ring shape
    innerRaceProfile.push_back(glm::dvec2(innerRadius, -raceHeight));        // Inner bottom
    innerRaceProfile.push_back(glm::dvec2(innerRaceOuter - grooveDepth, -raceHeight)); // Inner edge bottom
    
    // Inner race ball groove (curved)
    float innerGrooveMidRadius = (innerRadius + innerRaceOuter) / 2.0f;
    innerRaceProfile.push_back(glm::dvec2(innerGrooveMidRadius, -raceHeight * 0.8f)); // Groove bottom
    innerRaceProfile.push_back(glm::dvec2(innerGrooveMidRadius, 0.0f));               // Groove center  
    innerRaceProfile.push_back(glm::dvec2(innerGrooveMidRadius, raceHeight * 0.8f));  // Groove top
    
    innerRaceProfile.push_back(glm::dvec2(innerRaceOuter - grooveDepth, raceHeight)); // Inner edge top
    innerRaceProfile.push_back(glm::dvec2(innerRadius, raceHeight));          // Inner top
    
    // Generate inner race geometry
    revolveProfile(innerRaceProfile, segments);
    
    // Generate OUTER RACE profile (hollow design)
    std::vector<glm::dvec2> outerRaceProfile;
    
    // Outer race with ball groove
    outerRaceProfile.push_back(glm::dvec2(outerRaceInner + grooveDepth, -raceHeight)); // Inner edge bottom
    
    // Outer race ball groove (curved)
    float outerGrooveMidRadius = (outerRaceInner + outerRadius) / 2.0f;
    outerRaceProfile.push_back(glm::dvec2(outerGrooveMidRadius, -raceHeight * 0.8f)); // Groove bottom
    outerRaceProfile.push_back(glm::dvec2(outerGrooveMidRadius, 0.0f));               // Groove center
    outerRaceProfile.push_back(glm::dvec2(outerGrooveMidRadius, raceHeight * 0.8f));  // Groove top
    
    outerRaceProfile.push_back(glm::dvec2(outerRaceInner + grooveDepth, raceHeight)); // Inner edge top
    outerRaceProfile.push_back(glm::dvec2(outerRadius, raceHeight));       // Outer top
    outerRaceProfile.push_back(glm::dvec2(outerRadius, -raceHeight));      // Outer bottom
    
    // Close the loop back to start
    outerRaceProfile.push_back(glm::dvec2(outerRaceInner + grooveDepth, -raceHeight)); // Back to start
    
    // Generate outer race geometry  
    revolveProfile(outerRaceProfile, segments);
}

void BallBearingModel3D::generateBallGeometry(const std::vector<glm::dvec2>& ballPositions, float ballRadius, float scale) {
    if (ballPositions.empty()) return;
    
    unsigned int baseIndex = vertices.size() / 6; // 6 floats per vertex (pos + normal)
    int ballSegments = 20; // High quality spheres
    
    for (const auto& ballPos : ballPositions) {
        // Ball positions are already in world coordinates, so scale them to match the race scaling
        float ballX = ballPos.x * scale;
        float ballZ = ballPos.y * scale;
        
        // Generate a high-quality sphere for each ball
        for (int i = 0; i <= ballSegments; ++i) {
            float phi = (float)i / ballSegments * M_PI; // 0 to PI (latitude)
            
            for (int j = 0; j <= ballSegments; ++j) {
                float theta = (float)j / ballSegments * 2.0f * M_PI; // 0 to 2*PI (longitude)
                
                // Spherical coordinates to Cartesian
                float x = ballX + ballRadius * sin(phi) * cos(theta);
                float y = ballRadius * cos(phi) * 0.9f; // Slightly compressed to fit in groove
                float z = ballZ + ballRadius * sin(phi) * sin(theta);
                
                // Add vertex (position + placeholder for normal)
                vertices.push_back(x);
                vertices.push_back(y);
                vertices.push_back(z);
                
                // Add placeholder for normal vector (calculated later)
                vertices.push_back(0.0f);
                vertices.push_back(0.0f);
                vertices.push_back(0.0f);
            }
        }
        
        // Generate indices for this ball (sphere)
        for (int i = 0; i < ballSegments; ++i) {
            for (int j = 0; j < ballSegments; ++j) {
                unsigned int first = baseIndex + i * (ballSegments + 1) + j;
                unsigned int second = baseIndex + (i + 1) * (ballSegments + 1) + j;
                
                // Create two triangles for each quad
                indices.push_back(first);
                indices.push_back(second);
                indices.push_back(first + 1);
                
                indices.push_back(second);
                indices.push_back(second + 1);
                indices.push_back(first + 1);
            }
        }
        
        baseIndex += (ballSegments + 1) * (ballSegments + 1);
    }
}
