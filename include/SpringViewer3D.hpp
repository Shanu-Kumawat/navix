#ifndef SPRING_VIEWER_3D_HPP
#define SPRING_VIEWER_3D_HPP

#include <memory>
#include "Camera.hpp"
#include "Shader.hpp"
#include "SpringModel3D.hpp"
#include "shapes/BasicShapes.hpp"
#include <glad/glad.h>
#include <SDL2/SDL.h>
#include "imgui.h"

class SpringViewer3D {
public:
    SpringViewer3D();
    ~SpringViewer3D();
    void initialize();
    void render(const Drawing::Spring2D* spring, ImVec2 windowSize);
    void handleInput(const SDL_Event& event);
    Camera* getCamera();
private:
    unsigned int framebuffer;
    unsigned int textureColorBuffer;
    unsigned int rbo;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<Shader> shader;
    std::unique_ptr<SpringModel3D> springModel;
    int renderMode;
    glm::vec3 objectColor;
    glm::vec3 lightColor;
    float ambientStrength;
    float diffuseStrength;
    float specularStrength;
    float shininess;
    bool showCrossSection;
    int crossSectionAxis;
    float crossSectionPos;
    bool mousePressed;
    float lastX, lastY;
    int viewportWidth;
    int viewportHeight;
    void setupFramebuffer(int width, int height);
    void resizeFramebuffer(int width, int height);
};

#endif 