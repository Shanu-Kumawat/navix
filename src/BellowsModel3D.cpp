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
    
    // Convert bellows profile to 3D revolve format for horizontal orientation
    // The bellows profile has: x = axial position, y = radius
    // For horizontal bellows: x = axial position (along X-axis), y = radius (for revolution around X-axis)
    std::vector<ImVec2> bellowsProfile;
    for (const auto& point : profile) {
        // Keep x as axial position (horizontal axis), y as radius
        // Scale down by a reasonable factor to fit in the 3D viewport
        bellowsProfile.push_back(ImVec2(point.y / 100.0f, point.x / 100.0f));
    }
    
    // Revolve profile around X-axis to create horizontal 3D geometry
    // Use higher segment count for bellows (72 segments for smooth curves)
    revolveProfileAroundX(bellowsProfile, 72);
}
