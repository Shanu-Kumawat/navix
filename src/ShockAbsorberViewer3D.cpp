#include "ShockAbsorberViewer3D.hpp"
#include <iostream>

ShockAbsorberViewer3D::ShockAbsorberViewer3D() : Base3DViewer() {
    // Initialize shock absorber-specific default colors
    objectColor = glm::vec3(0.8f, 0.8f, 0.8f);
    lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    ambientStrength = 0.4f;
    diffuseStrength = 0.7f;
    specularStrength = 0.7f;
    shininess = 32.0f;
}

void ShockAbsorberViewer3D::initialize() {
    std::cout << "Initializing ShockAbsorberViewer3D with standardized architecture..." << std::endl;
    
    // Initialize camera with shock absorber-specific positioning
    camera = std::make_unique<Camera>(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);
    
    // Create shader (reuse bellows shader)
    shader = std::make_unique<Shader>("shaders/bellows.vs", "shaders/bellows.fs");
    
    // Create 3D model
    shockAbsorberModel = std::make_unique<ShockAbsorberModel3D>();
    shockAbsorberModel->setShader(shader.get());
    
    // Setup initial framebuffer
    setupFramebuffer(viewportWidth, viewportHeight);
}

void ShockAbsorberViewer3D::render(const Drawing::Spring2D* spring, const Drawing::ShockAbsorberEnd2D* end, 
                                   const Drawing::ShockAbsorberBottomEnd* bottomEnd, ImVec2 windowSize) {
    if (!shockAbsorberModel || !camera || !shader) return;
    
    // Update framebuffer if window size changed
    resizeFramebuffer(static_cast<int>(windowSize.x), static_cast<int>(windowSize.y));
    
    // Generate/update 3D mesh
    shockAbsorberModel->generateMesh(spring, end, bottomEnd);
    
    // Set material and lighting properties
    shockAbsorberModel->setMaterial(objectColor, ambientStrength, diffuseStrength, specularStrength, shininess);
    shockAbsorberModel->setLight(glm::vec3(2.0f, 3.0f, 2.0f), lightColor);
    shockAbsorberModel->setRenderMode(renderMode);
    
    // Bind framebuffer and render
    bindFramebufferForRendering();
    
    // Create projection matrix
    float aspectRatio = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);
    glm::mat4 projection = createProjectionMatrix(aspectRatio);
    glm::mat4 view = camera->GetViewMatrix();
    
    // Render the 3D model
    shockAbsorberModel->render(projection, view, camera->Position);
    
    // Unbind framebuffer
    unbindFramebuffer();
    
    // Display the rendered texture
    displayRenderedTexture(windowSize);
}

void ShockAbsorberViewer3D::handleInput(const SDL_Event& event) {
    if (!camera) return;
    
    // Handle standard mouse and keyboard input
    processStandardMouseInput(event);
    processStandardKeyboardInput(event);
    
    // Handle shock absorber-specific model interaction
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        if (event.button.button == SDL_BUTTON_LEFT) {
            shockAbsorberModel->processMousePress(true, lastX, lastY);
        }
    } else if (event.type == SDL_MOUSEBUTTONUP) {
        if (event.button.button == SDL_BUTTON_LEFT) {
            shockAbsorberModel->processMousePress(false, lastX, lastY);
        }
    } else if (event.type == SDL_MOUSEMOTION) {
        if (mousePressed) {
            shockAbsorberModel->processMouseMovement(lastX, lastY);
        }
    } else if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
            case SDLK_r:
                // Reset both camera and model view
                camera->Reset();
                shockAbsorberModel->resetView();
                break;
        }
    }
}
