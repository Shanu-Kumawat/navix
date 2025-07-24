#include "BallBearingViewer3D.hpp"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

BallBearingViewer3D::BallBearingViewer3D() 
    : framebuffer(0), 
      textureColorBuffer(0), 
      rbo(0),
      renderMode(0),
      objectColor(0.9f, 0.9f, 0.95f),  // Better metallic color
      lightColor(1.0f, 1.0f, 1.0f),
      ambientStrength(0.25f),           // Enhanced lighting
      diffuseStrength(0.8f),
      specularStrength(0.9f),
      shininess(128.0f),                // Higher shininess
      showCrossSection(false),
      crossSectionAxis(0),
      crossSectionPos(0.0f),
      mousePressed(false),
      lastX(0.0f),
      lastY(0.0f),
      viewportWidth(800),
      viewportHeight(600) {
}

BallBearingViewer3D::~BallBearingViewer3D() {
    if (framebuffer) glDeleteFramebuffers(1, &framebuffer);
    if (textureColorBuffer) glDeleteTextures(1, &textureColorBuffer);
    if (rbo) glDeleteRenderbuffers(1, &rbo);
}

void BallBearingViewer3D::initialize() {
    // Initialize camera with a slightly angled view
    camera = std::make_unique<Camera>(glm::vec3(2.0f, 1.5f, 3.0f), glm::vec3(0.0f, 1.0f, 0.0f), -110.0f, -15.0f); 
    
    // Create shader (reuse bellows shader as it's compatible)
    shader = std::make_unique<Shader>("shaders/bellows.vs", "shaders/bellows.fs");
    
    // Create 3D model
    ballBearingModel = std::make_unique<BallBearingModel3D>();
    ballBearingModel->setShader(shader.get());
    
    // Setup initial framebuffer
    setupFramebuffer(viewportWidth, viewportHeight);
}

// Add the implementation for the getter function
Camera* BallBearingViewer3D::getCamera() {
    return camera.get();
}

void BallBearingViewer3D::setupFramebuffer(int width, int height) {
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
    
    // Create a renderbuffer object for depth and stencil attachment
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
    
    // Check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    }
    
    // Unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void BallBearingViewer3D::resizeFramebuffer(int width, int height) {
    if (width != viewportWidth || height != viewportHeight) {
        setupFramebuffer(width, height);
    }
}

void BallBearingViewer3D::render(const Drawing::BallBearing* ballBearing, ImVec2 windowSize) {
    if (!ballBearingModel || !camera || !shader) return;
    
    // Update framebuffer if window size changed
    resizeFramebuffer(static_cast<int>(windowSize.x), static_cast<int>(windowSize.y));
    
    // Generate/update 3D mesh if needed
    ballBearingModel->generateMesh(ballBearing);
    
    // Set material and lighting properties for high-quality rendering
    ballBearingModel->setMaterial(objectColor, ambientStrength, diffuseStrength, specularStrength, shininess);
    ballBearingModel->setLight(glm::vec3(3.0f, 4.0f, 3.0f), lightColor); // Better light position
    ballBearingModel->setRenderMode(renderMode);
    
    // Always disable cross-section for now
    ballBearingModel->enableCrossSection(false);
    
    // Bind framebuffer and render to it
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);           // Better depth testing
    glEnable(GL_CULL_FACE);         // Enable face culling for performance
    glCullFace(GL_BACK);            // Cull back faces
    
    // Clear the framebuffer with darker background for better contrast
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Set viewport to match window size
    glViewport(0, 0, viewportWidth, viewportHeight);
    
    // Create projection matrix
    float aspectRatio = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);
    glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom), aspectRatio, 0.1f, 100.0f);
    
    // Get view matrix from camera
    glm::mat4 view = camera->GetViewMatrix();
    
    // Render the 3D model
    ballBearingModel->render(projection, view, camera->Position);
    
    // Unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Display the rendered texture in ImGui
    ImGui::Image((ImTextureID)(intptr_t)textureColorBuffer, windowSize, ImVec2(0, 1), ImVec2(1, 0));
    
    // Handle interactions if mouse is over the viewport
    ImVec2 mousePos = ImGui::GetIO().MousePos;
    ImVec2 windowPos = ImGui::GetWindowPos();
    if (ImGui::IsWindowHovered() && mousePressed) {
        float mouseX = mousePos.x - windowPos.x;
        float mouseY = mousePos.y - windowPos.y;
        ballBearingModel->processMouseMovement(mouseX, mouseY);
    }
}

void BallBearingViewer3D::handleInput(const SDL_Event& event) {
    if (!camera) return; // Add camera validity check
    
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        if (event.button.button == SDL_BUTTON_LEFT) {
            mousePressed = true;
            lastX = static_cast<float>(event.button.x);
            lastY = static_cast<float>(event.button.y);
            ballBearingModel->processMousePress(true, lastX, lastY);
        }
    } else if (event.type == SDL_MOUSEBUTTONUP) {
        if (event.button.button == SDL_BUTTON_LEFT) {
            mousePressed = false;
            ballBearingModel->processMousePress(false, lastX, lastY);
        }
    } else if (event.type == SDL_MOUSEMOTION) {
        if (mousePressed) {
            float xpos = static_cast<float>(event.motion.x);
            float ypos = static_cast<float>(event.motion.y);
            ballBearingModel->processMouseMovement(xpos, ypos);
            lastX = xpos;
            lastY = ypos;
        }
    } else if (event.type == SDL_MOUSEWHEEL) {
        // Use camera zoom instead of model scaling for consistent behavior with bellows
        camera->ProcessMouseScroll(static_cast<float>(event.wheel.y));
    } else if (event.type == SDL_KEYDOWN) {
        // Add keyboard controls like bellows viewer
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
            case SDLK_r:
                // Reset both camera and model view
                camera->Reset(); 
                ballBearingModel->resetView(); 
                break;
        }
    }
}
