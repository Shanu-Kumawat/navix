#include "ShockAbsorberModel3D.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

ShockAbsorberModel3D::ShockAbsorberModel3D()
    : VAO(0), VBO(0), EBO(0), shader(nullptr), modelScale(1.0f), modelRotationX(0.0f), modelRotationY(0.0f),
      objectColor(0.8f, 0.8f, 0.8f), ambientStrength(0.3f), diffuseStrength(0.7f), specularStrength(0.5f), shininess(32.0f),
      lightPos(1.0f, 1.0f, 2.0f), lightColor(1.0f, 1.0f, 1.0f), renderMode(0), mouseDragging(false), lastX(0.0f), lastY(0.0f) {
    model = glm::mat4(1.0f);
}

ShockAbsorberModel3D::~ShockAbsorberModel3D() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
}

void ShockAbsorberModel3D::generateMesh(const Drawing::Spring2D* spring, const Drawing::ShockAbsorberEnd2D* end, const Drawing::ShockAbsorberBottomEnd* bottomEnd) {
    vertices.clear();
    indices.clear();
    if (!spring || !end || !bottomEnd) {
        std::cerr << "[ShockAbsorberModel3D] Null pointer for spring, end, or bottomEnd!" << std::endl;
        return;
    }
    auto endProfile = end->generateProfile();
    auto springProfile = spring->generateProfile();
    auto bottomProfile = bottomEnd->generateProfile();
    if (endProfile.empty()) {
        std::cerr << "[ShockAbsorberModel3D] endProfile is empty!" << std::endl;
        return;
    }
    if (springProfile.empty()) {
        std::cerr << "[ShockAbsorberModel3D] springProfile is empty!" << std::endl;
        return;
    }
    if (bottomProfile.empty()) {
        std::cerr << "[ShockAbsorberModel3D] bottomProfile is empty!" << std::endl;
        return;
    }
    // Calculate Y offsets
    float springLength = spring->freeLength;
    float endLength = end->shaftLength;
    float bottomLength = bottomEnd->height + bottomEnd->plateThickness; // approximate
    float yTopEnd = spring->centerY + springLength / 2.0f + endLength / 2.0f;
    float ySpring = spring->centerY;
    float yBottomEnd = spring->centerY - springLength / 2.0f - bottomLength / 2.0f;
    std::cerr << "[ShockAbsorberModel3D] Offsets: yTopEnd=" << yTopEnd << ", ySpring=" << ySpring << ", yBottomEnd=" << yBottomEnd << std::endl;
    std::cerr << "[ShockAbsorberModel3D] Profile sizes: end=" << endProfile.size() << ", spring=" << springProfile.size() << ", bottom=" << bottomProfile.size() << std::endl;
    // Revolve each profile at the correct Y offset
    revolveProfile(endProfile, 36, yTopEnd - end->baseCenter.y, 1.0f);
    revolveProfile(springProfile, 36, ySpring - spring->centerY, 1.0f);
    revolveProfile(bottomProfile, 36, yBottomEnd - bottomEnd->baseCenter.y, 1.0f);
    std::cerr << "[ShockAbsorberModel3D] Mesh revolved. Generating normals and setting up mesh..." << std::endl;
    generateNormals();
    // Print vertex bounds for debugging
    float minX = 1e9, maxX = -1e9, minY = 1e9, maxY = -1e9, minZ = 1e9, maxZ = -1e9;
    for (size_t i = 0; i < vertices.size() / 6; ++i) {
        float x = vertices[i * 6 + 0];
        float y = vertices[i * 6 + 1];
        float z = vertices[i * 6 + 2];
        minX = std::min(minX, x); maxX = std::max(maxX, x);
        minY = std::min(minY, y); maxY = std::max(maxY, y);
        minZ = std::min(minZ, z); maxZ = std::max(maxZ, z);
    }
    std::cerr << "[ShockAbsorberModel3D] Vertex bounds: X[" << minX << ", " << maxX << "] Y[" << minY << ", " << maxY << "] Z[" << minZ << ", " << maxZ << "]\n";
    setupMesh();
    resetView();
    std::cerr << "[ShockAbsorberModel3D] Mesh generation complete." << std::endl;
}

void ShockAbsorberModel3D::render(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos) {
    if (!shader || indices.empty()) return;
    shader->use();
    shader->setMat4("projection", projection);
    shader->setMat4("view", view);
    shader->setMat4("model", model);
    shader->setVec3("objectColor", objectColor);
    shader->setVec3("lightColor", lightColor);
    shader->setVec3("lightPos", lightPos);
    shader->setVec3("viewPos", cameraPos);
    shader->setFloat("ambientStrength", ambientStrength);
    shader->setFloat("diffuseStrength", diffuseStrength);
    shader->setFloat("specularStrength", specularStrength);
    shader->setFloat("shininess", shininess);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    // Minimal OpenGL test: draw a hardcoded triangle if debug flag is set
    static bool showDebugTriangle = true; // Set to false to disable
    if (showDebugTriangle) {
        float testVertices[] = {
            0.0f,  0.5f, 0.0f,
           -0.5f, -0.5f, 0.0f,
            0.5f, -0.5f, 0.0f
        };
        unsigned int testVAO, testVBO;
        glGenVertexArrays(1, &testVAO);
        glGenBuffers(1, &testVBO);
        glBindVertexArray(testVAO);
        glBindBuffer(GL_ARRAY_BUFFER, testVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(testVertices), testVertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(testVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
        glDeleteVertexArrays(1, &testVAO);
        glDeleteBuffers(1, &testVBO);
    }
}

void ShockAbsorberModel3D::setShader(Shader* shader_) { shader = shader_; }
void ShockAbsorberModel3D::setMaterial(const glm::vec3& color, float ambient, float diffuse, float specular, float shininess_) {
    objectColor = color;
    ambientStrength = ambient;
    diffuseStrength = diffuse;
    specularStrength = specular;
    shininess = shininess_;
}
void ShockAbsorberModel3D::setLight(const glm::vec3& position, const glm::vec3& color) {
    lightPos = position;
    lightColor = color;
}
void ShockAbsorberModel3D::setRenderMode(int mode) { renderMode = mode; }
void ShockAbsorberModel3D::processMouseMovement(float xpos, float ypos) { /* TODO */ }
void ShockAbsorberModel3D::processMousePress(bool pressed, float xpos, float ypos) { /* TODO */ }
void ShockAbsorberModel3D::processMouseScroll(float yoffset) { /* TODO */ }
void ShockAbsorberModel3D::resetView() { modelRotationX = 0.0f; modelRotationY = 0.0f; modelScale = 1.0f; updateModelMatrix(); }
void ShockAbsorberModel3D::updateModelMatrix() {
    // Center the model at the origin and scale it down for visibility
    float minX = 1e9, maxX = -1e9, minY = 1e9, maxY = -1e9, minZ = 1e9, maxZ = -1e9;
    for (size_t i = 0; i < vertices.size() / 6; ++i) {
        float x = vertices[i * 6 + 0];
        float y = vertices[i * 6 + 1];
        float z = vertices[i * 6 + 2];
        minX = std::min(minX, x); maxX = std::max(maxX, x);
        minY = std::min(minY, y); maxY = std::max(maxY, y);
        minZ = std::min(minZ, z); maxZ = std::max(maxZ, z);
    }
    float centerX = (minX + maxX) / 2.0f;
    float centerY = (minY + maxY) / 2.0f;
    float centerZ = (minZ + maxZ) / 2.0f;
    float maxExtent = std::max({maxX - minX, maxY - minY, maxZ - minZ});
    float scale = 2.0f / (maxExtent > 0 ? maxExtent : 1.0f); // fit in [-1,1]
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-centerX, -centerY, -centerZ));
    model = glm::scale(model, glm::vec3(scale, scale, scale));
    // Add a 30-degree rotation around Y for visibility
    model = glm::rotate(model, glm::radians(30.0f), glm::vec3(0, 1, 0));
}
void ShockAbsorberModel3D::revolveProfile(const std::vector<ImVec2>& profile, int segments, float yOffset, float yScale) {
    if (profile.empty()) return;
    const float PI = 3.14159265359f;
    const float angleStep = 2.0f * PI / segments;
    size_t baseVertex = vertices.size() / 6; // 6 floats per vertex (pos + normal)

    // Generate vertices
    for (const auto& point : profile) {
        float x = point.x;
        float y = (point.y + yOffset) * yScale;
        for (int i = 0; i < segments; ++i) {
            float angle = i * angleStep;
            float vx = x * cos(angle);
            float vz = x * sin(angle);
            float vy = y;
            // Position
            vertices.push_back(vx);
            vertices.push_back(vy);
            vertices.push_back(vz);
            // Placeholder normal (to be computed later)
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
        }
    }
    // Generate indices
    size_t rings = profile.size();
    for (size_t i = 0; i < rings - 1; ++i) {
        for (int j = 0; j < segments; ++j) {
            unsigned int curr = baseVertex + i * segments + j;
            unsigned int next = baseVertex + i * segments + (j + 1) % segments;
            unsigned int currNext = baseVertex + (i + 1) * segments + j;
            unsigned int nextNext = baseVertex + (i + 1) * segments + (j + 1) % segments;
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
void ShockAbsorberModel3D::generateNormals() {
    std::cerr << "[ShockAbsorberModel3D] generateNormals: vertices=" << vertices.size() << std::endl;
    if (vertices.empty()) return;
    // Set all normals to (0, 1, 0) for testing
    for (size_t i = 0; i < vertices.size() / 6; ++i) {
        vertices[i * 6 + 3] = 0.0f;
        vertices[i * 6 + 4] = 1.0f;
        vertices[i * 6 + 5] = 0.0f;
    }
}
void ShockAbsorberModel3D::setupMesh() {
    std::cerr << "[ShockAbsorberModel3D] setupMesh: vertices=" << vertices.size() << ", indices=" << indices.size() << std::endl;
    if (vertices.empty() || indices.empty()) {
        std::cerr << "[ShockAbsorberModel3D] setupMesh: No vertices or indices to set up!" << std::endl;
        return;
    }
    if (VAO == 0) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
    }
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
} 