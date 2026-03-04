#include "utils/VectorMath.hpp"
#include <glm/glm.hpp>
#include "Base3DViewer.hpp"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

Base3DViewer::Base3DViewer()
    : framebuffer(0), 
      textureColorBuffer(0), 
      rbo(0),
      renderMode(0),
      objectColor(0.8f, 0.8f, 0.8f),
      lightColor(1.0f, 1.0f, 1.0f),
      ambientStrength(0.4f),
      diffuseStrength(0.7f),
      specularStrength(0.7f),
      shininess(32.0f),
      showCrossSection(false),
      crossSectionAxis(0),
      crossSectionPos(0.0f),
      mousePressed(false),
      lastX(0.0f),
      lastY(0.0f),
      viewportWidth(800),
      viewportHeight(600) {
}

Base3DViewer::~Base3DViewer() {
    if (framebuffer) glDeleteFramebuffers(1, &framebuffer);
    if (textureColorBuffer) glDeleteTextures(1, &textureColorBuffer);
    if (rbo) glDeleteRenderbuffers(1, &rbo);
}

Camera* Base3DViewer::getCamera() const {
    return camera.get();
}

void Base3DViewer::setupFramebuffer(int width, int height) {
    // Guard against zero or negative dimensions 
    if (width < 1) width = 1;
    if (height < 1) height = 1;
    
    // Delete existing framebuffer if it exists
    if (framebuffer) {
        glDeleteFramebuffers(1, &framebuffer);
        glDeleteTextures(1, &textureColorBuffer);
        glDeleteRenderbuffers(1, &rbo);
    }
    
    // Store new dimensions
    viewportWidth = width;
    viewportHeight = height;
    
    // Create framebuffer
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    
    // Create color attachment texture
    glGenTextures(1, &textureColorBuffer);
    glBindTexture(GL_TEXTURE_2D, textureColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorBuffer, 0);
    
    // Create depth and stencil attachment (renderbuffer)
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
    
    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Error: Framebuffer not complete!" << std::endl;
    }
    
    // Unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Base3DViewer::resizeFramebuffer(int width, int height) {
    // Guard against zero or negative dimensions (can happen on first ImGui frame)
    if (width < 1) width = 1;
    if (height < 1) height = 1;
    
    if (width != viewportWidth || height != viewportHeight) {
        setupFramebuffer(width, height);
    }
}

void Base3DViewer::bindFramebufferForRendering() {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    setupCommonRenderState();
}

void Base3DViewer::unbindFramebuffer() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Base3DViewer::setupCommonRenderState() {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    // Clear with consistent background color
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Set viewport
    glViewport(0, 0, viewportWidth, viewportHeight);
}

glm::mat4 Base3DViewer::createProjectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(camera->Zoom), aspectRatio, 0.1f, 100.0f);
}

void Base3DViewer::displayRenderedTexture(glm::dvec2 windowSize) {
    ImGui::Image((ImTextureID)(intptr_t)textureColorBuffer, Drawing::Math::toImVec2(windowSize), ImVec2(0, 1), ImVec2(1, 0));
}

void Base3DViewer::processStandardMouseInput(const SDL_Event& event) {
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        if (event.button.button == SDL_BUTTON_LEFT) {
            mousePressed = true;
            lastX = static_cast<float>(event.button.x);
            lastY = static_cast<float>(event.button.y);
        }
    } else if (event.type == SDL_MOUSEBUTTONUP) {
        if (event.button.button == SDL_BUTTON_LEFT) {
            mousePressed = false;
        }
    } else if (event.type == SDL_MOUSEMOTION && mousePressed) {
        float xpos = static_cast<float>(event.motion.x);
        float ypos = static_cast<float>(event.motion.y);
        
        // Calculate mouse movement offsets
        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // Reversed since y-coordinates range from bottom to top
        
        lastX = xpos;
        lastY = ypos;
        
        // Apply camera rotation
        if (camera) {
            camera->ProcessMouseMovement(xoffset, yoffset);
        }
    } else if (event.type == SDL_MOUSEWHEEL) {
        camera->ProcessMouseScroll(static_cast<float>(event.wheel.y));
    }
}

void Base3DViewer::processStandardKeyboardInput(const SDL_Event& event) {
    if (event.type == SDL_KEYDOWN && camera) {
        float deltaTime = 0.1f;
        switch (event.key.keysym.sym) {
            case SDLK_w:
                camera->ProcessKeyboard(FORWARD, deltaTime);
                break;
            case SDLK_s:
                camera->ProcessKeyboard(BACKWARD, deltaTime);
                break;
            case SDLK_a:
                camera->ProcessKeyboard(LEFT, deltaTime);
                break;
            case SDLK_d:
                camera->ProcessKeyboard(RIGHT, deltaTime);
                break;
        }
    }
}
