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
    
    // Shock absorber dimensions based on spring (matching reference image)
    float pistonRodRadius = springWireRadius * 0.8f;  // Thin central rod
    float damperTubeOuterRadius = springHelixRadius * 0.35f;  // Thin inner tube
    float damperTubeInnerRadius = damperTubeOuterRadius * 0.7f;
    
    // Total assembly dimensions
    float totalLength = springLength * 2.0f;  // Total shock absorber length
    float springStartY = -springLength * 0.5f;  // Spring starts at bottom half
    float springEndY = springStartY + springLength;  // Where spring ends
    
    // Top mount dimensions (nut-like structure with cap)
    float topNutHeight = springLength * 0.08f;
    float topNutRadius = springOuterRadius * 0.4f;
    float topCapHeight = springLength * 0.06f;
    float topCapRadius = springOuterRadius * 0.35f;
    float topCollarHeight = springLength * 0.05f;
    float topCollarRadius = springOuterRadius * 0.5f;
    
    // Upper spring seat (flange where spring top sits)
    float upperSeatThickness = springLength * 0.035f;
    float upperSeatRadius = springOuterRadius * 1.05f;
    
    // Lower spring seat (flange where spring bottom sits)  
    float lowerSeatThickness = springLength * 0.035f;
    float lowerSeatRadius = springOuterRadius * 1.05f;
    
    // Bottom mount dimensions (U-shaped clevis with hole)
    float bottomMountHeight = springLength * 0.35f;
    float bottomMountWidth = springOuterRadius * 0.9f;
    float bottomMountThickness = springWireRadius * 0.6f;
    float bottomHoleRadius = springWireRadius * 0.8f;
    float bottomPlateThickness = springWireRadius * 0.7f;
    
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
    
    // Lambda to generate top mount with rectangular block and 2 horizontal bars (left-right only)
    auto generateTopMountWithBars = [&](float bottomY, float topY, float blockWidth, float barLength, float barHeight) {
        // Central rectangular block (not hexagonal - just a cylinder for the main body)
        float blockRadius = blockWidth / 2.0f;
        generateCylinder(bottomY, topY, blockRadius, 0.0f, false);
        
        // Two horizontal rectangular bars extending left and right
        float barBottom = bottomY + (topY - bottomY) * 0.3f;
        float barTop = bottomY + (topY - bottomY) * 0.7f;
        float barDepth = blockWidth * 0.5f;  // Thickness in Z direction
        
        // Helper to add a box
        auto addBox = [&](float x1, float x2, float y1, float y2, float z1, float z2) {
            unsigned int base = vertices.size() / 6;
            
            // 8 vertices of the box
            float verts[8][3] = {
                {x1, y1, z1}, {x2, y1, z1}, {x2, y2, z1}, {x1, y2, z1},  // front face
                {x1, y1, z2}, {x2, y1, z2}, {x2, y2, z2}, {x1, y2, z2}   // back face
            };
            float norms[6][3] = {
                {0, 0, 1}, {0, 0, -1}, {-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}
            };
            
            // Front face (z1)
            for (int i = 0; i < 4; ++i) {
                vertices.push_back(verts[i][0]); vertices.push_back(verts[i][1]); vertices.push_back(verts[i][2]);
                vertices.push_back(0); vertices.push_back(0); vertices.push_back(-1);
            }
            indices.push_back(base); indices.push_back(base+1); indices.push_back(base+2);
            indices.push_back(base); indices.push_back(base+2); indices.push_back(base+3);
            
            // Back face (z2)
            base = vertices.size() / 6;
            for (int i = 4; i < 8; ++i) {
                vertices.push_back(verts[i][0]); vertices.push_back(verts[i][1]); vertices.push_back(verts[i][2]);
                vertices.push_back(0); vertices.push_back(0); vertices.push_back(1);
            }
            indices.push_back(base); indices.push_back(base+2); indices.push_back(base+1);
            indices.push_back(base); indices.push_back(base+3); indices.push_back(base+2);
            
            // Left face (x1)
            base = vertices.size() / 6;
            int leftIdx[4] = {0, 4, 7, 3};
            for (int i = 0; i < 4; ++i) {
                vertices.push_back(verts[leftIdx[i]][0]); vertices.push_back(verts[leftIdx[i]][1]); vertices.push_back(verts[leftIdx[i]][2]);
                vertices.push_back(-1); vertices.push_back(0); vertices.push_back(0);
            }
            indices.push_back(base); indices.push_back(base+1); indices.push_back(base+2);
            indices.push_back(base); indices.push_back(base+2); indices.push_back(base+3);
            
            // Right face (x2)
            base = vertices.size() / 6;
            int rightIdx[4] = {1, 5, 6, 2};
            for (int i = 0; i < 4; ++i) {
                vertices.push_back(verts[rightIdx[i]][0]); vertices.push_back(verts[rightIdx[i]][1]); vertices.push_back(verts[rightIdx[i]][2]);
                vertices.push_back(1); vertices.push_back(0); vertices.push_back(0);
            }
            indices.push_back(base); indices.push_back(base+2); indices.push_back(base+1);
            indices.push_back(base); indices.push_back(base+3); indices.push_back(base+2);
            
            // Bottom face (y1)
            base = vertices.size() / 6;
            int botIdx[4] = {0, 1, 5, 4};
            for (int i = 0; i < 4; ++i) {
                vertices.push_back(verts[botIdx[i]][0]); vertices.push_back(verts[botIdx[i]][1]); vertices.push_back(verts[botIdx[i]][2]);
                vertices.push_back(0); vertices.push_back(-1); vertices.push_back(0);
            }
            indices.push_back(base); indices.push_back(base+1); indices.push_back(base+2);
            indices.push_back(base); indices.push_back(base+2); indices.push_back(base+3);
            
            // Top face (y2)
            base = vertices.size() / 6;
            int topIdx[4] = {3, 2, 6, 7};
            for (int i = 0; i < 4; ++i) {
                vertices.push_back(verts[topIdx[i]][0]); vertices.push_back(verts[topIdx[i]][1]); vertices.push_back(verts[topIdx[i]][2]);
                vertices.push_back(0); vertices.push_back(1); vertices.push_back(0);
            }
            indices.push_back(base); indices.push_back(base+1); indices.push_back(base+2);
            indices.push_back(base); indices.push_back(base+2); indices.push_back(base+3);
        };
        
        // Left bar (extends from -blockRadius to -barLength)
        addBox(-barLength, -blockRadius * 0.8f, barBottom, barTop, -barDepth/2, barDepth/2);
        
        // Right bar (extends from blockRadius to barLength)
        addBox(blockRadius * 0.8f, barLength, barBottom, barTop, -barDepth/2, barDepth/2);
    };
    
    // Lambda to generate U-shaped bottom mount (clevis) - improved version
    auto generateUMount = [&](float topY, float mountHeight, float mountWidth, float legThickness) {
        // The U-mount consists of: a plate at top, two solid legs going down, connected by a curved bottom with a hole
        float plateTop = topY + bottomPlateThickness;
        float plateBottom = topY;
        float legBottom = topY - mountHeight;
        float halfWidth = mountWidth / 2.0f;
        float legWidth = legThickness * 2.0f;  // Width of each leg
        float holeY = legBottom + mountHeight * 0.4f;  // Hole position
        float holeR = mountHeight * 0.15f;  // Hole radius
        
        // Generate the plate at top (wider disc)
        generateCylinder(plateBottom, plateTop, mountWidth * 0.7f, pistonRodRadius, true);
        
        // Generate left leg as a solid box
        float leftOuterX = -halfWidth;
        float leftInnerX = -halfWidth + legWidth;
        float legDepth = legThickness * 1.5f;
        
        // Left leg - 8 vertices for a box
        unsigned int leftBase = vertices.size() / 6;
        // Front face vertices (4)
        vertices.push_back(leftOuterX); vertices.push_back(legBottom); vertices.push_back(legDepth);
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(1.0f);
        vertices.push_back(leftInnerX); vertices.push_back(legBottom); vertices.push_back(legDepth);
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(1.0f);
        vertices.push_back(leftInnerX); vertices.push_back(plateBottom); vertices.push_back(legDepth);
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(1.0f);
        vertices.push_back(leftOuterX); vertices.push_back(plateBottom); vertices.push_back(legDepth);
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(1.0f);
        // Front face
        indices.push_back(leftBase); indices.push_back(leftBase+1); indices.push_back(leftBase+2);
        indices.push_back(leftBase); indices.push_back(leftBase+2); indices.push_back(leftBase+3);
        
        // Back face vertices (4)
        unsigned int leftBackBase = vertices.size() / 6;
        vertices.push_back(leftOuterX); vertices.push_back(legBottom); vertices.push_back(-legDepth);
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(-1.0f);
        vertices.push_back(leftInnerX); vertices.push_back(legBottom); vertices.push_back(-legDepth);
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(-1.0f);
        vertices.push_back(leftInnerX); vertices.push_back(plateBottom); vertices.push_back(-legDepth);
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(-1.0f);
        vertices.push_back(leftOuterX); vertices.push_back(plateBottom); vertices.push_back(-legDepth);
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(-1.0f);
        // Back face (reversed winding)
        indices.push_back(leftBackBase); indices.push_back(leftBackBase+2); indices.push_back(leftBackBase+1);
        indices.push_back(leftBackBase); indices.push_back(leftBackBase+3); indices.push_back(leftBackBase+2);
        
        // Left outer face
        unsigned int leftOuterBase = vertices.size() / 6;
        vertices.push_back(leftOuterX); vertices.push_back(legBottom); vertices.push_back(legDepth);
        vertices.push_back(-1.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
        vertices.push_back(leftOuterX); vertices.push_back(legBottom); vertices.push_back(-legDepth);
        vertices.push_back(-1.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
        vertices.push_back(leftOuterX); vertices.push_back(plateBottom); vertices.push_back(-legDepth);
        vertices.push_back(-1.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
        vertices.push_back(leftOuterX); vertices.push_back(plateBottom); vertices.push_back(legDepth);
        vertices.push_back(-1.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
        indices.push_back(leftOuterBase); indices.push_back(leftOuterBase+1); indices.push_back(leftOuterBase+2);
        indices.push_back(leftOuterBase); indices.push_back(leftOuterBase+2); indices.push_back(leftOuterBase+3);
        
        // Left inner face
        unsigned int leftInnerBase = vertices.size() / 6;
        vertices.push_back(leftInnerX); vertices.push_back(legBottom); vertices.push_back(legDepth);
        vertices.push_back(1.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
        vertices.push_back(leftInnerX); vertices.push_back(legBottom); vertices.push_back(-legDepth);
        vertices.push_back(1.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
        vertices.push_back(leftInnerX); vertices.push_back(plateBottom); vertices.push_back(-legDepth);
        vertices.push_back(1.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
        vertices.push_back(leftInnerX); vertices.push_back(plateBottom); vertices.push_back(legDepth);
        vertices.push_back(1.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
        indices.push_back(leftInnerBase); indices.push_back(leftInnerBase+2); indices.push_back(leftInnerBase+1);
        indices.push_back(leftInnerBase); indices.push_back(leftInnerBase+3); indices.push_back(leftInnerBase+2);
        
        // Left bottom face
        unsigned int leftBotBase = vertices.size() / 6;
        vertices.push_back(leftOuterX); vertices.push_back(legBottom); vertices.push_back(legDepth);
        vertices.push_back(0.0f); vertices.push_back(-1.0f); vertices.push_back(0.0f);
        vertices.push_back(leftInnerX); vertices.push_back(legBottom); vertices.push_back(legDepth);
        vertices.push_back(0.0f); vertices.push_back(-1.0f); vertices.push_back(0.0f);
        vertices.push_back(leftInnerX); vertices.push_back(legBottom); vertices.push_back(-legDepth);
        vertices.push_back(0.0f); vertices.push_back(-1.0f); vertices.push_back(0.0f);
        vertices.push_back(leftOuterX); vertices.push_back(legBottom); vertices.push_back(-legDepth);
        vertices.push_back(0.0f); vertices.push_back(-1.0f); vertices.push_back(0.0f);
        indices.push_back(leftBotBase); indices.push_back(leftBotBase+2); indices.push_back(leftBotBase+1);
        indices.push_back(leftBotBase); indices.push_back(leftBotBase+3); indices.push_back(leftBotBase+2);
        
        // Right leg - mirror of left
        float rightOuterX = halfWidth;
        float rightInnerX = halfWidth - legWidth;
        
        // Right front face
        unsigned int rightBase = vertices.size() / 6;
        vertices.push_back(rightInnerX); vertices.push_back(legBottom); vertices.push_back(legDepth);
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(1.0f);
        vertices.push_back(rightOuterX); vertices.push_back(legBottom); vertices.push_back(legDepth);
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(1.0f);
        vertices.push_back(rightOuterX); vertices.push_back(plateBottom); vertices.push_back(legDepth);
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(1.0f);
        vertices.push_back(rightInnerX); vertices.push_back(plateBottom); vertices.push_back(legDepth);
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(1.0f);
        indices.push_back(rightBase); indices.push_back(rightBase+1); indices.push_back(rightBase+2);
        indices.push_back(rightBase); indices.push_back(rightBase+2); indices.push_back(rightBase+3);
        
        // Right back face
        unsigned int rightBackBase = vertices.size() / 6;
        vertices.push_back(rightInnerX); vertices.push_back(legBottom); vertices.push_back(-legDepth);
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(-1.0f);
        vertices.push_back(rightOuterX); vertices.push_back(legBottom); vertices.push_back(-legDepth);
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(-1.0f);
        vertices.push_back(rightOuterX); vertices.push_back(plateBottom); vertices.push_back(-legDepth);
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(-1.0f);
        vertices.push_back(rightInnerX); vertices.push_back(plateBottom); vertices.push_back(-legDepth);
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(-1.0f);
        indices.push_back(rightBackBase); indices.push_back(rightBackBase+2); indices.push_back(rightBackBase+1);
        indices.push_back(rightBackBase); indices.push_back(rightBackBase+3); indices.push_back(rightBackBase+2);
        
        // Right outer face
        unsigned int rightOuterBase = vertices.size() / 6;
        vertices.push_back(rightOuterX); vertices.push_back(legBottom); vertices.push_back(legDepth);
        vertices.push_back(1.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
        vertices.push_back(rightOuterX); vertices.push_back(legBottom); vertices.push_back(-legDepth);
        vertices.push_back(1.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
        vertices.push_back(rightOuterX); vertices.push_back(plateBottom); vertices.push_back(-legDepth);
        vertices.push_back(1.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
        vertices.push_back(rightOuterX); vertices.push_back(plateBottom); vertices.push_back(legDepth);
        vertices.push_back(1.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
        indices.push_back(rightOuterBase); indices.push_back(rightOuterBase+2); indices.push_back(rightOuterBase+1);
        indices.push_back(rightOuterBase); indices.push_back(rightOuterBase+3); indices.push_back(rightOuterBase+2);
        
        // Right inner face
        unsigned int rightInnerBase = vertices.size() / 6;
        vertices.push_back(rightInnerX); vertices.push_back(legBottom); vertices.push_back(legDepth);
        vertices.push_back(-1.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
        vertices.push_back(rightInnerX); vertices.push_back(legBottom); vertices.push_back(-legDepth);
        vertices.push_back(-1.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
        vertices.push_back(rightInnerX); vertices.push_back(plateBottom); vertices.push_back(-legDepth);
        vertices.push_back(-1.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
        vertices.push_back(rightInnerX); vertices.push_back(plateBottom); vertices.push_back(legDepth);
        vertices.push_back(-1.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
        indices.push_back(rightInnerBase); indices.push_back(rightInnerBase+1); indices.push_back(rightInnerBase+2);
        indices.push_back(rightInnerBase); indices.push_back(rightInnerBase+2); indices.push_back(rightInnerBase+3);
        
        // Right bottom face
        unsigned int rightBotBase = vertices.size() / 6;
        vertices.push_back(rightInnerX); vertices.push_back(legBottom); vertices.push_back(legDepth);
        vertices.push_back(0.0f); vertices.push_back(-1.0f); vertices.push_back(0.0f);
        vertices.push_back(rightOuterX); vertices.push_back(legBottom); vertices.push_back(legDepth);
        vertices.push_back(0.0f); vertices.push_back(-1.0f); vertices.push_back(0.0f);
        vertices.push_back(rightOuterX); vertices.push_back(legBottom); vertices.push_back(-legDepth);
        vertices.push_back(0.0f); vertices.push_back(-1.0f); vertices.push_back(0.0f);
        vertices.push_back(rightInnerX); vertices.push_back(legBottom); vertices.push_back(-legDepth);
        vertices.push_back(0.0f); vertices.push_back(-1.0f); vertices.push_back(0.0f);
        indices.push_back(rightBotBase); indices.push_back(rightBotBase+2); indices.push_back(rightBotBase+1);
        indices.push_back(rightBotBase); indices.push_back(rightBotBase+3); indices.push_back(rightBotBase+2);
        
        // Bottom connecting bar between legs
        float barTop = legBottom + mountHeight * 0.25f;
        float barBottom = legBottom;
        unsigned int barBase = vertices.size() / 6;
        
        // Bar front face
        vertices.push_back(leftInnerX); vertices.push_back(barBottom); vertices.push_back(legDepth);
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(1.0f);
        vertices.push_back(rightInnerX); vertices.push_back(barBottom); vertices.push_back(legDepth);
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(1.0f);
        vertices.push_back(rightInnerX); vertices.push_back(barTop); vertices.push_back(legDepth);
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(1.0f);
        vertices.push_back(leftInnerX); vertices.push_back(barTop); vertices.push_back(legDepth);
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(1.0f);
        indices.push_back(barBase); indices.push_back(barBase+1); indices.push_back(barBase+2);
        indices.push_back(barBase); indices.push_back(barBase+2); indices.push_back(barBase+3);
        
        // Bar back face
        unsigned int barBackBase = vertices.size() / 6;
        vertices.push_back(leftInnerX); vertices.push_back(barBottom); vertices.push_back(-legDepth);
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(-1.0f);
        vertices.push_back(rightInnerX); vertices.push_back(barBottom); vertices.push_back(-legDepth);
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(-1.0f);
        vertices.push_back(rightInnerX); vertices.push_back(barTop); vertices.push_back(-legDepth);
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(-1.0f);
        vertices.push_back(leftInnerX); vertices.push_back(barTop); vertices.push_back(-legDepth);
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(-1.0f);
        indices.push_back(barBackBase); indices.push_back(barBackBase+2); indices.push_back(barBackBase+1);
        indices.push_back(barBackBase); indices.push_back(barBackBase+3); indices.push_back(barBackBase+2);
        
        // Bar bottom face
        unsigned int barBotBase = vertices.size() / 6;
        vertices.push_back(leftInnerX); vertices.push_back(barBottom); vertices.push_back(legDepth);
        vertices.push_back(0.0f); vertices.push_back(-1.0f); vertices.push_back(0.0f);
        vertices.push_back(rightInnerX); vertices.push_back(barBottom); vertices.push_back(legDepth);
        vertices.push_back(0.0f); vertices.push_back(-1.0f); vertices.push_back(0.0f);
        vertices.push_back(rightInnerX); vertices.push_back(barBottom); vertices.push_back(-legDepth);
        vertices.push_back(0.0f); vertices.push_back(-1.0f); vertices.push_back(0.0f);
        vertices.push_back(leftInnerX); vertices.push_back(barBottom); vertices.push_back(-legDepth);
        vertices.push_back(0.0f); vertices.push_back(-1.0f); vertices.push_back(0.0f);
        indices.push_back(barBotBase); indices.push_back(barBotBase+2); indices.push_back(barBotBase+1);
        indices.push_back(barBotBase); indices.push_back(barBotBase+3); indices.push_back(barBotBase+2);
    };
    
    // ============ 1. LOWER SPRING SEAT (Flange for spring to sit on) ============
    generateCylinder(springStartY - lowerSeatThickness, springStartY, lowerSeatRadius, damperTubeOuterRadius, true);
    
    // ============ 2. UPPER DAMPER TUBE (Thicker tube coming down from top into spring area) ============
    // This is the outer tube that the piston rod slides into
    float upperTubeBottom = springStartY + springLength * 0.3f;  // Extends down to 30% into spring
    float upperTubeTop = springEndY + upperSeatThickness + topCollarHeight + topNutHeight + topCapHeight;
    generateCylinder(upperTubeBottom, upperTubeTop, damperTubeOuterRadius, damperTubeInnerRadius, true);
    
    // ============ 3. LOWER PISTON ROD (Thinner rod coming up from bottom into spring area) ============
    // This is the inner rod that slides into the damper tube
    float lowerRodTop = springEndY - springLength * 0.3f;  // Extends up to 70% into spring (overlaps with tube)
    float lowerRodBottom = springStartY - lowerSeatThickness - bottomMountHeight - bottomPlateThickness;
    generateCylinder(lowerRodBottom, lowerRodTop, pistonRodRadius, 0.0f, false);
    
    // ============ 3. COIL SPRING (Helical spring around the piston rod) ============
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
    
    // ============ 4. UPPER SPRING SEAT ============
    generateCylinder(springEndY, springEndY + upperSeatThickness, upperSeatRadius, pistonRodRadius, true);
    
    // ============ 5. TOP COLLAR (below the nut) ============
    float collarBottom = springEndY + upperSeatThickness;
    float collarTop = collarBottom + topCollarHeight;
    generateCylinder(collarBottom, collarTop, topCollarRadius, pistonRodRadius, true);
    
    // ============ 6. TOP NUT WITH HORIZONTAL BARS (2 bars: left and right only) ============
    float nutBottom = collarTop;
    float nutTop = nutBottom + topNutHeight;
    float barLength = topNutRadius * 2.5f;  // Bars extend out left and right
    generateTopMountWithBars(nutBottom, nutTop, topNutRadius * 2.0f, barLength, topNutHeight * 0.6f);
    
    // ============ 7. TOP CAP (Small cylinder on top) ============
    float capBottom = nutTop;
    float capTop = capBottom + topCapHeight;
    generateCylinder(capBottom, capTop, topCapRadius, 0.0f, false);
    
    // ============ 8. BOTTOM DISC (Large circular plate before U-mount) ============
    float discHeight = springLength * 0.04f;
    float discRadius = lowerSeatRadius * 1.3f;  // Larger than spring seat
    float discTop = springStartY - lowerSeatThickness;
    float discBottom = discTop - discHeight;
    generateCylinder(discBottom, discTop, discRadius, pistonRodRadius, true);
    
    // ============ 9. BOTTOM U-MOUNT (Clevis/fork mount below disc) ============
    float uMountTop = discBottom;
    generateUMount(uMountTop, bottomMountHeight, bottomMountWidth, bottomMountThickness);
}

std::vector<ImVec2> ShockAbsorberModel3D::generateEnhancedSpringProfile() const {
    return std::vector<ImVec2>();
}
