#include "ShockAbsorberModel3D.hpp"
#include <iostream>
#include <algorithm>

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
    if (!spring || !end || !bottomEnd) {
        std::cerr << "[ShockAbsorberModel3D] Missing component(s) for shock absorber generation!" << std::endl;
        return;
    }
    
    // Store references for potential regeneration
    currentSpring = spring;
    currentEnd = end;
    currentBottomEnd = bottomEnd;
    
    // Clear existing mesh data
    clearMesh();
    
    // Generate shock absorber-specific geometry
    generateShockAbsorberGeometry();
    
    // Calculate normals
    generateNormals();
    
    // Setup OpenGL buffers
    setupMesh();
    
    // Reset view to default
    resetView();
    
    std::cerr << "[ShockAbsorberModel3D] Mesh generation complete with standardized architecture." << std::endl;
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
    
    // Calculate Y offsets to position components correctly
    float springLength = currentSpring->freeLength;
    float endLength = currentEnd->shaftLength;
    float bottomLength = currentBottomEnd->height + currentBottomEnd->plateThickness;
    
    float yTopEnd = currentSpring->centerY + springLength / 2.0f + endLength / 2.0f;
    float ySpring = currentSpring->centerY;
    float yBottomEnd = currentSpring->centerY - springLength / 2.0f - bottomLength / 2.0f;
    
    std::cerr << "[ShockAbsorberModel3D] Component offsets: yTopEnd=" << yTopEnd << ", ySpring=" << ySpring << ", yBottomEnd=" << yBottomEnd << std::endl;
    
    // Revolve each profile at the correct Y offset using standardized method
    revolveProfile(endProfile, 36, yTopEnd - currentEnd->baseCenter.y, 1.0f);
    revolveProfile(springProfile, 36, ySpring - currentSpring->centerY, 1.0f);
    revolveProfile(bottomProfile, 36, yBottomEnd - currentBottomEnd->baseCenter.y, 1.0f);
    
    // Normalize the entire geometry to fit in reasonable bounds
    if (!vertices.empty()) {
        // Find bounds
        float minX = 1e9, maxX = -1e9, minY = 1e9, maxY = -1e9, minZ = 1e9, maxZ = -1e9;
        for (size_t i = 0; i < vertices.size() / 6; ++i) {
            float x = vertices[i * 6 + 0];
            float y = vertices[i * 6 + 1];
            float z = vertices[i * 6 + 2];
            minX = std::min(minX, x); maxX = std::max(maxX, x);
            minY = std::min(minY, y); maxY = std::max(maxY, y);
            minZ = std::min(minZ, z); maxZ = std::max(maxZ, z);
        }
        
        // Calculate center and scale
        float centerX = (minX + maxX) / 2.0f;
        float centerY = (minY + maxY) / 2.0f;
        float centerZ = (minZ + maxZ) / 2.0f;
        float maxExtent = std::max({maxX - minX, maxY - minY, maxZ - minZ});
        float scale = 2.0f / (maxExtent > 0 ? maxExtent : 1.0f); // fit in [-1,1]
        
        // Apply centering and scaling to all vertices
        for (size_t i = 0; i < vertices.size() / 6; ++i) {
            vertices[i * 6 + 0] = (vertices[i * 6 + 0] - centerX) * scale;
            vertices[i * 6 + 1] = (vertices[i * 6 + 1] - centerY) * scale;
            vertices[i * 6 + 2] = (vertices[i * 6 + 2] - centerZ) * scale;
        }
        
        std::cerr << "[ShockAbsorberModel3D] Normalized geometry: center(" << centerX << ", " << centerY << ", " << centerZ << "), scale=" << scale << std::endl;
    }
}
