#include <glm/glm.hpp>
#include "ComplexShape3DManager.hpp"
#include <imgui.h>
#include <iostream>
#include <functional>

void ComplexShape3DManager::renderBellows3DView(const Drawing::Bellows* bellows, bool& showFlag) {
    if (!showFlag || !bellows) return;

    ensureBellowsViewerInitialized();

    renderViewerWindow("3D Bellows Viewport", showFlag, [this, bellows](glm::dvec2 viewportSize) {
        bellowsViewer->render(bellows, viewportSize);
    });
}

void ComplexShape3DManager::renderBallBearing3DView(const Drawing::BallBearing* ballBearing, bool& showFlag) {
    if (!showFlag || !ballBearing) return;

    ensureBallBearingViewerInitialized();

    renderViewerWindow("3D Ball Bearing Viewport", showFlag, [this, ballBearing](glm::dvec2 viewportSize) {
        ballBearingViewer->render(ballBearing, viewportSize);
    });
}

void ComplexShape3DManager::renderShockAbsorber3DView(const Drawing::Spring2D* spring, 
                                                      const Drawing::ShockAbsorberEnd2D* end,
                                                      const Drawing::ShockAbsorberBottomEnd* bottomEnd, 
                                                      bool& showFlag) {
    if (!showFlag || !spring || !end || !bottomEnd) {
        if (showFlag && (!spring || !end || !bottomEnd)) {
            // Show error message in a window
            ImGui::SetNextWindowSize(glm::dvec2(400, 150), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, glm::dvec2(0.5f, 0.5f));
            
            if (ImGui::Begin("Shock Absorber 3D View - Missing Components", &showFlag)) {
                ImGui::TextWrapped("Please add all required components to view the 3D shock absorber:");
                ImGui::BulletText("Spring2D");
                ImGui::BulletText("ShockAbsorberEnd2D");  
                ImGui::BulletText("ShockAbsorberBottomEnd");
                
                if (ImGui::Button("Close")) {
                    showFlag = false;
                }
            }
            ImGui::End();
        }
        return;
    }

    ensureShockAbsorberViewerInitialized();

    renderViewerWindow("3D Shock Absorber Viewport", showFlag, [this, spring, end, bottomEnd](glm::dvec2 viewportSize) {
        shockAbsorberViewer->render(spring, end, bottomEnd, viewportSize);
    });
}

void ComplexShape3DManager::handleInput(const SDL_Event& event) {
    // Check if ImGui wants to capture input
    ImGuiIO& io = ImGui::GetIO();
    bool allowKeyboardInput = !io.WantCaptureKeyboard;

    // Pass input to initialized viewers
    if ((event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) && allowKeyboardInput) {
        if (bellowsViewer && viewerInitialized[Shape3DType::BELLOWS]) {
            bellowsViewer->handleInput(event);
        }
        if (ballBearingViewer && viewerInitialized[Shape3DType::BALL_BEARING]) {
            ballBearingViewer->handleInput(event);
        }
        if (shockAbsorberViewer && viewerInitialized[Shape3DType::SHOCK_ABSORBER]) {
            shockAbsorberViewer->handleInput(event);
        }
    }
    // Always pass mouse events
    else if (event.type == SDL_MOUSEWHEEL || event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
        if (bellowsViewer && viewerInitialized[Shape3DType::BELLOWS]) {
            bellowsViewer->handleInput(event);
        }
        if (ballBearingViewer && viewerInitialized[Shape3DType::BALL_BEARING]) {
            ballBearingViewer->handleInput(event);
        }
        if (shockAbsorberViewer && viewerInitialized[Shape3DType::SHOCK_ABSORBER]) {
            shockAbsorberViewer->handleInput(event);
        }
    }
}

void ComplexShape3DManager::resetViewerState(Shape3DType type) {
    viewerInitialized[type] = false;
}

void ComplexShape3DManager::resetAllViewerStates() {
    viewerInitialized.clear();
}

bool ComplexShape3DManager::render3DViewButton(const std::string& label, const std::string& tooltip) {
    // Standardized 3D view button styling
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.6f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.5f, 0.7f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.3f, 0.5f, 1.0f));
    
    bool clicked = ImGui::Button(label.c_str(), glm::dvec2(0, 30));
    
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tooltip.c_str());
    }
    
    ImGui::PopStyleColor(3);
    return clicked;
}

void ComplexShape3DManager::ensureBellowsViewerInitialized() {
    if (!bellowsViewer) {
        bellowsViewer = std::make_unique<BellowsViewer3D>();
    }
    
    if (!viewerInitialized[Shape3DType::BELLOWS]) {
        bellowsViewer->initialize();
        viewerInitialized[Shape3DType::BELLOWS] = true;
    }
}

void ComplexShape3DManager::ensureBallBearingViewerInitialized() {
    if (!ballBearingViewer) {
        ballBearingViewer = std::make_unique<BallBearingViewer3D>();
    }
    
    if (!viewerInitialized[Shape3DType::BALL_BEARING]) {
        ballBearingViewer->initialize();
        viewerInitialized[Shape3DType::BALL_BEARING] = true;
    }
}

void ComplexShape3DManager::ensureShockAbsorberViewerInitialized() {
    if (!shockAbsorberViewer) {
        shockAbsorberViewer = std::make_unique<ShockAbsorberViewer3D>();
    }
    
    if (!viewerInitialized[Shape3DType::SHOCK_ABSORBER]) {
        shockAbsorberViewer->initialize();
        viewerInitialized[Shape3DType::SHOCK_ABSORBER] = true;
    }
}

void ComplexShape3DManager::renderViewerWindow(const std::string& title, bool& showFlag, 
                                               std::function<void(glm::dvec2)> renderCallback) {
    // Standardized window setup
    glm::dvec2 screenSize = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowSize(glm::dvec2(screenSize.x * 0.75f, screenSize.y * 0.75f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(glm::dvec2(screenSize.x * 0.5f, screenSize.y * 0.5f), ImGuiCond_FirstUseEver, glm::dvec2(0.5f, 0.5f));
    
    // Remove padding for full viewport rendering
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, glm::dvec2(0, 0));
    
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    
    if (ImGui::Begin(title.c_str(), &showFlag, flags)) {
        glm::dvec2 viewportSize = ImGui::GetContentRegionAvail();
        
        // Call the rendering callback
        renderCallback(viewportSize);
        
        // Standardized mouse rotation handling
        if (ImGui::IsWindowHovered()) {
            ImGui::SetWindowFocus();
            
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                glm::dvec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
                ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
                
                if (std::abs(delta.x) > 0.1f || std::abs(delta.y) > 0.1f) {
                    // Apply rotation to the appropriate viewer's camera
                    // The renderCallback will handle the specific viewer logic
                }
            }
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(); // WindowPadding
}
