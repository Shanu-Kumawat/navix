#include "ShockAbsorberViewer3D.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <iostream> // Added for print statements

ShockAbsorberViewer3D::ShockAbsorberViewer3D()
    : framebuffer(0), textureColorBuffer(0), rbo(0), renderMode(0), objectColor(0.8f, 0.8f, 0.8f), lightColor(1.0f, 1.0f, 1.0f),
      ambientStrength(0.4f), diffuseStrength(0.7f), specularStrength(0.7f), shininess(32.0f), mousePressed(false), lastX(0.0f), lastY(0.0f), viewportWidth(800), viewportHeight(600) {}

ShockAbsorberViewer3D::~ShockAbsorberViewer3D() {
    if (framebuffer) glDeleteFramebuffers(1, &framebuffer);
    if (textureColorBuffer) glDeleteTextures(1, &textureColorBuffer);
    if (rbo) glDeleteRenderbuffers(1, &rbo);
}

void ShockAbsorberViewer3D::initialize() {
    std::cout << "Initializing ShockAbsorberViewer3D..." << std::endl;
    // Place camera at (0, 0, 5) looking at the origin for better visibility
    camera = std::make_unique<Camera>(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);
    shader = std::make_unique<Shader>("shaders/bellows.vs", "shaders/bellows.fs");
    model = std::make_unique<ShockAbsorberModel3D>();
    model->setShader(shader.get());
    setupFramebuffer(viewportWidth, viewportHeight);
}

Camera* ShockAbsorberViewer3D::getCamera() { return camera.get(); }

void ShockAbsorberViewer3D::setupFramebuffer(int width, int height) {
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
        // Error handling
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShockAbsorberViewer3D::resizeFramebuffer(int width, int height) {
    if (width != viewportWidth || height != viewportHeight) {
        viewportWidth = width;
        viewportHeight = height;
        setupFramebuffer(width, height);
    }
}

void ShockAbsorberViewer3D::render(const Drawing::Spring2D* spring, const Drawing::ShockAbsorberEnd2D* end, const Drawing::ShockAbsorberBottomEnd* bottomEnd, ImVec2 windowSize) {
    if (!model || !camera || !shader) return;
    // Update framebuffer if window size changed
    resizeFramebuffer(static_cast<int>(windowSize.x), static_cast<int>(windowSize.y));
    // Generate/update 3D mesh
    model->generateMesh(spring, end, bottomEnd);
    // Set material and lighting properties
    model->setMaterial(objectColor, ambientStrength, diffuseStrength, specularStrength, shininess);
    model->setLight(glm::vec3(2.0f, 3.0f, 2.0f), lightColor);
    model->setRenderMode(renderMode);
    // Bind framebuffer and render to it
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, viewportWidth, viewportHeight);
    // Create projection matrix
    float aspectRatio = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);
    glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom), aspectRatio, 0.1f, 100.0f);
    glm::mat4 view = camera->GetViewMatrix();
    model->render(projection, view, camera->Position);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // Display the rendered texture in ImGui
    ImGui::Image((ImTextureID)(intptr_t)textureColorBuffer, windowSize, ImVec2(0, 1), ImVec2(1, 0));
}

void ShockAbsorberViewer3D::handleInput(const SDL_Event& event) {
    // TODO: Implement input handling for camera and model interaction
} 