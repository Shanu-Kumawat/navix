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
}

bool BellowsModel3D::generateFEMMesh(float elementSize) {
#ifdef USE_GMSH
    if (!currentBellows) return false;

    clearFEMMesh();
    femElementSize = elementSize;

    const auto& rawProfile = currentBellows->getCachedProfile();
    if (rawProfile.size() < 2) return false;

    // Apply the SAME coordinate transform as the visualization:
    // visualization does: dvec2(point.y / 100, point.x / 100)
    // then revolveProfileAroundX uses: radius = profile.x, axial = profile.y
    // So: radius = raw.y / 100, axial = raw.x / 100
    // For Gmsh revolve around X-axis: point at (axial, radius, 0)

    // Build transformed + de-duplicated profile
    struct ProfilePt { double axial; double radius; };
    std::vector<ProfilePt> profile;
    for (size_t i = 0; i < rawProfile.size(); ++i) {
        double axial  = rawProfile[i].x / 100.0;
        double radius = rawProfile[i].y / 100.0;
        if (radius < 0) radius = -radius; // radius must be non-negative for revolve
        if (!profile.empty()) {
            double dx = axial - profile.back().axial;
            double dy = radius - profile.back().radius;
            double dist = std::sqrt(dx * dx + dy * dy);
            if (dist < 1e-6) continue; // skip near-duplicate points
        }
        profile.push_back({axial, radius});
    }

    std::cout << "[BellowsModel3D] Profile: " << profile.size() << " unique points." << std::endl;
    if (profile.size() < 2) return false;

    try {
        gmsh::initialize();
        gmsh::option::setNumber("General.Terminal", 0);
        gmsh::model::add("Bellows3D");

        // Create profile points: (axial_pos, radius, 0) — revolve around X-axis
        std::vector<int> pointTags;
        for (size_t i = 0; i < profile.size(); ++i) {
            int tag = gmsh::model::occ::addPoint(profile[i].axial, profile[i].radius, 0.0, elementSize);
            pointTags.push_back(tag);
        }

        // Create lines connecting consecutive profile points
        std::vector<int> lineTags;
        for (size_t i = 0; i + 1 < pointTags.size(); ++i) {
            try {
                int tag = gmsh::model::occ::addLine(pointTags[i], pointTags[i + 1]);
                lineTags.push_back(tag);
            } catch (const std::exception&) {
                // Skip degenerate lines (coincident points after OCC tolerance)
            }
        }

        if (lineTags.empty()) {
            std::cerr << "[BellowsModel3D] No valid lines created." << std::endl;
            gmsh::finalize();
            return false;
        }

        // Create a wire from the lines
        int wireTag = gmsh::model::occ::addWire(lineTags);

        // Revolve the wire around X-axis to create the bellows surface
        gmsh::vectorpair inDimTags = {{1, wireTag}};
        gmsh::vectorpair outDimTags;
        gmsh::model::occ::revolve(inDimTags, 0, 0, 0, 1, 0, 0, 2.0 * M_PI, outDimTags);

        gmsh::model::occ::synchronize();

        // Set mesh size constraints
        gmsh::option::setNumber("Mesh.CharacteristicLengthMin", elementSize * 0.5);
        gmsh::option::setNumber("Mesh.CharacteristicLengthMax", elementSize);

        // Generate surface mesh
        gmsh::model::mesh::generate(2);

        // Extract mesh
        Core::Meshing::GmshExtractor extractor;
        bool ok = extractor.extractMesh(femMesh, 2);
        gmsh::finalize();

        if (ok && !femMesh.isEmpty()) {
            showFEMMesh = true;
            setupMeshWireframeBuffers();
            std::cout << "[BellowsModel3D] FEM mesh: " << femMesh.getNodes().size()
                      << " nodes, " << femMesh.getElements().size() << " elements." << std::endl;
            return true;
        } else {
            std::cerr << "[BellowsModel3D] Mesh extraction failed or mesh is empty." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[BellowsModel3D] FEM mesh failed: " << e.what() << std::endl;
        try { gmsh::finalize(); } catch (...) {}
    }
    return false;
#else
    std::cerr << "[BellowsModel3D] Built without USE_GMSH." << std::endl;
    return false;
#endif
}
