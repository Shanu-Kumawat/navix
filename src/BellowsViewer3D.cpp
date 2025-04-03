#include "BellowsViewer3D.hpp"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

BellowsViewer3D::BellowsViewer3D() 
    : framebuffer(0), 
      textureColorBuffer(0), 
      rbo(0),
      renderMode(0),
      objectColor(0.7f, 0.7f, 0.7f),
      lightColor(1.0f, 1.0f, 1.0f),
      ambientStrength(0.3f),
      diffuseStrength(0.7f),
      specularStrength(0.5f),
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

BellowsViewer3D::~BellowsViewer3D() {
    if (framebuffer) glDeleteFramebuffers(1, &framebuffer);
    if (textureColorBuffer) glDeleteTextures(1, &textureColorBuffer);
    if (rbo) glDeleteRenderbuffers(1, &rbo);
}

void BellowsViewer3D::initialize() {
    // Initialize camera with a slightly angled view by passing Yaw and Pitch to the constructor
    // Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH);
    camera = std::make_unique<Camera>(glm::vec3(2.0f, 1.5f, 3.0f), glm::vec3(0.0f, 1.0f, 0.0f), -110.0f, -15.0f); 
    
    // Create shader
    shader = std::make_unique<Shader>("shaders/bellows.vs", "shaders/bellows.fs");
    
    // Create 3D model (This line was missing in the previous file content shown, adding it back)
    bellowsModel = std::make_unique<BellowsModel3D>();
    bellowsModel->setShader(shader.get());
    
    // Setup initial framebuffer (This line was missing in the previous file content shown, adding it back)
    setupFramebuffer(viewportWidth, viewportHeight);

    // Removed erroneous ImGui::End() call that was here
}

// Add the implementation for the getter function
Camera* BellowsViewer3D::getCamera() {
    return camera.get();
}

void BellowsViewer3D::setupFramebuffer(int width, int height) {
    // Delete existing framebuffer if it exists
    if (framebuffer) {
        glDeleteFramebuffers(1, &framebuffer);
        glDeleteTextures(1, &textureColorBuffer);
        glDeleteRenderbuffers(1, &rbo);
    }
    
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
    
    // Create depth and stencil attachment
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
    
    // Check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void BellowsViewer3D::resizeFramebuffer(int width, int height) {
    if (width != viewportWidth || height != viewportHeight) {
        viewportWidth = width;
        viewportHeight = height;
        setupFramebuffer(width, height);
    }
}

void BellowsViewer3D::render(const Drawing::Bellows* bellows, ImVec2 windowSize) {
    if (!bellowsModel || !camera || !shader) return;
    
    // Update framebuffer if window size changed
    resizeFramebuffer(static_cast<int>(windowSize.x), static_cast<int>(windowSize.y));
    
    // Generate/update 3D mesh if needed
    bellowsModel->generateMesh(bellows);
    
    // Set material and lighting properties
    bellowsModel->setMaterial(objectColor, ambientStrength, diffuseStrength, specularStrength, shininess);
    bellowsModel->setLight(glm::vec3(2.0f, 2.0f, 2.0f), lightColor);
    bellowsModel->setRenderMode(renderMode);
    
    // Set cross-section
    if (showCrossSection) {
        bellowsModel->enableCrossSection(true);
        bellowsModel->setCrossSectionAxis(crossSectionAxis);
        bellowsModel->setCrossSectionPosition(crossSectionPos);
    } else {
        bellowsModel->enableCrossSection(false);
    }
    
    // Bind framebuffer and render to it
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glEnable(GL_DEPTH_TEST);
    
    // Clear the framebuffer
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Set viewport to match window size
    glViewport(0, 0, viewportWidth, viewportHeight);
    
    // Create projection matrix
    float aspectRatio = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);
    glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom), aspectRatio, 0.1f, 100.0f);
    
    // Get view matrix from camera
    glm::mat4 view = camera->GetViewMatrix();
    
    // Render the 3D model
    bellowsModel->render(projection, view, camera->Position);
    
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
        bellowsModel->processMouseMovement(mouseX, mouseY);
    }
}

void BellowsViewer3D::handleInput(const SDL_Event& event) {
    if (!camera) return;
    
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        if (event.button.button == SDL_BUTTON_LEFT) {
            mousePressed = true;
            lastX = static_cast<float>(event.button.x);
            lastY = static_cast<float>(event.button.y);
            bellowsModel->processMousePress(true, lastX, lastY);
        }
    } else if (event.type == SDL_MOUSEBUTTONUP) {
        if (event.button.button == SDL_BUTTON_LEFT) {
            mousePressed = false;
            bellowsModel->processMousePress(false, 0.0f, 0.0f);
        }
    // Removed SDL_MOUSEMOTION handling here, as it's now managed in main.cpp using ImGui::IsMouseDragging
    } else if (event.type == SDL_MOUSEWHEEL) {
        // Process scroll only if the viewport window is hovered (check in main.cpp or pass hover state)
        // For simplicity, we process it here for now, but ideally check hover status.
        camera->ProcessMouseScroll(static_cast<float>(event.wheel.y));
    } else if (event.type == SDL_KEYDOWN) {
        // TODO: Replace 0.1f with actual deltaTime calculated from frame time for smoother movement
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
                bellowsModel->resetView(); 
                break;
        }
    }
}

void BellowsViewer3D::showSettingsPanel() {
    // Window Begin/End is now handled in main.cpp's Render3DViewWindow
    // This function just renders the *content* of the settings panel.
    
    // View presets
    if (ImGui::Button("Front View")) bellowsModel->setFrontView();
    ImGui::SameLine();
    if (ImGui::Button("Side View")) bellowsModel->setSideView();
    ImGui::SameLine();
    if (ImGui::Button("Top View")) bellowsModel->setTopView();
    ImGui::SameLine();
    if (ImGui::Button("Isometric")) bellowsModel->setIsometricView();
    
    ImGui::Separator();
    
    // Rendering options
    const char* renderModes[] = { "Solid", "Wireframe", "Textured" };
    ImGui::Combo("Render Mode", &renderMode, renderModes, IM_ARRAYSIZE(renderModes));
    
    // Material properties
    ImGui::Text("Material Properties:");
    ImGui::ColorEdit3("Object Color", &objectColor.x);
    ImGui::SliderFloat("Ambient Strength", &ambientStrength, 0.0f, 1.0f);
    ImGui::SliderFloat("Diffuse Strength", &diffuseStrength, 0.0f, 1.0f);
    ImGui::SliderFloat("Specular Strength", &specularStrength, 0.0f, 1.0f);
    ImGui::SliderFloat("Shininess", &shininess, 1.0f, 128.0f);
    
    // Light properties
    ImGui::Text("Light Properties:");
    ImGui::ColorEdit3("Light Color", &lightColor.x);
    
    // Cross-section settings
    ImGui::Separator();
    ImGui::Text("Cross-Section View:");
    ImGui::Checkbox("Show Cross-Section", &showCrossSection);
    
    if (showCrossSection) {
        const char* axes[] = { "X-Axis", "Y-Axis", "Z-Axis" };
        ImGui::Combo("Section Axis", &crossSectionAxis, axes, IM_ARRAYSIZE(axes));
        ImGui::SliderFloat("Section Position", &crossSectionPos, -1.0f, 1.0f);
    }
    
    // ImGui::End(); // Removed, as Begin/End is handled in main.cpp
}
