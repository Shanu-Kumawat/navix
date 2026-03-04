#include <glm/glm.hpp>
#include "Base3DModel.hpp"
#include <iostream>
#include <unordered_map>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

Base3DModel::Base3DModel()
    : VAO(0), VBO(0), EBO(0),
      shader(nullptr),
      modelScale(1.0f),
      modelRotationX(0.0f),
      modelRotationY(0.0f),
      objectColor(0.8f, 0.8f, 0.8f),
      ambientStrength(0.4f),
      diffuseStrength(0.7f),
      specularStrength(0.7f),
      shininess(32.0f),
      lightPos(1.0f, 1.0f, 2.0f),
      lightColor(1.0f, 1.0f, 1.0f),
      renderMode(0),
      mouseDragging(false),
      lastX(0.0f),
      lastY(0.0f),
      showCrossSection(false),
      crossSectionAxis(0),
      crossSectionPos(0.0f),
      showFEMMesh(true),
      femElementSize(0.1f),
      meshWireVAO(0),
      meshWireVBO(0),
      meshWireVertexCount(0) {
    
    model = glm::mat4(1.0f);
}

Base3DModel::~Base3DModel() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
    if (meshWireVAO) glDeleteVertexArrays(1, &meshWireVAO);
    if (meshWireVBO) glDeleteBuffers(1, &meshWireVBO);
}

void Base3DModel::render(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos) {
    if (!shader || VAO == 0 || indices.empty()) return;
    
    shader->use();
    setCommonShaderUniforms(projection, view, cameraPos);
    
    glBindVertexArray(VAO);
    
    if (showCrossSection) {
        applyCrossSectionClipping();
    } else {
        glDisable(GL_CLIP_DISTANCE0);
    }
    
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Base3DModel::setShader(Shader* newShader) {
    shader = newShader;
}

void Base3DModel::setMaterial(const glm::vec3& color, float ambient, float diffuse, float specular, float shininessValue) {
    objectColor = color;
    ambientStrength = ambient;
    diffuseStrength = diffuse;
    specularStrength = specular;
    shininess = shininessValue;
}

void Base3DModel::setLight(const glm::vec3& position, const glm::vec3& color) {
    lightPos = position;
    lightColor = color;
}

void Base3DModel::setRenderMode(int mode) {
    renderMode = mode;
}

void Base3DModel::processMouseMovement(float xpos, float ypos) {
    if (mouseDragging) {
        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos;
        
        const float sensitivity = 0.5f;
        xoffset *= sensitivity;
        yoffset *= sensitivity;
        
        modelRotationY += xoffset;
        modelRotationX += yoffset;
        
        // Constrain pitch
        if (modelRotationX > 89.0f) modelRotationX = 89.0f;
        if (modelRotationX < -89.0f) modelRotationX = -89.0f;
        
        updateModelMatrix();
    }
    
    lastX = xpos;
    lastY = ypos;
}

void Base3DModel::processMousePress(bool pressed, float xpos, float ypos) {
    mouseDragging = pressed;
    lastX = xpos;
    lastY = ypos;
}

void Base3DModel::processMouseScroll(float yoffset) {
    modelScale += yoffset * 0.1f;
    if (modelScale < 0.1f) modelScale = 0.1f;
    if (modelScale > 10.0f) modelScale = 10.0f;
    updateModelMatrix();
}

void Base3DModel::resetView() {
    modelScale = 1.0f;
    modelRotationX = 0.0f;
    modelRotationY = 0.0f;
    updateModelMatrix();
}

void Base3DModel::setFrontView() {
    modelRotationX = 0.0f;
    modelRotationY = 0.0f;
    updateModelMatrix();
}

void Base3DModel::setSideView() {
    modelRotationX = 0.0f;
    modelRotationY = 90.0f;
    updateModelMatrix();
}

void Base3DModel::setTopView() {
    modelRotationX = -90.0f;
    modelRotationY = 0.0f;
    updateModelMatrix();
}

void Base3DModel::setIsometricView() {
    modelRotationX = -30.0f;
    modelRotationY = -45.0f;
    updateModelMatrix();
}

void Base3DModel::enableCrossSection(bool enable) {
    showCrossSection = enable;
}

void Base3DModel::setCrossSectionAxis(int axis) {
    crossSectionAxis = axis;
}

void Base3DModel::setCrossSectionPosition(float position) {
    crossSectionPos = position;
}

void Base3DModel::setupMesh() {
    if (vertices.empty() || indices.empty()) {
        std::cerr << "Warning: Attempting to setup mesh with no vertices or indices" << std::endl;
        return;
    }
    
    // Create buffers if they don't exist
    if (VAO == 0) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
    }
    
    // Bind and upload data
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    // Set vertex attribute pointers
    // Position attribute (first 3 floats)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute (next 3 floats)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
}

void Base3DModel::updateModelMatrix() {
    model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(modelScale));
    model = glm::rotate(model, glm::radians(modelRotationX), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(modelRotationY), glm::vec3(0.0f, 1.0f, 0.0f));
}

void Base3DModel::revolveProfile(const std::vector<glm::dvec2>& profile, int segments, float yOffset, float yScale) {
    if (profile.empty()) return;
    
    const float PI = 3.14159265359f;
    const float angleStep = 2.0f * PI / segments;
    size_t baseVertex = vertices.size() / 6; // 6 floats per vertex (pos + normal)
    
    // Generate vertices by revolving the profile
    for (const auto& point : profile) {
        float radius = point.x;
        float y = (point.y + yOffset) * yScale;
        
        for (int i = 0; i < segments; ++i) {
            float angle = i * angleStep;
            float x = radius * cos(angle);
            float z = radius * sin(angle);
            
            // Add vertex position
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            
            // Add placeholder normal (will be calculated later)
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
        }
    }
    
    // Generate indices for the revolved surface
    for (size_t i = 0; i < profile.size() - 1; ++i) {
        for (int j = 0; j < segments; ++j) {
            unsigned int curr = baseVertex + i * segments + j;
            unsigned int next = baseVertex + (i + 1) * segments + j;
            unsigned int currNext = baseVertex + i * segments + ((j + 1) % segments);
            unsigned int nextNext = baseVertex + (i + 1) * segments + ((j + 1) % segments);
            
            // First triangle
            indices.push_back(curr);
            indices.push_back(next);
            indices.push_back(currNext);
            
            // Second triangle
            indices.push_back(next);
            indices.push_back(nextNext);
            indices.push_back(currNext);
        }
    }
}

void Base3DModel::revolveProfileAroundX(const std::vector<glm::dvec2>& profile, int segments, float xOffset, float xScale) {
    if (profile.empty()) return;
    
    const float PI = 3.14159265359f;
    const float angleStep = 2.0f * PI / segments;
    size_t baseVertex = vertices.size() / 6; // 6 floats per vertex (pos + normal)
    
    // Generate vertices by revolving the profile around X-axis
    for (const auto& point : profile) {
        float radius = point.x;  // radius from X-axis
        float x = (point.y + xOffset) * xScale;  // position along X-axis
        
        for (int i = 0; i < segments; ++i) {
            float angle = i * angleStep;
            float y = radius * cos(angle);
            float z = radius * sin(angle);
            
            // Add vertex position
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            
            // Add placeholder normal (will be calculated later)
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
        }
    }
    
    // Generate indices for the revolved surface
    for (size_t i = 0; i < profile.size() - 1; ++i) {
        for (int j = 0; j < segments; ++j) {
            unsigned int curr = baseVertex + i * segments + j;
            unsigned int next = baseVertex + (i + 1) * segments + j;
            unsigned int currNext = baseVertex + i * segments + ((j + 1) % segments);
            unsigned int nextNext = baseVertex + (i + 1) * segments + ((j + 1) % segments);
            
            // First triangle
            indices.push_back(curr);
            indices.push_back(next);
            indices.push_back(currNext);
            
            // Second triangle
            indices.push_back(next);
            indices.push_back(nextNext);
            indices.push_back(currNext);
        }
    }
}

void Base3DModel::generateNormals() {
    if (vertices.empty() || indices.empty()) return;
    
    // Initialize all normals to zero
    for (size_t i = 0; i < vertices.size() / 6; ++i) {
        vertices[i * 6 + 3] = 0.0f;
        vertices[i * 6 + 4] = 0.0f;
        vertices[i * 6 + 5] = 0.0f;
    }
    
    // Calculate face normals and accumulate to vertex normals
    for (size_t i = 0; i < indices.size(); i += 3) {
        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];
        
        // Get vertex positions
        glm::vec3 v0(vertices[i0 * 6], vertices[i0 * 6 + 1], vertices[i0 * 6 + 2]);
        glm::vec3 v1(vertices[i1 * 6], vertices[i1 * 6 + 1], vertices[i1 * 6 + 2]);
        glm::vec3 v2(vertices[i2 * 6], vertices[i2 * 6 + 1], vertices[i2 * 6 + 2]);
        
        // Calculate face normal using cross product
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
        
        // Add to vertex normals
        vertices[i0 * 6 + 3] += normal.x;
        vertices[i0 * 6 + 4] += normal.y;
        vertices[i0 * 6 + 5] += normal.z;
        
        vertices[i1 * 6 + 3] += normal.x;
        vertices[i1 * 6 + 4] += normal.y;
        vertices[i1 * 6 + 5] += normal.z;
        
        vertices[i2 * 6 + 3] += normal.x;
        vertices[i2 * 6 + 4] += normal.y;
        vertices[i2 * 6 + 5] += normal.z;
    }
    
    // Normalize all vertex normals
    for (size_t i = 0; i < vertices.size() / 6; ++i) {
        glm::vec3 normal(vertices[i * 6 + 3], vertices[i * 6 + 4], vertices[i * 6 + 5]);
        if (glm::length(normal) > 0.0f) {
            normal = glm::normalize(normal);
            vertices[i * 6 + 3] = normal.x;
            vertices[i * 6 + 4] = normal.y;
            vertices[i * 6 + 5] = normal.z;
        }
    }
}

void Base3DModel::clearMesh() {
    vertices.clear();
    indices.clear();
}

void Base3DModel::applyCrossSectionClipping() {
    glm::vec4 clipPlane;
    switch (crossSectionAxis) {
        case 0: // X-axis
            clipPlane = glm::vec4(1.0f, 0.0f, 0.0f, -crossSectionPos);
            break;
        case 1: // Y-axis
            clipPlane = glm::vec4(0.0f, 1.0f, 0.0f, -crossSectionPos);
            break;
        case 2: // Z-axis
            clipPlane = glm::vec4(0.0f, 0.0f, 1.0f, -crossSectionPos);
            break;
        default:
            clipPlane = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
            break;
    }
    
    glEnable(GL_CLIP_DISTANCE0);
    shader->setVec4("clipPlane", clipPlane);
}

void Base3DModel::setCommonShaderUniforms(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos) {
    // Set matrices
    shader->setMat4("projection", projection);
    shader->setMat4("view", view);
    shader->setMat4("model", model);
    
    // Set material properties
    shader->setVec3("objectColor", objectColor);
    shader->setFloat("ambientStrength", ambientStrength);
    shader->setFloat("diffuseStrength", diffuseStrength);
    shader->setFloat("specularStrength", specularStrength);
    shader->setFloat("shininess", shininess);
    
    // Set light properties
    shader->setVec3("lightPos", lightPos);
    shader->setVec3("lightColor", lightColor);
    shader->setVec3("viewPos", cameraPos);
    
    // Set render mode
    shader->setInt("renderMode", renderMode);
}

// ============================================================================
// FEM Mesh Methods
// ============================================================================

bool Base3DModel::generateFEMMesh(float elementSize) {
    // Default implementation — derived classes override with Gmsh geometry
    return false;
}

void Base3DModel::clearFEMMesh() {
    femMesh.clear();
    if (meshWireVAO) { glDeleteVertexArrays(1, &meshWireVAO); meshWireVAO = 0; }
    if (meshWireVBO) { glDeleteBuffers(1, &meshWireVBO); meshWireVBO = 0; }
    meshWireVertexCount = 0;
    showFEMMesh = false;
}

void Base3DModel::setupMeshWireframeBuffers() {
    const auto& nodes = femMesh.getNodes();
    const auto& elements = femMesh.getElements();
    if (nodes.empty() || elements.empty()) return;

    // Build a tag→index lookup for fast node finding
    std::unordered_map<uint64_t, size_t> tagToIdx;
    tagToIdx.reserve(nodes.size());
    for (size_t i = 0; i < nodes.size(); ++i) {
        tagToIdx[nodes[i].tag] = i;
    }

    // Convert FEM mesh triangles into GL_LINES (edge pairs)
    std::vector<float> lineVerts;
    lineVerts.reserve(elements.size() * 6 * 6); // rough estimate

    for (const auto& elem : elements) {
        if (elem.nodeTags.size() < 2) continue;
        for (size_t i = 0; i < elem.nodeTags.size(); ++i) {
            size_t j = (i + 1) % elem.nodeTags.size();
            auto it1 = tagToIdx.find(elem.nodeTags[i]);
            auto it2 = tagToIdx.find(elem.nodeTags[j]);
            if (it1 == tagToIdx.end() || it2 == tagToIdx.end()) continue;
            const auto& n1 = nodes[it1->second];
            const auto& n2 = nodes[it2->second];
            // Vertex 1: pos + dummy normal
            lineVerts.push_back(static_cast<float>(n1.position.x));
            lineVerts.push_back(static_cast<float>(n1.position.y));
            lineVerts.push_back(static_cast<float>(n1.position.z));
            lineVerts.push_back(0.0f); lineVerts.push_back(0.0f); lineVerts.push_back(1.0f);
            // Vertex 2: pos + dummy normal
            lineVerts.push_back(static_cast<float>(n2.position.x));
            lineVerts.push_back(static_cast<float>(n2.position.y));
            lineVerts.push_back(static_cast<float>(n2.position.z));
            lineVerts.push_back(0.0f); lineVerts.push_back(0.0f); lineVerts.push_back(1.0f);
        }
    }

    meshWireVertexCount = static_cast<int>(lineVerts.size() / 6);
    if (meshWireVertexCount == 0) return;

    if (meshWireVAO == 0) {
        glGenVertexArrays(1, &meshWireVAO);
        glGenBuffers(1, &meshWireVBO);
    }
    glBindVertexArray(meshWireVAO);
    glBindBuffer(GL_ARRAY_BUFFER, meshWireVBO);
    glBufferData(GL_ARRAY_BUFFER, lineVerts.size() * sizeof(float), lineVerts.data(), GL_STATIC_DRAW);
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal attribute (dummy, needed by shader)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    std::cout << "[Base3DModel] FEM wireframe: " << meshWireVertexCount / 2 << " line segments." << std::endl;
}

void Base3DModel::renderFEMMeshWireframe(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos) {
    if (!showFEMMesh || !shader || meshWireVAO == 0 || meshWireVertexCount == 0) return;

    shader->use();
    shader->setMat4("projection", projection);
    shader->setMat4("view", view);
    shader->setMat4("model", model);
    // Bright green wireframe, fully ambient lit (no shading)
    shader->setVec3("objectColor", glm::vec3(0.0f, 1.0f, 0.4f));
    shader->setFloat("ambientStrength", 1.0f);
    shader->setFloat("diffuseStrength", 0.0f);
    shader->setFloat("specularStrength", 0.0f);
    shader->setFloat("shininess", 1.0f);
    shader->setVec3("lightPos", glm::vec3(0.0f));
    shader->setVec3("lightColor", glm::vec3(1.0f));
    shader->setVec3("viewPos", cameraPos);
    shader->setInt("renderMode", 0);
    shader->setVec4("clipPlane", glm::vec4(0.0f));
    glDisable(GL_CLIP_DISTANCE0);

    glBindVertexArray(meshWireVAO);
    glDrawArrays(GL_LINES, 0, meshWireVertexCount);
    glBindVertexArray(0);
}