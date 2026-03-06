#include "utils/VectorMath.hpp"
#include <glm/glm.hpp>
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
    
    // Initialize camera with shock absorber-specific positioning (use same as Spring3D which works)
    camera = std::make_unique<Camera>(glm::vec3(2.0f, 1.5f, 3.0f), glm::vec3(0.0f, 1.0f, 0.0f), -110.0f, -15.0f);
    
    // Create shader (reuse bellows shader)
    shader = std::make_unique<Shader>("shaders/bellows.vs", "shaders/bellows.fs");
    
    // Create 3D model
    shockAbsorberModel = std::make_unique<ShockAbsorberModel3D>();
    shockAbsorberModel->setShader(shader.get());
    
    // Setup initial framebuffer
    setupFramebuffer(viewportWidth, viewportHeight);
    
    std::cout << "ShockAbsorberViewer3D initialization complete." << std::endl;
}

void ShockAbsorberViewer3D::render(const Drawing::Spring2D* spring, const Drawing::ShockAbsorberEnd2D* end, 
                                   const Drawing::ShockAbsorberBottomEnd* bottomEnd, glm::dvec2 windowSize) {
    if (!shockAbsorberModel || !camera || !shader) {
        std::cout << "ShockAbsorberViewer3D::render - Missing components: "
                  << "model=" << (shockAbsorberModel ? "OK" : "NULL") 
                  << ", camera=" << (camera ? "OK" : "NULL")
                  << ", shader=" << (shader ? "OK" : "NULL") << std::endl;
        return;
    }
    
    // Update framebuffer if window size changed
    resizeFramebuffer(static_cast<int>(windowSize.x), static_cast<int>(windowSize.y));
    
    // Generate/update 3D mesh
    shockAbsorberModel->generateMesh(spring, end, bottomEnd);
    
    // FEM mesh is generated on demand via PropertyPanel — not auto-generated here
    
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
    
    // Render FEM mesh wireframe overlay if enabled
    shockAbsorberModel->renderFEMMeshWireframe(projection, view, camera->Position);
    
    // Unbind framebuffer
    unbindFramebuffer();
    
    // Display the rendered texture
    displayRenderedTexture(windowSize);
    
    // Handle mouse interactions for camera control using ImGui
    if (ImGui::IsWindowHovered()) {
        ImGuiIO& io = ImGui::GetIO();
        
        // Handle mouse drag for camera rotation
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            glm::dvec2 delta = Drawing::Math::toDVec2(ImGui::GetMouseDragDelta(ImGuiMouseButton_Left));
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
