#include <glm/glm.hpp>
#include "BellowsModel3D.hpp"
#include "meshing/GmshExtractor.hpp"
#include <iostream>
#include <cmath>

#ifdef USE_GMSH
#include <gmsh.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
    const std::vector<glm::dvec2>& profile = bellows->getCachedProfile();
    if (profile.empty()) return;
    
    // Convert bellows profile to 3D revolve format for horizontal orientation
    // The bellows profile has: x = axial position, y = radius
    // For horizontal bellows: x = axial position (along X-axis), y = radius (for revolution around X-axis)
    std::vector<glm::dvec2> bellowsProfile;
    for (const auto& point : profile) {
        // Keep x as axial position (horizontal axis), y as radius
        // Scale down by a reasonable factor to fit in the 3D viewport
        bellowsProfile.push_back(glm::dvec2(point.y / 100.0f, point.x / 100.0f));
    }
    
    // Revolve profile around X-axis to create horizontal 3D geometry
    // Use higher segment count for bellows (72 segments for smooth curves)
    revolveProfileAroundX(bellowsProfile, 72);
    
    // Debug: print visual geometry bounding box
    {
        float xMin = 1e30f, xMax = -1e30f, yMin = 1e30f, yMax = -1e30f, zMin = 1e30f, zMax = -1e30f;
        for (size_t i = 0; i < vertices.size() / 6; ++i) {
            float vx = vertices[i*6+0], vy = vertices[i*6+1], vz = vertices[i*6+2];
            xMin = std::min(xMin, vx); xMax = std::max(xMax, vx);
            yMin = std::min(yMin, vy); yMax = std::max(yMax, vy);
            zMin = std::min(zMin, vz); zMax = std::max(zMax, vz);
        }
        static bool printed = false;
        if (!printed) {
            std::cout << "[BellowsModel3D] Visual geometry bbox: X[" << xMin << ".." << xMax
                      << "] Y[" << yMin << ".." << yMax
                      << "] Z[" << zMin << ".." << zMax << "]"
                      << " (" << vertices.size()/6 << " verts)" << std::endl;
            printed = true;
        }
    }
}

bool BellowsModel3D::generateFEMMesh(float elementSize) {
    if (!currentBellows) return false;

    clearFEMMesh();
    femElementSize = elementSize;

    const auto& rawProfile = currentBellows->getCachedProfile();
    if (rawProfile.size() < 4) return false;

    // Apply the SAME coordinate transform as the visualization:
    // visualization does: dvec2(point.y / 100, point.x / 100)
    // then revolveProfileAroundX uses: radius = profile.x, axial = profile.y
    // So for each raw point: axial = raw.x / 100, radius = raw.y / 100
    // World coords after revolve: (axial, radius*cos(θ), radius*sin(θ))

    struct ProfilePt { double axial; double radius; };
    std::vector<ProfilePt> fullProfile;
    for (size_t i = 0; i < rawProfile.size(); ++i) {
        double axial  = rawProfile[i].x / 100.0;
        double radius = rawProfile[i].y / 100.0;
        if (radius < 0) radius = -radius;
        fullProfile.push_back({axial, radius});
    }

    // Remove closing duplicate point (last==first in closed polygon)
    if (fullProfile.size() > 2) {
        double dx = fullProfile.back().axial - fullProfile.front().axial;
        double dy = fullProfile.back().radius - fullProfile.front().radius;
        if (std::sqrt(dx * dx + dy * dy) < 1e-4) {
            fullProfile.pop_back();
        }
    }

    // Subsample profile based on element size to control mesh density.
    // Keep minimum distance proportional to element size.
    double minDist = std::max(static_cast<double>(elementSize) * 0.5, 0.003);
    std::vector<ProfilePt> profile;
    profile.push_back(fullProfile.front());
    for (size_t i = 1; i < fullProfile.size(); ++i) {
        double dx = fullProfile[i].axial - profile.back().axial;
        double dy = fullProfile[i].radius - profile.back().radius;
        if (std::sqrt(dx * dx + dy * dy) >= minDist) {
            profile.push_back(fullProfile[i]);
        }
    }
    // Always include the last point
    {
        double dx = fullProfile.back().axial - profile.back().axial;
        double dy = fullProfile.back().radius - profile.back().radius;
        if (std::sqrt(dx * dx + dy * dy) > 1e-6) {
            profile.push_back(fullProfile.back());
        }
    }

    if (profile.size() < 2) return false;

    // Determine angular resolution: aim for element size along circumference
    // Use average radius to determine number of angular segments
    double avgRadius = 0.0;
    for (const auto& p : profile) avgRadius += p.radius;
    avgRadius /= profile.size();
    double circumference = 2.0 * M_PI * avgRadius;
    int numAngularSegments = std::max(12, static_cast<int>(circumference / elementSize));
    // Cap at reasonable maximum
    numAngularSegments = std::min(numAngularSegments, 120);

    std::cout << "[BellowsModel3D] Procedural FEM mesh: " << profile.size()
              << " profile pts, " << numAngularSegments << " angular segments" << std::endl;

    // Generate mesh nodes by revolving profile around X-axis
    // Node layout: profile_pts × angular_segments grid
    size_t nProfile = profile.size();
    size_t nAngle = static_cast<size_t>(numAngularSegments);
    double angleStep = 2.0 * M_PI / nAngle;

    // Create nodes
    uint64_t nodeTag = 1;
    for (size_t pi = 0; pi < nProfile; ++pi) {
        double axial = profile[pi].axial;
        double r = profile[pi].radius;
        for (size_t ai = 0; ai < nAngle; ++ai) {
            double theta = ai * angleStep;
            double x = axial;
            double y = r * std::cos(theta);
            double z = r * std::sin(theta);
            femMesh.addNode(nodeTag++, x, y, z);
        }
    }

    // Create triangular elements connecting adjacent rings
    // Element type 2 = 3-node triangle in Gmsh convention
    uint64_t elemTag = 1;
    for (size_t pi = 0; pi + 1 < nProfile; ++pi) {
        for (size_t ai = 0; ai < nAngle; ++ai) {
            size_t aiNext = (ai + 1) % nAngle;

            // Node tags for the quad formed by two adjacent profile points and two angular positions
            uint64_t n00 = pi * nAngle + ai + 1;         // current profile, current angle
            uint64_t n10 = (pi + 1) * nAngle + ai + 1;   // next profile, current angle
            uint64_t n01 = pi * nAngle + aiNext + 1;      // current profile, next angle
            uint64_t n11 = (pi + 1) * nAngle + aiNext + 1; // next profile, next angle

            // Triangle 1
            femMesh.addElement(elemTag++, 2, {n00, n10, n01});
            // Triangle 2
            femMesh.addElement(elemTag++, 2, {n10, n11, n01});
        }
    }

    if (!femMesh.isEmpty()) {
        showFEMMesh = true;
        setupMeshWireframeBuffers();
        std::cout << "[BellowsModel3D] FEM mesh: " << femMesh.getNodes().size()
                  << " nodes, " << femMesh.getElements().size() << " elements." << std::endl;
        return true;
    }
    return false;
}
