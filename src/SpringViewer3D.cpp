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

void SpringViewer3D::render(const Drawing::Spring2D* spring, ImVec2 /*windowSize*/) {
    ImGui::Text("3D Spring Viewport Active");

    if (!springModel || !camera || !shader) {
        ImGui::Text("Spring model, camera, or shader not initialized!");
        return;
    }

    // Debug panel: show spring parameters
    if (spring) {
        ImGui::Text("Spring2D parameters:");
        ImGui::Text("centerX: %.2f", spring->centerX);
        ImGui::Text("centerY: %.2f", spring->centerY);
        ImGui::Text("outerDiameter: %.2f", spring->outerDiameter);
        ImGui::Text("wireDiameter: %.2f", spring->wireDiameter);
        ImGui::Text("freeLength: %.2f", spring->freeLength);
        ImGui::Text("numCoils: %d", spring->numCoils);
    } else {
        ImGui::Text("No 2D spring selected! Showing default spring.");
    }

    // Use a fixed framebuffer size
    int fixedWidth = 800;
    int fixedHeight = 600;
    resizeFramebuffer(fixedWidth, fixedHeight);

    // Use selected spring or a default visible spring
    Drawing::Spring2D defaultSpring(0.0f, 0.0f, 100.0f, 10.0f, 200.0f, 8);
    const Drawing::Spring2D* springToRender = spring ? spring : &defaultSpring;
    bool usedDefault = false;
    if (!spring || spring->outerDiameter <= 0.0f || spring->wireDiameter <= 0.0f ||
        spring->freeLength <= 0.0f || spring->numCoils <= 0 ||
        spring->outerDiameter <= spring->wireDiameter) {
        springToRender = &defaultSpring;
        usedDefault = true;
    }
    springModel->generateMesh(springToRender);
    springModel->setMaterial(objectColor, ambientStrength, diffuseStrength, specularStrength, shininess);
    springModel->setLight(glm::vec3(2.0f, 3.0f, 2.0f), lightColor);
    springModel->setRenderMode(renderMode);

    if (usedDefault) {
        ImGui::TextColored(ImVec4(1,0.5,0.5,1), "Invalid spring parameters! Showing default spring.");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        ImGui::Text("Framebuffer incomplete! (width: %d, height: %d)", fixedWidth, fixedHeight);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }
    if (!springModel || !springModel->hasMesh()) {
        ImGui::Text("No mesh to display!");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, fixedWidth, fixedHeight);
    float aspectRatio = static_cast<float>(fixedWidth) / static_cast<float>(fixedHeight);
    glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom), aspectRatio, 0.1f, 100.0f);
    glm::mat4 view = camera->GetViewMatrix();
    springModel->render(projection, view, camera->Position);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    ImGui::Image((ImTextureID)(intptr_t)textureColorBuffer, ImVec2(fixedWidth, fixedHeight), ImVec2(0, 1), ImVec2(1, 0));

    ImVec2 mousePos = ImGui::GetIO().MousePos;
    ImVec2 windowPos = ImGui::GetWindowPos();
    if (ImGui::IsWindowHovered() && mousePressed) {
        float mouseX = mousePos.x - windowPos.x;
        float mouseY = mousePos.y - windowPos.y;
        springModel->processMouseMovement(mouseX, mouseY);
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