#include "BellowsModel3D.hpp"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

BellowsModel3D::BellowsModel3D() 
    : VAO(0), VBO(0), EBO(0), 
      shader(nullptr), 
      modelScale(1.0f), 
      modelRotationX(0.0f), 
      modelRotationY(0.0f),
      objectColor(0.8f, 0.8f, 0.8f), // Metallic gray color
      ambientStrength(0.3f),
      diffuseStrength(0.7f),
      specularStrength(0.5f),
      shininess(32.0f),
      lightPos(1.0f, 1.0f, 2.0f),
      lightColor(1.0f, 1.0f, 1.0f),
      renderMode(0),
      mouseDragging(false),
      lastX(0.0f), 
      lastY(0.0f),
      showCrossSection(false),
      crossSectionAxis(0),
      crossSectionPos(0.0f) {
    
    model = glm::mat4(1.0f);
}

BellowsModel3D::~BellowsModel3D() {
    // Clean up buffers
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
}

void BellowsModel3D::generateMesh(const Drawing::Bellows* bellows) {
    if (!bellows) return;
    
    // Get 2D profile from bellows
    std::vector<ImVec2> profile = bellows->generateProfile();
    
    // Clear any existing mesh data
    vertices.clear();
    indices.clear();
    
    // Generate 3D mesh by revolving the 2D profile
    int segments = 36; // Number of segments in 360 degrees
    revolveProfile(profile, segments);
    
    // Calculate normals
    generateNormals();
    
    // Setup OpenGL buffers
    setupMesh();
    
    // Reset view
    resetView();
}

void BellowsModel3D::revolveProfile(const std::vector<ImVec2>& profile, int segments) {
    if (profile.empty()) return;
    
    // For each profile point, generate a ring of vertices
    const float PI = 3.14159265359f;
    const float angleStep = 2.0f * PI / segments;
    
    // Find center/bounds of profile for scaling
    float minX = profile[0].x;
    float maxX = profile[0].x;
    float minY = profile[0].y;
    float maxY = profile[0].y;
    
    for (const auto& point : profile) {
        minX = std::min(minX, point.x);
        maxX = std::max(maxX, point.x);
        minY = std::min(minY, point.y);
        maxY = std::max(maxY, point.y);
    }
    
    float scale = 1.0f / std::max(maxX - minX, maxY - minY);
    float xOffset = (maxX + minX) / 2.0f;
    
    // Generate vertices
    for (const auto& point : profile) {
        // Normalize coordinates to [-1, 1] range for OpenGL
        float x = (point.x - xOffset) * scale;
        float y = point.y * scale;
        
        // Generate vertices around the ring
        for (int i = 0; i < segments; ++i) {
            float angle = i * angleStep;
            
            // Position (x, r*cos(θ), r*sin(θ))
            // Where r = y (the radius)
            float vx = x;
            float vy = y * cos(angle);
            float vz = y * sin(angle);
            
            // Add vertex to array (position only, normals calculated later)
            vertices.push_back(vx);
            vertices.push_back(vy);
            vertices.push_back(vz);
            
            // Add placeholder for normal vector
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
        }
    }
    
    // Generate indices for triangles
    for (size_t i = 0; i < profile.size() - 1; ++i) {
        for (int j = 0; j < segments; ++j) {
            // Calculate vertex indices
            unsigned int current = i * segments + j;
            unsigned int next = i * segments + (j + 1) % segments;
            unsigned int currentNext = (i + 1) * segments + j;
            unsigned int nextNext = (i + 1) * segments + (j + 1) % segments;
            
            // First triangle (current, next, currentNext)
            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(currentNext);
            
            // Second triangle (next, nextNext, currentNext)
            indices.push_back(next);
            indices.push_back(nextNext);
            indices.push_back(currentNext);
        }
    }
}

void BellowsModel3D::generateNormals() {
    // Reset all normals to zero
    for (size_t i = 0; i < vertices.size() / 6; ++i) {
        vertices[i * 6 + 3] = 0.0f;
        vertices[i * 6 + 4] = 0.0f;
        vertices[i * 6 + 5] = 0.0f;
    }
    
    // Calculate normals for each face and accumulate
    for (size_t i = 0; i < indices.size(); i += 3) {
        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];
        
        // Get positions of the three vertices
        glm::vec3 v0(vertices[i0 * 6], vertices[i0 * 6 + 1], vertices[i0 * 6 + 2]);
        glm::vec3 v1(vertices[i1 * 6], vertices[i1 * 6 + 1], vertices[i1 * 6 + 2]);
        glm::vec3 v2(vertices[i2 * 6], vertices[i2 * 6 + 1], vertices[i2 * 6 + 2]);
        
        // Calculate face normal using cross product
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
        
        // Add to the normals of all three vertices
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
        normal = glm::normalize(normal);
        
        vertices[i * 6 + 3] = normal.x;
        vertices[i * 6 + 4] = normal.y;
        vertices[i * 6 + 5] = normal.z;
    }
}

void BellowsModel3D::setupMesh() {
    // Create buffers if they don't exist
    if (VAO == 0) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
    }
    
    // Bind buffers and upload data
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    // Set vertex attribute pointers
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Unbind VAO
    glBindVertexArray(0);
}

void BellowsModel3D::render(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos) {
    if (!shader || VAO == 0 || indices.empty()) return;
    
    // Use shader
    shader->use();
    
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
    
    // Draw mesh
    glBindVertexArray(VAO);
    
    if (showCrossSection) {
        // Setup cross-section clipping plane
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
        }
        
        glEnable(GL_CLIP_DISTANCE0);
        shader->setVec4("clipPlane", clipPlane);
    } else {
        glDisable(GL_CLIP_DISTANCE0);
    }
    
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void BellowsModel3D::setShader(Shader* shader) {
    this->shader = shader;
}

void BellowsModel3D::setMaterial(const glm::vec3& color, float ambient, float diffuse, float specular, float shininess) {
    this->objectColor = color;
    this->ambientStrength = ambient;
    this->diffuseStrength = diffuse;
    this->specularStrength = specular;
    this->shininess = shininess;
}

void BellowsModel3D::setLight(const glm::vec3& position, const glm::vec3& color) {
    this->lightPos = position;
    this->lightColor = color;
}

void BellowsModel3D::setRenderMode(int mode) {
    this->renderMode = mode;
}

void BellowsModel3D::processMouseMovement(float xpos, float ypos) {
    if (!mouseDragging) return;
    
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Reversed since y coordinates go from bottom to top
    
    lastX = xpos;
    lastY = ypos;
    
    // Adjust sensitivity based on model scale for more consistent feel
    const float baseSensitivity = 0.1f;
    float scaledSensitivity = baseSensitivity / std::max(0.5f, modelScale);
    
    // Apply smoothed sensitivity 
    xoffset *= scaledSensitivity;
    yoffset *= scaledSensitivity;
    
    // Only update rotation values - no translation
    modelRotationY += xoffset;
    modelRotationX += yoffset;
    
    // Limit pitch to avoid flipping issues
    if (modelRotationX > 89.0f) modelRotationX = 89.0f;
    if (modelRotationX < -89.0f) modelRotationX = -89.0f;
    
    updateModelMatrix();
}

void BellowsModel3D::processMousePress(bool pressed, float xpos, float ypos) {
    if (pressed) {
        mouseDragging = true;
        lastX = xpos;
        lastY = ypos;
    } else {
        mouseDragging = false;
    }
}

void BellowsModel3D::processMouseScroll(float yoffset) {
    // More responsive and smoother zoom
    float zoomFactor = yoffset * 0.08f;
    
    // Apply non-linear scaling for better zoom control
    if (yoffset > 0) {
        // Zooming in - slower at higher zoom levels
        modelScale *= (1.0f + zoomFactor);
    } else {
        // Zooming out - faster at higher zoom levels
        modelScale /= (1.0f - zoomFactor);
    }
    
    // Clamp scale to reasonable limits
    modelScale = std::max(0.1f, std::min(modelScale, 5.0f));
    
    updateModelMatrix();
}

void BellowsModel3D::resetView() {
    modelScale = 1.0f;
    modelRotationX = 0.0f;
    modelRotationY = 0.0f;
    
    updateModelMatrix();
}

void BellowsModel3D::setFrontView() {
    modelRotationX = 0.0f;
    modelRotationY = 0.0f;
    updateModelMatrix();
}

void BellowsModel3D::setSideView() {
    modelRotationX = 0.0f;
    modelRotationY = 90.0f;
    updateModelMatrix();
}

void BellowsModel3D::setTopView() {
    modelRotationX = -90.0f;
    modelRotationY = 0.0f;
    updateModelMatrix();
}

void BellowsModel3D::setIsometricView() {
    modelRotationX = 30.0f;
    modelRotationY = 45.0f;
    updateModelMatrix();
}

void BellowsModel3D::updateModelMatrix() {
    model = glm::mat4(1.0f);
    
    // First apply scale
    model = glm::scale(model, glm::vec3(modelScale));
    
    // Apply rotations around the center of the model
    // Use a stabilized rotation approach
    glm::mat4 rotationMatrix = glm::mat4(1.0f);
    rotationMatrix = glm::rotate(rotationMatrix, glm::radians(modelRotationX), glm::vec3(1.0f, 0.0f, 0.0f));
    rotationMatrix = glm::rotate(rotationMatrix, glm::radians(modelRotationY), glm::vec3(0.0f, 1.0f, 0.0f));
    
    model = model * rotationMatrix;
    
    // No translation is applied, ensuring model stays centered
}

void BellowsModel3D::enableCrossSection(bool enable) {
    this->showCrossSection = enable;
}

void BellowsModel3D::setCrossSectionAxis(int axis) {
    this->crossSectionAxis = axis;
}

void BellowsModel3D::setCrossSectionPosition(float position) {
    this->crossSectionPos = position;
} 