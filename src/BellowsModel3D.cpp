#include <glm/glm.hpp>
#include "BellowsModel3D.hpp"
#include "meshing/GmshExtractor.hpp"
#include <iostream>
#include <cmath>
#include <unordered_map>

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

    // Subsample profile based on element size to control mesh density,
    // with **curvature-adaptive density** to pack elements tightly at critical bends.
    std::vector<ProfilePt> profile;
    profile.push_back(fullProfile.front());
    
    for (size_t i = 1; i < fullProfile.size() - 1; ++i) {
        double dx = fullProfile[i].axial - profile.back().axial;
        double dy = fullProfile[i].radius - profile.back().radius;
        double dist = std::sqrt(dx * dx + dy * dy);
        
        // Calculate local curvature using neighboring points
        double prev_dx = fullProfile[i].axial - fullProfile[i-1].axial;
        double prev_dy = fullProfile[i].radius - fullProfile[i-1].radius;
        double next_dx = fullProfile[i+1].axial - fullProfile[i].axial;
        double next_dy = fullProfile[i+1].radius - fullProfile[i].radius;
        
        double len_prev = std::sqrt(prev_dx*prev_dx + prev_dy*prev_dy);
        double len_next = std::sqrt(next_dx*next_dx + next_dy*next_dy);
        
        double curvature = 0.0;
        if (len_prev > 1e-6 && len_next > 1e-6) {
            // Dot product to find angle change
            double dot = (prev_dx*next_dx + prev_dy*next_dy) / (len_prev * len_next);
            // Clamp to [-1, 1] to avoid acos domain errors
            dot = std::max(-1.0, std::min(1.0, dot));
            curvature = std::acos(dot); // Angle in radians
        }

        // Base element size, scale down heavily if there is high curvature (tight corners)
        double adaptiveDist = std::max(static_cast<double>(elementSize) * 0.5, 0.003);
        if (curvature > 0.05) { // If angle changes by more than ~3 degrees
            adaptiveDist *= 0.25; // Pack them 4x denser
            adaptiveDist = std::max(adaptiveDist, 0.0005); // Absolute minimum limit
        }

        if (dist >= adaptiveDist) {
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

    uint64_t elemTag = 1;
    for (size_t pi = 0; pi + 1 < nProfile; ++pi) {
        for (size_t ai = 0; ai < nAngle; ++ai) {
            size_t aiNext = (ai + 1) % nAngle;

            uint64_t n00 = pi * nAngle + ai + 1;
            uint64_t n10 = (pi + 1) * nAngle + ai + 1;
            uint64_t n01 = pi * nAngle + aiNext + 1;
            uint64_t n11 = (pi + 1) * nAngle + aiNext + 1;

            if (meshType == MeshType::QUADRILATERAL) {
                // Quad element (type 3)
                femMesh.addElement(elemTag++, 3, {n00, n10, n11, n01});
            } else {
                // Triangular elements (type 2)
                femMesh.addElement(elemTag++, 2, {n00, n10, n01});
                femMesh.addElement(elemTag++, 2, {n10, n11, n01});
            }
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

// ─── FEM Analysis ───────────────────────────────────────────────────

bool BellowsModel3D::runFEMAnalysis(const Core::FEM::AnalysisConfig& config) {
    if (!currentBellows || femMesh.isEmpty()) {
        std::cerr << "[BellowsModel3D] Need a bellows and surface mesh before FEM.\n";
        return false;
    }

    clearFEMResult();

    Core::FEM::BellowsFEMAnalysis analysis;
    bool ok = analysis.run(femMesh,
                           static_cast<double>(currentBellows->wallThickness),
                           femMaterial, config);
    if (ok) {
        femResult = analysis.getResult();
        setupStressBuffers();
    }
    return ok;
}

// ─── Stress contour rendering helpers ───────────────────────────────

glm::vec3 BellowsModel3D::stressColor(double value, double minVal, double maxVal) const {
    // Jet colormap: blue → cyan → green → yellow → red
    double range = maxVal - minVal;
    double t = (range > 1e-30) ? (value - minVal) / range : 0.5;
    t = std::max(0.0, std::min(1.0, t));

    float r, g, b;
    if (t < 0.25) {
        float s = static_cast<float>(t / 0.25);
        r = 0.0f; g = s; b = 1.0f;
    } else if (t < 0.5) {
        float s = static_cast<float>((t - 0.25) / 0.25);
        r = 0.0f; g = 1.0f; b = 1.0f - s;
    } else if (t < 0.75) {
        float s = static_cast<float>((t - 0.5) / 0.25);
        r = s; g = 1.0f; b = 0.0f;
    } else {
        float s = static_cast<float>((t - 0.75) / 0.25);
        r = 1.0f; g = 1.0f - s; b = 0.0f;
    }
    return glm::vec3(r, g, b);
}

void BellowsModel3D::cleanupStressBuffers() {
    if (stressVAO) { glDeleteVertexArrays(1, &stressVAO); stressVAO = 0; }
    if (stressVBO) { glDeleteBuffers(1, &stressVBO); stressVBO = 0; }
    stressVertexCount = 0;
}

void BellowsModel3D::setupStressBuffers() {
    cleanupStressBuffers();
    if (!femResult.isValid || femResult.nodeVonMises.empty()) return;

    const auto& nodes = femMesh.getNodes();
    const auto& elems = femMesh.getElements();

    // Build tag → index map
    std::unordered_map<uint64_t, int> tagIdx;
    for (int i = 0; i < static_cast<int>(nodes.size()); ++i)
        tagIdx[nodes[i].tag] = i;

    double sMin = femResult.minVonMises;
    double sMax = femResult.maxVonMises;

    // Vertices: [x, y, z, r, g, b] per vertex, 3 vertices per triangle
    std::vector<float> verts;
    verts.reserve(elems.size() * 3 * 6);

    for (const auto& e : elems) {
        if (e.elementType != 2 || e.nodeTags.size() != 3) continue;
        for (int n = 0; n < 3; ++n) {
            auto it = tagIdx.find(e.nodeTags[n]);
            if (it == tagIdx.end()) continue;
            int idx = it->second;
            const auto& nd = nodes[idx];
            // Position (mesh coordinates — same space as wireframe)
            verts.push_back(static_cast<float>(nd.position.x));
            verts.push_back(static_cast<float>(nd.position.y));
            verts.push_back(static_cast<float>(nd.position.z));
            // Color from node von Mises
            double vm = (idx < static_cast<int>(femResult.nodeVonMises.size()))
                        ? femResult.nodeVonMises[idx] : 0.0;
            glm::vec3 c = stressColor(vm, sMin, sMax);
            verts.push_back(c.r);
            verts.push_back(c.g);
            verts.push_back(c.b);
        }
    }

    stressVertexCount = static_cast<int>(verts.size() / 6);
    if (stressVertexCount == 0) return;

    glGenVertexArrays(1, &stressVAO);
    glGenBuffers(1, &stressVBO);

    glBindVertexArray(stressVAO);
    glBindBuffer(GL_ARRAY_BUFFER, stressVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(verts.size() * sizeof(float)),
                 verts.data(), GL_STATIC_DRAW);

    // Position attrib (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Color attrib (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void BellowsModel3D::renderStressContours(const glm::mat4& projection,
                                           const glm::mat4& view,
                                           const glm::vec3& cameraPos) {
    if (!showStressContours || stressVertexCount == 0 || !shader) return;

    shader->use();
    shader->setMat4("projection", projection);
    shader->setMat4("view", view);
    shader->setMat4("model", model);
    shader->setVec3("viewPos", cameraPos);
    shader->setVec3("lightPos", lightPos);
    shader->setVec3("lightColor", lightColor);
    // Render with per-vertex color flag
    shader->setInt("useVertexColor", 1);
    shader->setFloat("ambientStrength", 0.5f);
    shader->setFloat("diffuseStrength", 0.6f);
    shader->setFloat("specularStrength", 0.1f);
    shader->setFloat("shininess", 16.0f);

    glBindVertexArray(stressVAO);
    glDrawArrays(GL_TRIANGLES, 0, stressVertexCount);
    glBindVertexArray(0);

    // Reset vertex color flag
    shader->setInt("useVertexColor", 0);
}

// ─── Clear FEM result ───────────────────────────────────────────────

void BellowsModel3D::clearFEMResult() {
    std::cout << "[DEBUG] clearFEMResult called. Was valid=" << femResult.isValid << std::endl;
    femResult = Core::FEM::FEMResult();
    showStressContours = false;
    showDeformed = false;
    cleanupStressBuffers();
    cleanupDeformedBuffers();
    std::cout << "[DEBUG] clearFEMResult done. Now valid=" << femResult.isValid << std::endl;
}

// ─── Deformed shape overlay ─────────────────────────────────────────

void BellowsModel3D::cleanupDeformedBuffers() {
    if (deformedVAO) { glDeleteVertexArrays(1, &deformedVAO); deformedVAO = 0; }
    if (deformedVBO) { glDeleteBuffers(1, &deformedVBO); deformedVBO = 0; }
    deformedVertexCount = 0;
}

void BellowsModel3D::setupDeformedBuffers() {
    cleanupDeformedBuffers();
    if (!femResult.isValid || femResult.displacements.size() == 0) return;

    const auto& nodes = femMesh.getNodes();
    const auto& elems = femMesh.getElements();

    std::unordered_map<uint64_t, int> tagIdx;
    for (int i = 0; i < static_cast<int>(nodes.size()); ++i)
        tagIdx[nodes[i].tag] = i;

    const double meshToMeters = 0.1;
    // Scale factor: deformScale * (mesh units / meters)
    double dispToMesh = deformScale / meshToMeters;

    // Use a translucent cyan wireframe for the deformed shape
    std::vector<float> verts;
    verts.reserve(elems.size() * 3 * 6);

    for (const auto& e : elems) {
        if (e.elementType != 2 || e.nodeTags.size() != 3) continue;
        for (int n = 0; n < 3; ++n) {
            auto it = tagIdx.find(e.nodeTags[n]);
            if (it == tagIdx.end()) continue;
            int idx = it->second;
            const auto& nd = nodes[idx];
            int base = idx * 6;
            double ux = femResult.displacements(base + 0) * dispToMesh;
            double uy = femResult.displacements(base + 1) * dispToMesh;
            double uz = femResult.displacements(base + 2) * dispToMesh;
            verts.push_back(static_cast<float>(nd.position.x + ux));
            verts.push_back(static_cast<float>(nd.position.y + uy));
            verts.push_back(static_cast<float>(nd.position.z + uz));
            // Cyan color for deformed shape
            verts.push_back(0.0f);
            verts.push_back(0.9f);
            verts.push_back(1.0f);
        }
    }

    deformedVertexCount = static_cast<int>(verts.size() / 6);
    if (deformedVertexCount == 0) return;

    glGenVertexArrays(1, &deformedVAO);
    glGenBuffers(1, &deformedVBO);
    glBindVertexArray(deformedVAO);
    glBindBuffer(GL_ARRAY_BUFFER, deformedVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(verts.size() * sizeof(float)),
                 verts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void BellowsModel3D::renderDeformed(const glm::mat4& projection,
                                     const glm::mat4& view,
                                     const glm::vec3& cameraPos) {
    if (!showDeformed || deformedVertexCount == 0 || !shader) return;

    shader->use();
    shader->setMat4("projection", projection);
    shader->setMat4("view", view);
    shader->setMat4("model", model);
    shader->setVec3("viewPos", cameraPos);
    shader->setVec3("lightPos", lightPos);
    shader->setVec3("lightColor", lightColor);
    shader->setInt("useVertexColor", 1);
    shader->setFloat("ambientStrength", 0.7f);
    shader->setFloat("diffuseStrength", 0.4f);
    shader->setFloat("specularStrength", 0.1f);
    shader->setFloat("shininess", 16.0f);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBindVertexArray(deformedVAO);
    glDrawArrays(GL_TRIANGLES, 0, deformedVertexCount);
    glBindVertexArray(0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    shader->setInt("useVertexColor", 0);
}

// ─── Modal analysis ─────────────────────────────────────────────────

bool BellowsModel3D::runModalAnalysis(int numModes) {
    if (!currentBellows || femMesh.isEmpty()) {
        std::cerr << "[BellowsModel3D] Need bellows and mesh before modal analysis.\n";
        return false;
    }

    clearModalResult();

    Core::FEM::BellowsFEMAnalysis analysis;
    bool ok = analysis.runModal(femMesh,
                                static_cast<double>(currentBellows->wallThickness),
                                femMaterial, numModes, modalResult);
    if (ok && !modalResult.modes.empty()) {
        setActiveMode(0);
    }
    return ok;
}

void BellowsModel3D::setActiveMode(int modeIdx) {
    if (!modalResult.isValid || modeIdx < 0 ||
        modeIdx >= static_cast<int>(modalResult.modes.size())) {
        activeMode = -1;
        cleanupModalBuffers();
        return;
    }
    activeMode = modeIdx;
    setupModalBuffers();
}

void BellowsModel3D::cleanupModalBuffers() {
    if (modalVAO) { glDeleteVertexArrays(1, &modalVAO); modalVAO = 0; }
    if (modalVBO) { glDeleteBuffers(1, &modalVBO); modalVBO = 0; }
    modalVertexCount = 0;
}

void BellowsModel3D::setupModalBuffers() {
    cleanupModalBuffers();
    if (!modalResult.isValid || activeMode < 0 ||
        activeMode >= static_cast<int>(modalResult.modes.size())) return;

    const auto& mode = modalResult.modes[activeMode];
    const auto& nodes = femMesh.getNodes();
    const auto& elems = femMesh.getElements();

    std::unordered_map<uint64_t, int> tagIdx;
    for (int i = 0; i < static_cast<int>(nodes.size()); ++i)
        tagIdx[nodes[i].tag] = i;

    // Scale mode shape to be visible (mode vector is max-normalized to 1)
    // Use a fraction of the bounding box diagonal as scale
    float scale = 0.3f; // 30% of max mesh dimension

    std::vector<float> verts;
    verts.reserve(elems.size() * 3 * 6);

    for (const auto& e : elems) {
        if (e.elementType != 2 || e.nodeTags.size() != 3) continue;
        for (int n = 0; n < 3; ++n) {
            auto it = tagIdx.find(e.nodeTags[n]);
            if (it == tagIdx.end()) continue;
            int idx = it->second;
            const auto& nd = nodes[idx];
            int base = idx * 6;
            double ux = mode.modeVector(base + 0) * scale;
            double uy = mode.modeVector(base + 1) * scale;
            double uz = mode.modeVector(base + 2) * scale;

            // Displacement magnitude for coloring
            double mag = std::sqrt(ux*ux + uy*uy + uz*uz);
            glm::vec3 c = stressColor(mag, 0, scale);

            verts.push_back(static_cast<float>(nd.position.x + ux));
            verts.push_back(static_cast<float>(nd.position.y + uy));
            verts.push_back(static_cast<float>(nd.position.z + uz));
            verts.push_back(c.r);
            verts.push_back(c.g);
            verts.push_back(c.b);
        }
    }

    modalVertexCount = static_cast<int>(verts.size() / 6);
    if (modalVertexCount == 0) return;

    glGenVertexArrays(1, &modalVAO);
    glGenBuffers(1, &modalVBO);
    glBindVertexArray(modalVAO);
    glBindBuffer(GL_ARRAY_BUFFER, modalVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(verts.size() * sizeof(float)),
                 verts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void BellowsModel3D::renderModeShape(const glm::mat4& projection,
                                      const glm::mat4& view,
                                      const glm::vec3& cameraPos) {
    if (activeMode < 0 || modalVertexCount == 0 || !shader) return;

    shader->use();
    shader->setMat4("projection", projection);
    shader->setMat4("view", view);
    shader->setMat4("model", model);
    shader->setVec3("viewPos", cameraPos);
    shader->setVec3("lightPos", lightPos);
    shader->setVec3("lightColor", lightColor);
    shader->setInt("useVertexColor", 1);
    shader->setFloat("ambientStrength", 0.6f);
    shader->setFloat("diffuseStrength", 0.5f);
    shader->setFloat("specularStrength", 0.1f);
    shader->setFloat("shininess", 16.0f);

    glBindVertexArray(modalVAO);
    glDrawArrays(GL_TRIANGLES, 0, modalVertexCount);
    glBindVertexArray(0);

    shader->setInt("useVertexColor", 0);
}

// ─── Mesh convergence study ─────────────────────────────────────────

bool BellowsModel3D::runConvergenceStudy(const Core::FEM::AnalysisConfig& config, int numPoints) {
    if (!currentBellows) return false;

    convergenceData.clear();

    // Element sizes from coarse to fine
    float sizes[] = {0.15f, 0.10f, 0.07f, 0.05f, 0.035f, 0.025f};
    int count = std::min(numPoints, 6);

    std::cout << "[Convergence] Running " << count << " mesh refinement levels..." << std::endl;

    // Save current FEM mesh state
    Core::Meshing::Mesh savedMesh = femMesh;
    bool hadMesh = !femMesh.isEmpty();

    for (int i = 0; i < count; ++i) {
        float es = sizes[i];

        // Generate mesh at this element size
        clearFEMMesh();
        if (!generateFEMMesh(es)) {
            std::cout << "[Convergence] Failed to generate mesh at element size " << es << std::endl;
            continue;
        }

        // Run FEM analysis
        Core::FEM::BellowsFEMAnalysis analysis;
        bool ok = analysis.run(femMesh,
                               static_cast<double>(currentBellows->wallThickness),
                               femMaterial, config);

        if (ok) {
            const auto& res = analysis.getResult();
            Core::FEM::ConvergencePoint pt;
            pt.elementSize = es;
            pt.numNodes = static_cast<int>(femMesh.getNodes().size());
            pt.numElements = static_cast<int>(femMesh.getElements().size());
            pt.maxStress = res.maxVonMises / 1e6;
            pt.maxDisplacement = res.maxDisplacement * 1e3;
            convergenceData.push_back(pt);

            std::cout << "  h=" << es << ": " << pt.numNodes << " nodes, "
                      << pt.numElements << " elems, σ_max=" << pt.maxStress
                      << " MPa, u_max=" << pt.maxDisplacement << " mm" << std::endl;
        }
    }

    // Restore original mesh
    clearFEMMesh();
    femMesh = savedMesh;
    if (!femMesh.isEmpty()) {
        showFEMMesh = true;
        setupMeshWireframeBuffers();
    }

    std::cout << "[Convergence] Study complete: " << convergenceData.size() << " data points." << std::endl;
    return !convergenceData.empty();
}
