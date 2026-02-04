#include "SpringViewer3D.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <iostream>

SpringViewer3D::SpringViewer3D()
    : framebuffer(0), textureColorBuffer(0), rbo(0), renderMode(0), objectColor(0.8f, 0.8f, 0.8f),
      lightColor(1.0f, 1.0f, 1.0f), ambientStrength(0.4f), diffuseStrength(0.7f), specularStrength(0.7f), shininess(32.0f),
      showCrossSection(false), crossSectionAxis(0), crossSectionPos(0.0f), mousePressed(false), lastX(0.0f), lastY(0.0f),
      viewportWidth(800), viewportHeight(600) {}

SpringViewer3D::~SpringViewer3D() {
    if (framebuffer) glDeleteFramebuffers(1, &framebuffer);
    if (textureColorBuffer) glDeleteTextures(1, &textureColorBuffer);
    if (rbo) glDeleteRenderbuffers(1, &rbo);
}

void SpringViewer3D::initialize() {
    camera = std::make_unique<Camera>(glm::vec3(2.0f, 1.5f, 3.0f), glm::vec3(0.0f, 1.0f, 0.0f), -110.0f, -15.0f);
    shader = std::make_unique<Shader>("shaders/bellows.vs", "shaders/bellows.fs");
    springModel = std::make_unique<SpringModel3D>();
    springModel->setShader(shader.get());
    setupFramebuffer(viewportWidth, viewportHeight);
}

Camera* SpringViewer3D::getCamera() { return camera.get(); }

void SpringViewer3D::setupFramebuffer(int width, int height) {
    if (width <= 0 || height <= 0) return;
    if (framebuffer) {
        glDeleteFramebuffers(1, &framebuffer);
        glDeleteTextures(1, &textureColorBuffer);
        glDeleteRenderbuffers(1, &rbo);
    }
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glGenTextures(1, &textureColorBuffer);
    glBindTexture(GL_TEXTURE_2D, textureColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorBuffer, 0);
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SpringViewer3D::resizeFramebuffer(int width, int height) {
    if (width != viewportWidth || height != viewportHeight) {
        viewportWidth = width;
        viewportHeight = height;
        setupFramebuffer(width, height);
    }
}

void SpringViewer3D::render(const Drawing::Spring2D* spring, ImVec2 windowSize) {
    if (!springModel || !camera || !shader) {
        ImGui::Text("Spring model, camera, or shader not initialized!");
        return;
    }

    // Update framebuffer if window size changed
    int width = static_cast<int>(windowSize.x);
    int height = static_cast<int>(windowSize.y);
    if (width <= 0) width = 800;
    if (height <= 0) height = 600;
    resizeFramebuffer(width, height);

    // Use selected spring or a default visible spring
    Drawing::Spring2D defaultSpring(0.0f, 0.0f, 100.0f, 10.0f, 200.0f, 8);
    const Drawing::Spring2D* springToRender = spring ? spring : &defaultSpring;
    if (!spring || spring->outerDiameter <= 0.0f || spring->wireDiameter <= 0.0f ||
        spring->freeLength <= 0.0f || spring->numCoils <= 0 ||
        spring->outerDiameter <= spring->wireDiameter) {
        springToRender = &defaultSpring;
    }
    
    springModel->generateMesh(springToRender);
    springModel->setMaterial(objectColor, ambientStrength, diffuseStrength, specularStrength, shininess);
    springModel->setLight(glm::vec3(2.0f, 3.0f, 2.0f), lightColor);
    springModel->setRenderMode(renderMode);

    // Bind framebuffer and render
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        ImGui::Text("Framebuffer error!");
        return;
    }
    
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, width, height);
    
    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom), aspectRatio, 0.1f, 100.0f);
    glm::mat4 view = camera->GetViewMatrix();
    springModel->render(projection, view, camera->Position);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Display the rendered texture
    ImGui::Image((ImTextureID)(intptr_t)textureColorBuffer, windowSize, ImVec2(0, 1), ImVec2(1, 0));

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

void SpringViewer3D::handleInput(const SDL_Event& event) {
    if (!camera) return;
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        if (event.button.button == SDL_BUTTON_LEFT) {
            mousePressed = true;
            lastX = static_cast<float>(event.button.x);
            lastY = static_cast<float>(event.button.y);
            if (springModel) springModel->processMousePress(true, lastX, lastY);
        }
    } else if (event.type == SDL_MOUSEBUTTONUP) {
        if (event.button.button == SDL_BUTTON_LEFT) {
            mousePressed = false;
            if (springModel) springModel->processMousePress(false, 0.0f, 0.0f);
        }
    } else if (event.type == SDL_MOUSEWHEEL) {
        camera->ProcessMouseScroll(static_cast<float>(event.wheel.y));
        if (springModel) springModel->processMouseScroll(static_cast<float>(event.wheel.y));
    } else if (event.type == SDL_KEYDOWN) {
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
                camera->Reset();
                if (springModel) springModel->resetView();
                break;
        }
    }
} 