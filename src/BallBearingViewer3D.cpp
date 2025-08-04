#include "BallBearingViewer3D.hpp"
#include <iostream>

BallBearingViewer3D::BallBearingViewer3D() : Base3DViewer() {
    // Initialize ball bearing-specific default colors (metallic appearance)
    objectColor = glm::vec3(0.9f, 0.9f, 0.95f);
    lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    ambientStrength = 0.25f;
    diffuseStrength = 0.8f;
    specularStrength = 0.9f;
    shininess = 128.0f;
}

void BallBearingViewer3D::initialize() {
    // Initialize camera with ball bearing-specific positioning
    camera = std::make_unique<Camera>(glm::vec3(2.0f, 1.5f, 3.0f), glm::vec3(0.0f, 1.0f, 0.0f), -110.0f, -15.0f);
    
    // Create shader (reuse bellows shader as it's compatible)
    shader = std::make_unique<Shader>("shaders/bellows.vs", "shaders/bellows.fs");
    
    // Create 3D model
    ballBearingModel = std::make_unique<BallBearingModel3D>();
    ballBearingModel->setShader(shader.get());
    
    // Setup initial framebuffer
    setupFramebuffer(viewportWidth, viewportHeight);
}

void BallBearingViewer3D::render(const Drawing::BallBearing* ballBearing, ImVec2 windowSize) {
    if (!ballBearingModel || !camera || !shader) return;
    
    // Update framebuffer if window size changed
    resizeFramebuffer(static_cast<int>(windowSize.x), static_cast<int>(windowSize.y));
    
    // Generate/update 3D mesh
    ballBearingModel->generateMesh(ballBearing);
    
    // Set material and lighting properties for high-quality metallic rendering
    ballBearingModel->setMaterial(objectColor, ambientStrength, diffuseStrength, specularStrength, shininess);
    ballBearingModel->setLight(glm::vec3(3.0f, 4.0f, 3.0f), lightColor);
    ballBearingModel->setRenderMode(renderMode);
    
    // Disable cross-section for now (can be enabled later)
    ballBearingModel->enableCrossSection(false);
    
    // Bind framebuffer and render
    bindFramebufferForRendering();
    
    // Use darker background for better contrast with metallic surfaces
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Create projection matrix
    float aspectRatio = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);
    glm::mat4 projection = createProjectionMatrix(aspectRatio);
    glm::mat4 view = camera->GetViewMatrix();
    
    // Render the 3D model
    ballBearingModel->render(projection, view, camera->Position);
    
    // Unbind framebuffer
    unbindFramebuffer();
    
    // Display the rendered texture
    displayRenderedTexture(windowSize);
    
    // Handle mouse interactions if over viewport
    if (ImGui::IsWindowHovered() && mousePressed) {
        ImVec2 mousePos = ImGui::GetIO().MousePos;
        ImVec2 windowPos = ImGui::GetWindowPos();
        float mouseX = mousePos.x - windowPos.x;
        float mouseY = mousePos.y - windowPos.y;
        ballBearingModel->processMouseMovement(mouseX, mouseY);
    }
}

void BallBearingViewer3D::handleInput(const SDL_Event& event) {
    if (!camera) return;
    
    // Handle standard mouse input
    processStandardMouseInput(event);
    
    // Handle ball bearing-specific model interaction
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        if (event.button.button == SDL_BUTTON_LEFT) {
            ballBearingModel->processMousePress(true, lastX, lastY);
        }
    } else if (event.type == SDL_MOUSEBUTTONUP) {
        if (event.button.button == SDL_BUTTON_LEFT) {
            ballBearingModel->processMousePress(false, lastX, lastY);
        }
    } else if (event.type == SDL_MOUSEMOTION) {
        if (mousePressed) {
            ballBearingModel->processMouseMovement(lastX, lastY);
        }
    } else if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
            case SDLK_r:
                // Reset both camera and model view
                camera->Reset();
                ballBearingModel->resetView();
                break;
            default:
                // Handle standard keyboard input
                processStandardKeyboardInput(event);
                break;
        }
    }
}
