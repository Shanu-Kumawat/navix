#ifndef BASE_3D_VIEWER_HPP
#define BASE_3D_VIEWER_HPP

#include <memory>
#include "Camera.hpp"
#include "Shader.hpp"
#include <glad/glad.h>
#include <SDL2/SDL.h>
#include "imgui.h"
#include <glm/glm.hpp>

/**
 * Base class for all 3D shape viewers providing standardized architecture
 * This class encapsulates common OpenGL framebuffer management, camera handling,
 * and basic rendering pipeline setup that all complex shapes should follow.
 */
class Base3DViewer {
public:
    Base3DViewer();
    virtual ~Base3DViewer();
    
    // Core interface - must be implemented by derived classes
    virtual void initialize() = 0;
    virtual void handleInput(const SDL_Event& event) = 0;
    
    // Common functionality provided by base class
    Camera* getCamera() const;
    void resizeFramebuffer(int width, int height);
    
protected:
    // Framebuffer management
    void setupFramebuffer(int width, int height);
    void bindFramebufferForRendering();
    void unbindFramebuffer();
    unsigned int getColorTexture() const { return textureColorBuffer; }
    
    // Common rendering setup
    void setupCommonRenderState();
    glm::mat4 createProjectionMatrix(float aspectRatio) const;
    
    // OpenGL objects - protected so derived classes can access
    unsigned int framebuffer;
    unsigned int textureColorBuffer;
    unsigned int rbo;
    
    // Core components
    std::unique_ptr<Camera> camera;
    std::unique_ptr<Shader> shader;
    
    // Common rendering settings
    int renderMode;
    glm::vec3 objectColor;
    glm::vec3 lightColor;
    float ambientStrength;
    float diffuseStrength;
    float specularStrength;
    float shininess;
    
    // Cross-section settings (standardized across all shapes)
    bool showCrossSection;
    int crossSectionAxis;    // 0=X, 1=Y, 2=Z
    float crossSectionPos;
    
    // Mouse handling
    bool mousePressed;
    float lastX, lastY;
    
    // Window dimensions
    int viewportWidth;
    int viewportHeight;
    
    // Helper methods for derived classes
    void displayRenderedTexture(glm::dvec2 windowSize);
    void processStandardMouseInput(const SDL_Event& event);
    void processStandardKeyboardInput(const SDL_Event& event);
};

#endif
