#include "ShockAbsorberModel3D.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

ShockAbsorberModel3D::ShockAbsorberModel3D() : Base3DModel(), 
    currentSpring(nullptr), currentEnd(nullptr), currentBottomEnd(nullptr) {
    // Initialize shock absorber-specific default material properties
    objectColor = glm::vec3(0.7f, 0.7f, 0.75f);
    ambientStrength = 0.3f;
    diffuseStrength = 0.7f;
    specularStrength = 0.6f;
    shininess = 64.0f;
}

void ShockAbsorberModel3D::generateMesh() {
    if (currentSpring && currentEnd && currentBottomEnd) {
        generateMesh(currentSpring, currentEnd, currentBottomEnd);
    }
}

void ShockAbsorberModel3D::generateMesh(const Drawing::Spring2D* spring, const Drawing::ShockAbsorberEnd2D* end, 
                                        const Drawing::ShockAbsorberBottomEnd* bottomEnd) {
    if (!spring || !end || !bottomEnd) {
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
}

void ShockAbsorberModel3D::generateShockAbsorberGeometry() {
    // Scale factor to convert from mm/pixels to reasonable 3D units
    float scale = 1.0f / 100.0f;
    
    // Get spring parameters
    float springOuterRadius = (currentSpring->outerDiameter / 2.0f) * scale;
    float springWireRadius = (currentSpring->wireDiameter / 2.0f) * scale;
    float springHelixRadius = springOuterRadius - springWireRadius;
    float springLength = currentSpring->freeLength * scale;
    int numCoils = currentSpring->numCoils;
    
    // Shock absorber dimensions based on spring
    float damperTubeOuterRadius = springHelixRadius * 0.5f;  // Inner tube the spring wraps around
    float damperTubeInnerRadius = damperTubeOuterRadius * 0.8f;
    float pistonRodRadius = damperTubeInnerRadius * 0.6f;
    
    // Total assembly dimensions
    float totalLength = springLength * 1.8f;  // Total shock absorber length
    float springStartY = -springLength * 0.4f;  // Where spring starts (from center)
    float springEndY = springStartY + springLength;  // Where spring ends
    
    // Top mount dimensions
    float topMountHeight = totalLength * 0.15f;
    float topMountRadius = springOuterRadius * 0.6f;
    float topMountY = totalLength / 2.0f;
    
    // Bottom mount dimensions  
    float bottomMountHeight = totalLength * 0.12f;
    float bottomMountRadius = springOuterRadius * 1.1f;
    float bottomMountY = -totalLength / 2.0f;
    
    // Spring seat (flange) dimensions
    float springSeaTthickness = springLength * 0.03f;
    float springSeatRadius = springOuterRadius * 1.05f;
    
    int radialSegments = 32;
    int helixSegments = numCoils * 24;
    
    // Lambda to generate a cylinder
    auto generateCylinder = [&](float bottomY, float topY, float outerRadius, float innerRadius, bool hasInnerHole) {
        unsigned int baseVertex = vertices.size() / 6;
        
        // Outer surface - bottom ring
        for (int j = 0; j < radialSegments; ++j) {
            float angle = (float)j / radialSegments * 2.0f * M_PI;
            float nx = std::cos(angle);
            float nz = std::sin(angle);
            vertices.push_back(outerRadius * nx);
            vertices.push_back(bottomY);
            vertices.push_back(outerRadius * nz);
            vertices.push_back(nx);
            vertices.push_back(0.0f);
            vertices.push_back(nz);
        }
        
        // Outer surface - top ring
        for (int j = 0; j < radialSegments; ++j) {
            float angle = (float)j / radialSegments * 2.0f * M_PI;
            float nx = std::cos(angle);
            float nz = std::sin(angle);
            vertices.push_back(outerRadius * nx);
            vertices.push_back(topY);
            vertices.push_back(outerRadius * nz);
            vertices.push_back(nx);
            vertices.push_back(0.0f);
            vertices.push_back(nz);
        }
        
        // Outer surface faces
        for (int j = 0; j < radialSegments; ++j) {
            unsigned int bl = baseVertex + j;
            unsigned int br = baseVertex + (j + 1) % radialSegments;
            unsigned int tl = baseVertex + radialSegments + j;
            unsigned int tr = baseVertex + radialSegments + (j + 1) % radialSegments;
            indices.push_back(bl);
            indices.push_back(br);
            indices.push_back(tl);
            indices.push_back(br);
            indices.push_back(tr);
            indices.push_back(tl);
        }
        
        if (hasInnerHole && innerRadius > 0) {
            // Inner surface (for hollow cylinders)
            unsigned int innerBase = vertices.size() / 6;
            
            // Inner surface - bottom ring
            for (int j = 0; j < radialSegments; ++j) {
                float angle = (float)j / radialSegments * 2.0f * M_PI;
                float nx = -std::cos(angle);
                float nz = -std::sin(angle);
                vertices.push_back(innerRadius * std::cos(angle));
                vertices.push_back(bottomY);
                vertices.push_back(innerRadius * std::sin(angle));
                vertices.push_back(nx);
                vertices.push_back(0.0f);
                vertices.push_back(nz);
            }
            
            // Inner surface - top ring
            for (int j = 0; j < radialSegments; ++j) {
                float angle = (float)j / radialSegments * 2.0f * M_PI;
                float nx = -std::cos(angle);
                float nz = -std::sin(angle);
                vertices.push_back(innerRadius * std::cos(angle));
                vertices.push_back(topY);
                vertices.push_back(innerRadius * std::sin(angle));
                vertices.push_back(nx);
                vertices.push_back(0.0f);
                vertices.push_back(nz);
            }
            
            // Inner surface faces (reversed winding)
            for (int j = 0; j < radialSegments; ++j) {
                unsigned int bl = innerBase + j;
                unsigned int br = innerBase + (j + 1) % radialSegments;
                unsigned int tl = innerBase + radialSegments + j;
                unsigned int tr = innerBase + radialSegments + (j + 1) % radialSegments;
                indices.push_back(bl);
                indices.push_back(tl);
                indices.push_back(br);
                indices.push_back(br);
                indices.push_back(tl);
                indices.push_back(tr);
            }
            
            // Top annular ring
            unsigned int topRingBase = vertices.size() / 6;
            for (int j = 0; j < radialSegments; ++j) {
                float angle = (float)j / radialSegments * 2.0f * M_PI;
                vertices.push_back(innerRadius * std::cos(angle));
                vertices.push_back(topY);
                vertices.push_back(innerRadius * std::sin(angle));
                vertices.push_back(0.0f);
                vertices.push_back(1.0f);
                vertices.push_back(0.0f);
            }
            for (int j = 0; j < radialSegments; ++j) {
                float angle = (float)j / radialSegments * 2.0f * M_PI;
                vertices.push_back(outerRadius * std::cos(angle));
                vertices.push_back(topY);
                vertices.push_back(outerRadius * std::sin(angle));
                vertices.push_back(0.0f);
                vertices.push_back(1.0f);
                vertices.push_back(0.0f);
            }
            for (int j = 0; j < radialSegments; ++j) {
                unsigned int i0 = topRingBase + j;
                unsigned int i1 = topRingBase + (j + 1) % radialSegments;
                unsigned int o0 = topRingBase + radialSegments + j;
                unsigned int o1 = topRingBase + radialSegments + (j + 1) % radialSegments;
                indices.push_back(i0);
                indices.push_back(o0);
                indices.push_back(i1);
                indices.push_back(i1);
                indices.push_back(o0);
                indices.push_back(o1);
            }
            
            // Bottom annular ring
            unsigned int botRingBase = vertices.size() / 6;
            for (int j = 0; j < radialSegments; ++j) {
                float angle = (float)j / radialSegments * 2.0f * M_PI;
                vertices.push_back(innerRadius * std::cos(angle));
                vertices.push_back(bottomY);
                vertices.push_back(innerRadius * std::sin(angle));
                vertices.push_back(0.0f);
                vertices.push_back(-1.0f);
                vertices.push_back(0.0f);
            }
            for (int j = 0; j < radialSegments; ++j) {
                float angle = (float)j / radialSegments * 2.0f * M_PI;
                vertices.push_back(outerRadius * std::cos(angle));
                vertices.push_back(bottomY);
                vertices.push_back(outerRadius * std::sin(angle));
                vertices.push_back(0.0f);
                vertices.push_back(-1.0f);
                vertices.push_back(0.0f);
            }
            for (int j = 0; j < radialSegments; ++j) {
                unsigned int i0 = botRingBase + j;
                unsigned int i1 = botRingBase + (j + 1) % radialSegments;
                unsigned int o0 = botRingBase + radialSegments + j;
                unsigned int o1 = botRingBase + radialSegments + (j + 1) % radialSegments;
                indices.push_back(i0);
                indices.push_back(i1);
                indices.push_back(o0);
                indices.push_back(i1);
                indices.push_back(o1);
                indices.push_back(o0);
            }
        } else {
            // Solid caps for solid cylinders
            // Top cap
            unsigned int topCapBase = vertices.size() / 6;
            vertices.push_back(0.0f);
            vertices.push_back(topY);
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
            vertices.push_back(1.0f);
            vertices.push_back(0.0f);
            for (int j = 0; j < radialSegments; ++j) {
                float angle = (float)j / radialSegments * 2.0f * M_PI;
                vertices.push_back(outerRadius * std::cos(angle));
                vertices.push_back(topY);
                vertices.push_back(outerRadius * std::sin(angle));
                vertices.push_back(0.0f);
                vertices.push_back(1.0f);
                vertices.push_back(0.0f);
            }
            for (int j = 0; j < radialSegments; ++j) {
                indices.push_back(topCapBase);
                indices.push_back(topCapBase + 1 + j);
                indices.push_back(topCapBase + 1 + (j + 1) % radialSegments);
            }
            
            // Bottom cap
            unsigned int botCapBase = vertices.size() / 6;
            vertices.push_back(0.0f);
            vertices.push_back(bottomY);
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
            vertices.push_back(-1.0f);
            vertices.push_back(0.0f);
            for (int j = 0; j < radialSegments; ++j) {
                float angle = (float)j / radialSegments * 2.0f * M_PI;
                vertices.push_back(outerRadius * std::cos(angle));
                vertices.push_back(bottomY);
                vertices.push_back(outerRadius * std::sin(angle));
                vertices.push_back(0.0f);
                vertices.push_back(-1.0f);
                vertices.push_back(0.0f);
            }
            for (int j = 0; j < radialSegments; ++j) {
                indices.push_back(botCapBase);
                indices.push_back(botCapBase + 1 + (j + 1) % radialSegments);
                indices.push_back(botCapBase + 1 + j);
            }
        }
    };
    
    // ============ 1. BOTTOM MOUNT (Eye/Clevis mount) ============
    generateCylinder(bottomMountY, bottomMountY + bottomMountHeight, bottomMountRadius, bottomMountRadius * 0.3f, true);
    
    // ============ 2. DAMPER TUBE (Outer cylinder that spring wraps around) ============
    float damperTubeBottom = bottomMountY + bottomMountHeight;
    float damperTubeTop = springEndY + springSeaTthickness;
    generateCylinder(damperTubeBottom, damperTubeTop, damperTubeOuterRadius, damperTubeInnerRadius, true);
    
    // ============ 3. BOTTOM SPRING SEAT (Flange for spring to sit on) ============
    generateCylinder(springStartY - springSeaTthickness, springStartY, springSeatRadius, damperTubeOuterRadius, true);
    
    // ============ 4. COIL SPRING (Helical spring around the damper tube) ============
    for (int i = 0; i <= helixSegments; ++i) {
        float t = (float)i / helixSegments;
        float theta = t * numCoils * 2.0f * M_PI;
        float y = springStartY + t * springLength;
        
        // Center of wire at this point on helix
        float cx = springHelixRadius * std::sin(theta);
        float cz = springHelixRadius * std::cos(theta);
        
        // Tangent direction along helix for proper normal calculation
        float tangentX = springHelixRadius * std::cos(theta) * numCoils * 2.0f * M_PI / helixSegments;
        float tangentY = springLength / helixSegments;
        float tangentZ = -springHelixRadius * std::sin(theta) * numCoils * 2.0f * M_PI / helixSegments;
        float tangentLen = std::sqrt(tangentX*tangentX + tangentY*tangentY + tangentZ*tangentZ);
        tangentX /= tangentLen;
        tangentY /= tangentLen;
        tangentZ /= tangentLen;
        
        // Generate ring of vertices around wire cross-section
        for (int j = 0; j < radialSegments; ++j) {
            float phi = (float)j / radialSegments * 2.0f * M_PI;
            
            // Binormal and normal for the helix
            float binormalX = -std::sin(theta);
            float binormalY = 0.0f;
            float binormalZ = -std::cos(theta);
            
            // Normal points outward from helix center
            float helixNormalX = std::sin(theta);
            float helixNormalY = 0.0f;
            float helixNormalZ = std::cos(theta);
            
            // Wire surface normal
            float wireNx = std::cos(phi) * helixNormalX + std::sin(phi) * 0.0f;
            float wireNy = std::sin(phi);
            float wireNz = std::cos(phi) * helixNormalZ + std::sin(phi) * 0.0f;
            
            // Normalize
            float nLen = std::sqrt(wireNx*wireNx + wireNy*wireNy + wireNz*wireNz);
            if (nLen > 0) {
                wireNx /= nLen;
                wireNy /= nLen;
                wireNz /= nLen;
            }
            
            // Position on wire surface
            float vx = cx + springWireRadius * std::cos(phi) * helixNormalX;
            float vy = y + springWireRadius * std::sin(phi);
            float vz = cz + springWireRadius * std::cos(phi) * helixNormalZ;
            
            vertices.push_back(vx);
            vertices.push_back(vy);
            vertices.push_back(vz);
            vertices.push_back(wireNx);
            vertices.push_back(wireNy);
            vertices.push_back(wireNz);
        }
    }
    
    // Generate indices for spring
    unsigned int springBaseVertex = vertices.size() / 6 - (helixSegments + 1) * radialSegments;
    for (int i = 0; i < helixSegments; ++i) {
        for (int j = 0; j < radialSegments; ++j) {
            unsigned int curr = springBaseVertex + i * radialSegments + j;
            unsigned int next = springBaseVertex + i * radialSegments + (j + 1) % radialSegments;
            unsigned int currNext = springBaseVertex + (i + 1) * radialSegments + j;
            unsigned int nextNext = springBaseVertex + (i + 1) * radialSegments + (j + 1) % radialSegments;
            
            indices.push_back(curr);
            indices.push_back(next);
            indices.push_back(currNext);
            
            indices.push_back(next);
            indices.push_back(nextNext);
            indices.push_back(currNext);
        }
    }
    
    // ============ 5. TOP SPRING SEAT ============
    generateCylinder(springEndY, springEndY + springSeaTthickness, springSeatRadius, damperTubeOuterRadius, true);
    
    // ============ 6. PISTON ROD (Extends from top) ============
    float pistonRodBottom = damperTubeTop;
    float pistonRodTop = topMountY - topMountHeight * 0.5f;
    generateCylinder(pistonRodBottom, pistonRodTop, pistonRodRadius, 0.0f, false);
    
    // ============ 7. TOP MOUNT (Bushing/eye mount) ============
    generateCylinder(pistonRodTop, topMountY, topMountRadius, topMountRadius * 0.3f, true);
}

std::vector<ImVec2> ShockAbsorberModel3D::generateEnhancedSpringProfile() const {
    return std::vector<ImVec2>();
}
