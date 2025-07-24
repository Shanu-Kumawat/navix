#ifndef BALL_BEARING_VIEWER_3D_HPP
#define BALL_BEARING_VIEWER_3D_HPP

#include <memory>
#include "Camera.hpp"
#include "Shader.hpp"
#include "BallBearingModel3D.hpp"
#include "shapes/ComplexShapes.hpp"
#include <glad/glad.h>
#include <SDL2/SDL.h>
#include "imgui.h"

class BallBearingViewer3D {
public:
    BallBearingViewer3D();
    ~BallBearingViewer3D();
    
    void initialize();
    void render(const Drawing::BallBearing* ballBearing, ImVec2 windowSize);
    void handleInput(const SDL_Event& event);
    
    // Public getter for the camera
    Camera* getCamera(); 

private:
    // OpenGL objects
    unsigned int framebuffer;
    unsigned int textureColorBuffer;
    unsigned int rbo;
    
    // Camera
    std::unique_ptr<Camera> camera;
    
    // Shader
    std::unique_ptr<Shader> shader;
    
    // 3D model
    std::unique_ptr<BallBearingModel3D> ballBearingModel;
    
    // Rendering settings
    int renderMode;
    glm::vec3 objectColor;
    glm::vec3 lightColor;
    float ambientStrength;
    float diffuseStrength;
    float specularStrength;
    float shininess;
    
    // Cross-section settings
    bool showCrossSection;
    int crossSectionAxis;
    float crossSectionPos;
    
    // Mouse handling
    bool mousePressed;
    float lastX, lastY;
    
    // Window size
    int viewportWidth;
    int viewportHeight;
    
    // Setup methods
    void setupFramebuffer(int width, int height);
    void resizeFramebuffer(int width, int height);
};

#endif
