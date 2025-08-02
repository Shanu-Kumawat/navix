#pragma once
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "imgui.h"
#include "Shader.hpp"
#include "shapes/BasicShapes.hpp"
#include "shapes/ShockAbsorberBottomEnd.hpp"

class ShockAbsorberModel3D {
public:
    ShockAbsorberModel3D();
    ~ShockAbsorberModel3D();

    void generateMesh(const Drawing::Spring2D* spring, const Drawing::ShockAbsorberEnd2D* end, const Drawing::ShockAbsorberBottomEnd* bottomEnd);
    void render(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos);

    void setShader(Shader* shader);
    void setMaterial(const glm::vec3& color, float ambient, float diffuse, float specular, float shininess);
    void setLight(const glm::vec3& position, const glm::vec3& color);
    void setRenderMode(int mode);

    void processMouseMovement(float xpos, float ypos);
    void processMousePress(bool pressed, float xpos, float ypos);
    void processMouseScroll(float yoffset);

    void resetView();
    void updateModelMatrix();

    bool hasMesh() const { return !indices.empty(); }

private:
    unsigned int VAO, VBO, EBO;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    Shader* shader;
    glm::mat4 model;
    float modelScale;
    float modelRotationX;
    float modelRotationY;
    glm::vec3 objectColor;
    float ambientStrength;
    float diffuseStrength;
    float specularStrength;
    float shininess;
    glm::vec3 lightPos;
    glm::vec3 lightColor;
    int renderMode;
    bool mouseDragging;
    float lastX, lastY;
    void revolveProfile(const std::vector<ImVec2>& profile, int segments, float yOffset = 0.0f, float yScale = 1.0f);
    void generateNormals();
    void setupMesh();
}; 