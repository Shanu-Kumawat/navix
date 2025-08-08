#include "ShockAbsorberModel3D.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

ShockAbsorberModel3D::ShockAbsorberModel3D() : Base3DModel(), 
    currentSpring(nullptr), currentEnd(nullptr), currentBottomEnd(nullptr) {
    // Initialize shock absorber-specific default material properties
    objectColor = glm::vec3(0.8f, 0.8f, 0.8f);
    ambientStrength = 0.3f;
    diffuseStrength = 0.7f;
    specularStrength = 0.5f;
    shininess = 32.0f;
}

void ShockAbsorberModel3D::generateMesh() {
    if (currentSpring && currentEnd && currentBottomEnd) {
        generateMesh(currentSpring, currentEnd, currentBottomEnd);
    }
}

void ShockAbsorberModel3D::generateMesh(const Drawing::Spring2D* spring, const Drawing::ShockAbsorberEnd2D* end, 
                                        const Drawing::ShockAbsorberBottomEnd* bottomEnd) {
    std::cout << "[ShockAbsorberModel3D] generateMesh called with spring=" << (spring ? "valid" : "null") 
              << ", end=" << (end ? "valid" : "null") 
              << ", bottomEnd=" << (bottomEnd ? "valid" : "null") << std::endl;
              
    if (!spring || !end || !bottomEnd) {
        std::cout << "[ShockAbsorberModel3D] ERROR: One or more components are null!" << std::endl;
        return;
    }
    
    // Store references for potential regeneration
    currentSpring = spring;
    currentEnd = end;
    currentBottomEnd = bottomEnd;
    
    // Clear existing mesh data
    clearMesh();
    
    std::cout << "[ShockAbsorberModel3D] Generating geometry..." << std::endl;
    
    // Generate shock absorber-specific geometry
    generateShockAbsorberGeometry();
    
    // Calculate normals
    generateNormals();
    
    // Setup OpenGL buffers
    setupMesh();
    
    // Reset view to default
    resetView();
    
    std::cout << "[ShockAbsorberModel3D] Mesh generation complete with " << vertices.size() << " vertices." << std::endl;
}

void ShockAbsorberModel3D::generateShockAbsorberGeometry() {
    // Get profiles from all components
    auto endProfile = currentEnd->generateProfile();
    auto springProfile = currentSpring->generateProfile();
    auto bottomProfile = currentBottomEnd->generateProfile();
    
    if (endProfile.empty() || springProfile.empty() || bottomProfile.empty()) {
        std::cerr << "[ShockAbsorberModel3D] One or more profiles are empty!" << std::endl;
        return;
    }
    
    // Calculate proper scaling - convert from mm to reasonable 3D units
    float scale = 1.0f / 100.0f; // Convert mm to centimeters
    
    // Calculate Y offsets to position components correctly
    float springLength = currentSpring->freeLength * scale;
    float endLength = currentEnd->shaftLength * scale;
    float bottomLength = (currentBottomEnd->height + currentBottomEnd->plateThickness) * scale;
    
    float yTopEnd = currentSpring->centerY * scale + springLength / 2.0f + endLength / 2.0f;
    float ySpring = currentSpring->centerY * scale;
    float yBottomEnd = currentSpring->centerY * scale - springLength / 2.0f - bottomLength / 2.0f;
    
    std::cerr << "[ShockAbsorberModel3D] Component offsets: yTopEnd=" << yTopEnd << ", ySpring=" << ySpring << ", yBottomEnd=" << yBottomEnd << std::endl;
    
    // Convert profiles to proper format for revolution (x = radius, y = axial position)
    // Scale profiles appropriately
    std::vector<ImVec2> scaledEndProfile, scaledSpringProfile, scaledBottomProfile;
    
    // Scale end profile
    for (const auto& point : endProfile) {
        scaledEndProfile.push_back(ImVec2(std::abs(point.x - currentEnd->baseCenter.x) * scale, 
                                         (point.y - currentEnd->baseCenter.y) * scale));
    }
    
    // Generate enhanced spring profile with visible coils
    scaledSpringProfile = generateEnhancedSpringProfile();
    
    // Scale bottom profile  
    for (const auto& point : bottomProfile) {
        scaledBottomProfile.push_back(ImVec2(std::abs(point.x - currentBottomEnd->baseCenter.x) * scale,
                                            (point.y - currentBottomEnd->baseCenter.y) * scale));
    }
    
    // Revolve each profile with higher segment count for better quality
    revolveProfile(scaledEndProfile, 72, yTopEnd, 1.0f);
    revolveProfile(scaledSpringProfile, 72, ySpring, 1.0f);
    revolveProfile(scaledBottomProfile, 72, yBottomEnd, 1.0f);
    
    std::cerr << "[ShockAbsorberModel3D] Generated " << vertices.size()/6 << " vertices and " << indices.size()/3 << " triangles" << std::endl;
}

std::vector<ImVec2> ShockAbsorberModel3D::generateEnhancedSpringProfile() const {
    std::vector<ImVec2> profile;
    
    // Spring parameters (scaled)
    float scale = 1.0f / 100.0f;
    float outerRadius = (currentSpring->outerDiameter / 2.0f) * scale;
    float wireRadius = (currentSpring->wireDiameter / 2.0f) * scale;
    float innerRadius = outerRadius - wireRadius;
    float springLength = currentSpring->freeLength * scale;
    
    std::cout << "[ShockAbsorberModel3D] Spring profile: outerRadius=" << outerRadius 
              << ", wireRadius=" << wireRadius << ", springLength=" << springLength << std::endl;
    
    // Generate a more detailed spring profile that shows the wire cross-section
    int profilePoints = 20; // Points around the wire cross-section
    float pitchHeight = springLength / currentSpring->numCoils;
    
    // Define PI if not available
    const float PI = 3.14159265358979323846f;
    
    // Generate one complete coil profile with wire detail
    for (int i = 0; i < profilePoints; ++i) {
        float angle = (float)i / profilePoints * 2.0f * PI;
        
        // Wire cross-section: create a circular cross-section
        float wireOffset = wireRadius * cos(angle);
        float heightOffset = wireRadius * sin(angle);
        
        // Create profile points that will show the wire when revolved
        float radius = outerRadius + wireOffset;
        float height = heightOffset; // Relative to spring center
        
        profile.push_back(ImVec2(radius, height));
    }
    
    // Close the profile
    if (!profile.empty()) {
        profile.push_back(profile[0]);
    }
    
    return profile;
}
