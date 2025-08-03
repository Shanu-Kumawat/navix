#include "SpringModel3D.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

SpringModel3D::SpringModel3D()
    : VAO(0), VBO(0), EBO(0), shader(nullptr), modelScale(1.0f), modelRotationX(0.0f), modelRotationY(0.0f),
      objectColor(0.8f, 0.8f, 0.8f), ambientStrength(0.3f), diffuseStrength(0.7f), specularStrength(0.5f), shininess(32.0f),
      lightPos(1.0f, 1.0f, 2.0f), lightColor(1.0f, 1.0f, 1.0f), renderMode(0), mouseDragging(false), lastX(0.0f), lastY(0.0f) {
    model = glm::mat4(1.0f);
}

SpringModel3D::~SpringModel3D() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
}

void SpringModel3D::generateMesh(const Drawing::Spring2D* spring) {
    // Parameter validation
    Drawing::Spring2D defaultSpring(0.0f, 0.0f, 100.0f, 10.0f, 200.0f, 8);
    const Drawing::Spring2D* validSpring = spring;
    if (!spring || spring->outerDiameter <= 0.0f || spring->wireDiameter <= 0.0f ||
        spring->freeLength <= 0.0f || spring->numCoils <= 0 ||
        spring->outerDiameter <= spring->wireDiameter) {
        validSpring = &defaultSpring;
    }
    vertices.clear();
    indices.clear();
    // Use 16 radial segments and 200 helix segments for smoothness
    generateHelicalMesh(validSpring, 16, 200);
    generateNormals();
    setupMesh();
    resetView();
}

void SpringModel3D::generateHelicalMesh(const Drawing::Spring2D* spring, int radialSegments, int helixSegments) {
    float R = spring->outerDiameter / 2.0f - spring->wireDiameter / 2.0f; // Helix radius
    float r = spring->wireDiameter / 2.0f; // Wire radius
    float pitch = spring->freeLength / spring->numCoils;
    float totalLength = spring->freeLength;
    int coils = spring->numCoils;
    float centerY = 0.0f;
    // Generate vertices
    for (int i = 0; i <= helixSegments; ++i) {
        float t = (float)i / helixSegments;
        float theta = t * coils * 2.0f * M_PI;
        float y = t * totalLength - totalLength / 2.0f;
        float cx = R * std::sin(theta);
        float cz = R * std::cos(theta);
        // For each point on the helix, generate a ring of vertices
        for (int j = 0; j < radialSegments; ++j) {
            float phi = (float)j / radialSegments * 2.0f * M_PI;
            float nx = std::cos(phi) * std::cos(theta);
            float ny = std::sin(phi);
            float nz = -std::cos(phi) * std::sin(theta);
            float x = cx + r * nx;
            float z = cz + r * nz;
            float vx = x;
            float vy = y + centerY + r * ny;
            float vz = z;
            // Position
            vertices.push_back(vx);
            vertices.push_back(vy);
            vertices.push_back(vz);
            // Normal (will be normalized later)
            vertices.push_back(nx);
            vertices.push_back(ny);
            vertices.push_back(nz);
        }
    }
    // Generate indices
    for (int i = 0; i < helixSegments; ++i) {
        for (int j = 0; j < radialSegments; ++j) {
            int curr = i * radialSegments + j;
            int next = i * radialSegments + (j + 1) % radialSegments;
            int currNext = (i + 1) * radialSegments + j;
            int nextNext = (i + 1) * radialSegments + (j + 1) % radialSegments;
            indices.push_back(curr);
            indices.push_back(next);
            indices.push_back(currNext);
            indices.push_back(next);
            indices.push_back(nextNext);
            indices.push_back(currNext);
        }
    }
}

void SpringModel3D::generateNormals() {
    // Normals are already set per vertex, but can be normalized if needed
    for (size_t i = 0; i < vertices.size() / 6; ++i) {
        float nx = vertices[i * 6 + 3];
        float ny = vertices[i * 6 + 4];
        float nz = vertices[i * 6 + 5];
        float len = std::sqrt(nx * nx + ny * ny + nz * nz);
        if (len > 0.0f) {
            vertices[i * 6 + 3] /= len;
            vertices[i * 6 + 4] /= len;
            vertices[i * 6 + 5] /= len;
        }
    }
}

void SpringModel3D::setupMesh() {
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

void SpringModel3D::render(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos) {
    if (!shader || VAO == 0 || indices.empty()) return;
    shader->use();
    shader->setMat4("projection", projection);
    shader->setMat4("view", view);
    shader->setMat4("model", model);
    shader->setVec3("objectColor", objectColor);
    shader->setFloat("ambientStrength", ambientStrength);
    shader->setFloat("diffuseStrength", diffuseStrength);
    shader->setFloat("specularStrength", specularStrength);
    shader->setFloat("shininess", shininess);
    shader->setVec3("lightPos", lightPos);
    shader->setVec3("lightColor", lightColor);
    shader->setVec3("viewPos", cameraPos);
    shader->setInt("renderMode", renderMode);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void SpringModel3D::setShader(Shader* shader) { this->shader = shader; }
void SpringModel3D::setMaterial(const glm::vec3& color, float ambient, float diffuse, float specular, float shininess) {
    this->objectColor = color;
    this->ambientStrength = ambient;
    this->diffuseStrength = diffuse;
    this->specularStrength = specular;
    this->shininess = shininess;
}
void SpringModel3D::setLight(const glm::vec3& position, const glm::vec3& color) {
    this->lightPos = position;
    this->lightColor = color;
}
void SpringModel3D::setRenderMode(int mode) { this->renderMode = mode; }
void SpringModel3D::processMouseMovement(float xpos, float ypos) {
    if (!mouseDragging) return;
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    const float baseSensitivity = 0.1f;
    float scaledSensitivity = baseSensitivity / std::max(0.5f, modelScale);
    xoffset *= scaledSensitivity;
    yoffset *= scaledSensitivity;
    modelRotationY += xoffset;
    modelRotationX += yoffset;
    if (modelRotationX > 89.0f) modelRotationX = 89.0f;
    if (modelRotationX < -89.0f) modelRotationX = -89.0f;
    updateModelMatrix();
}
void SpringModel3D::processMousePress(bool pressed, float xpos, float ypos) {
    if (pressed) {
        mouseDragging = true;
        lastX = xpos;
        lastY = ypos;
    } else {
        mouseDragging = false;
    }
}
void SpringModel3D::processMouseScroll(float yoffset) {
    float zoomFactor = yoffset * 0.08f;
    if (yoffset > 0) {
        modelScale *= (1.0f + zoomFactor);
    } else {
        modelScale /= (1.0f - zoomFactor);
    }
    modelScale = std::max(0.1f, std::min(modelScale, 5.0f));
    updateModelMatrix();
}
void SpringModel3D::resetView() {
    modelScale = 1.0f;
    modelRotationX = 0.0f;
    modelRotationY = 0.0f;
    updateModelMatrix();
}
void SpringModel3D::setFrontView() { modelRotationX = 0.0f; modelRotationY = 0.0f; updateModelMatrix(); }
void SpringModel3D::setSideView() { modelRotationX = 0.0f; modelRotationY = 90.0f; updateModelMatrix(); }
void SpringModel3D::setTopView() { modelRotationX = -90.0f; modelRotationY = 0.0f; updateModelMatrix(); }
void SpringModel3D::setIsometricView() { modelRotationX = 30.0f; modelRotationY = 45.0f; updateModelMatrix(); }
void SpringModel3D::updateModelMatrix() {
    model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(modelScale));
    glm::mat4 rotationMatrix = glm::mat4(1.0f);
    rotationMatrix = glm::rotate(rotationMatrix, glm::radians(modelRotationX), glm::vec3(1.0f, 0.0f, 0.0f));
    rotationMatrix = glm::rotate(rotationMatrix, glm::radians(modelRotationY), glm::vec3(0.0f, 1.0f, 0.0f));
    model = model * rotationMatrix;
} 