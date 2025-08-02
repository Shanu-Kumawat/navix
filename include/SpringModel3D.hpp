#ifndef SPRING_MODEL_3D_HPP
#define SPRING_MODEL_3D_HPP

#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "imgui.h"
#include "Shader.hpp"
#include "shapes/BasicShapes.hpp"

class SpringModel3D {
public:
    SpringModel3D();
    ~SpringModel3D();

    void generateMesh(const Drawing::Spring2D* spring);
    void render(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos);

    void setShader(Shader* shader);
    void setMaterial(const glm::vec3& color, float ambient, float diffuse, float specular, float shininess);
    void setLight(const glm::vec3& position, const glm::vec3& color);
    void setRenderMode(int mode);

    void processMouseMovement(float xpos, float ypos);
    void processMousePress(bool pressed, float xpos, float ypos);
    void processMouseScroll(float yoffset);

    void resetView();
    void setFrontView();
    void setSideView();
    void setTopView();
    void setIsometricView();
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
    void generateHelicalMesh(const Drawing::Spring2D* spring, int radialSegments, int helixSegments);
    void generateNormals();
    void setupMesh();
};

#endif 