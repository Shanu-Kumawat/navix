#pragma once
#include <memory>
#include "Camera.hpp"
#include "Shader.hpp"
#include "ShockAbsorberModel3D.hpp"
#include "shapes/BasicShapes.hpp"
#include "shapes/ShockAbsorberBottomEnd.hpp"
#include <glad/glad.h>
#include <SDL2/SDL.h>
#include "imgui.h"

class ShockAbsorberViewer3D {
public:
    ShockAbsorberViewer3D();
    ~ShockAbsorberViewer3D();

    void initialize();
    void render(const Drawing::Spring2D* spring, const Drawing::ShockAbsorberEnd2D* end, const Drawing::ShockAbsorberBottomEnd* bottomEnd, ImVec2 windowSize);
    void handleInput(const SDL_Event& event);
    Camera* getCamera();

private:
    unsigned int framebuffer;
    unsigned int textureColorBuffer;
    unsigned int rbo;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<Shader> shader;
    std::unique_ptr<ShockAbsorberModel3D> model;
    int renderMode;
    glm::vec3 objectColor;
    glm::vec3 lightColor;
    float ambientStrength;
    float diffuseStrength;
    float specularStrength;
    float shininess;
    bool mousePressed;
    float lastX, lastY;
    int viewportWidth;
    int viewportHeight;
    void setupFramebuffer(int width, int height);
    void resizeFramebuffer(int width, int height);
}; 