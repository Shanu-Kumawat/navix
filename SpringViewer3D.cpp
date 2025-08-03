#include "SpringViewer3D.hpp"

SpringViewer3D::SpringViewer3D() {}
SpringViewer3D::~SpringViewer3D() {}

void SpringViewer3D::initialize() {}

void SpringViewer3D::render(const Drawing::Spring2D* spring, ImVec2 windowSize) {}

void SpringViewer3D::handleInput(const SDL_Event& event) {}

Camera* SpringViewer3D::getCamera() { return camera.get(); }

void SpringViewer3D::setupFramebuffer(int width, int height) {}
void SpringViewer3D::resizeFramebuffer(int width, int height) {} 