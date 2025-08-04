#include "BellowsModel3D.hpp"
#include <iostream>

BellowsModel3D::BellowsModel3D() : Base3DModel(), currentBellows(nullptr) {
    // Initialize bellows-specific default material properties
    objectColor = glm::vec3(0.8f, 0.8f, 0.8f);
    ambientStrength = 0.4f;
    diffuseStrength = 0.7f;
    specularStrength = 0.7f;
    shininess = 32.0f;
}

void BellowsModel3D::generateMesh() {
    if (currentBellows) {
        generateMesh(currentBellows);
    }
}

void BellowsModel3D::generateMesh(const Drawing::Bellows* bellows) {
    if (!bellows) return;
    
    // Store reference for potential regeneration
    currentBellows = bellows;
    
    // Clear existing mesh data
    clearMesh();
    
    // Generate bellows-specific geometry
    generateBellowsGeometry(bellows);
    
    // Calculate normals
    generateNormals();
    
    // Setup OpenGL buffers
    setupMesh();
    
    // Reset view to default
    resetView();
}

void BellowsModel3D::generateBellowsGeometry(const Drawing::Bellows* bellows) {
    // Get bellows profile using cached version
    const std::vector<ImVec2>& profile = bellows->getCachedProfile();
    if (profile.empty()) return;
    
    // Normalize profile to fit in [-1, 1] range for consistent rendering
    float maxRadius = 0.0f;
    float maxHeight = 0.0f;
    
    for (const auto& point : profile) {
        maxRadius = std::max(maxRadius, std::abs(point.x));
        maxHeight = std::max(maxHeight, std::abs(point.y));
    }
    
    float maxDimension = std::max(maxRadius, maxHeight);
    float scale = 1.0f / (maxDimension > 0 ? maxDimension : 1.0f);
    
    // Scale profile points
    std::vector<ImVec2> scaledProfile;
    for (const auto& point : profile) {
        scaledProfile.push_back(ImVec2(point.x * scale, point.y * scale));
    }
    
    // Revolve profile around Y-axis to create 3D geometry
    // Use higher segment count for bellows (72 segments for smooth curves)
    revolveProfile(scaledProfile, 72);
}
