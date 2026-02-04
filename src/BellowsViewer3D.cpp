#include "BellowsViewer3D.hpp"
#include <iostream>

BellowsViewer3D::BellowsViewer3D() : Base3DViewer() {
    // Initialize bellows-specific default colors
    objectColor = glm::vec3(0.8f, 0.8f, 0.8f);
    lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    ambientStrength = 0.4f;
    diffuseStrength = 0.7f;
    specularStrength = 0.7f;
    shininess = 32.0f;
}

void BellowsViewer3D::initialize() {
    // Initialize camera with bellows-specific positioning
    camera = std::make_unique<Camera>(glm::vec3(2.0f, 1.5f, 3.0f), glm::vec3(0.0f, 1.0f, 0.0f), -110.0f, -15.0f);
    
    // Create shader
    shader = std::make_unique<Shader>("shaders/bellows.vs", "shaders/bellows.fs");
    
    // Create 3D model
    bellowsModel = std::make_unique<BellowsModel3D>();
    bellowsModel->setShader(shader.get());
    
    // Setup initial framebuffer
    setupFramebuffer(viewportWidth, viewportHeight);
}

void BellowsViewer3D::render(const Drawing::Bellows* bellows, ImVec2 windowSize) {
    if (!bellowsModel || !camera || !shader) return;
    
    // Update framebuffer if window size changed
    resizeFramebuffer(static_cast<int>(windowSize.x), static_cast<int>(windowSize.y));
    
    // Generate/update 3D mesh
    bellowsModel->generateMesh(bellows);
    
    // Set material and lighting properties
    bellowsModel->setMaterial(objectColor, ambientStrength, diffuseStrength, specularStrength, shininess);
    bellowsModel->setLight(glm::vec3(2.0f, 3.0f, 2.0f), lightColor);
    bellowsModel->setRenderMode(renderMode);
    
    // Set cross-section settings
    bellowsModel->enableCrossSection(showCrossSection);
    bellowsModel->setCrossSectionAxis(crossSectionAxis);
    bellowsModel->setCrossSectionPosition(crossSectionPos);
    
    // Bind framebuffer and render
    bindFramebufferForRendering();
    
    // Create projection matrix
    float aspectRatio = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);
    glm::mat4 projection = createProjectionMatrix(aspectRatio);
    glm::mat4 view = camera->GetViewMatrix();
    
    // Render the 3D model
    bellowsModel->render(projection, view, camera->Position);
    
    // Unbind framebuffer
    unbindFramebuffer();
    
    // Display the rendered texture
    displayRenderedTexture(windowSize);
    
    // Handle mouse interactions for camera control using ImGui
    if (ImGui::IsWindowHovered()) {
        ImGuiIO& io = ImGui::GetIO();
        
        // Handle mouse drag for camera rotation
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
            if (delta.x != 0.0f || delta.y != 0.0f) {
                camera->ProcessMouseMovement(delta.x, -delta.y);
                ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
            }
        }
        
        // Handle mouse wheel for zoom
        if (io.MouseWheel != 0.0f) {
            camera->ProcessMouseScroll(io.MouseWheel);
        }
        
        // Handle keyboard input for camera movement
        float deltaTime = 0.1f;
        if (ImGui::IsKeyDown(ImGuiKey_W)) camera->ProcessKeyboard(FORWARD, deltaTime);
        if (ImGui::IsKeyDown(ImGuiKey_S)) camera->ProcessKeyboard(BACKWARD, deltaTime);
        if (ImGui::IsKeyDown(ImGuiKey_A)) camera->ProcessKeyboard(LEFT, deltaTime);
        if (ImGui::IsKeyDown(ImGuiKey_D)) camera->ProcessKeyboard(RIGHT, deltaTime);
        if (ImGui::IsKeyPressed(ImGuiKey_R)) camera->Reset();
    }
}

void BellowsViewer3D::handleInput(const SDL_Event& event) {
    if (!camera) return;
    
    // Handle standard mouse input
    processStandardMouseInput(event);
    
    // Handle bellows-specific model interaction
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        if (event.button.button == SDL_BUTTON_LEFT) {
            bellowsModel->processMousePress(true, lastX, lastY);
        }
    } else if (event.type == SDL_MOUSEBUTTONUP) {
        if (event.button.button == SDL_BUTTON_LEFT) {
            bellowsModel->processMousePress(false, lastX, lastY);
        }
    } else if (event.type == SDL_MOUSEMOTION) {
        if (mousePressed) {
            bellowsModel->processMouseMovement(lastX, lastY);
        }
    } else if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
            case SDLK_r:
                // Reset both camera and model view
                camera->Reset();
                bellowsModel->resetView();
                break;
            default:
                // Handle standard keyboard input
                processStandardKeyboardInput(event);
                break;
        }
    }
}
