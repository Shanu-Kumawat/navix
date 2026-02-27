#include "ui/Viewers3DUI.hpp"
#include "ui/UIState.hpp"
#include <imgui.h>
#include <iostream>

void UI::Viewers3DUI::RenderSpring3DViewWindow(Drawing::Canvas &canvas, Core::ApplicationContext& appContext) {
  if (!UIState::spring3DViewInitialized) {
    UIState::springViewer.initialize();
    UIState::spring3DViewInitialized = true;
  }
  
  if (ImGui::Begin("Spring 3D View", &appContext.showSpring3DView)) {
    // Get available content region size for dynamic viewport
    glm::dvec2 availableSize = Drawing::Math::toDVec2(ImGui::GetContentRegionAvail());
    glm::dvec2 viewportSize = glm::dvec2(availableSize.x, availableSize.y - 20); // Leave some margin
    
    // Get the currently selected Spring2D from the canvas
    const Drawing::Shape* selectedShape = canvas.getSelectedShape();
    if (selectedShape && selectedShape->type == Drawing::ShapeType::SPRING2D) {
      const Drawing::Spring2D* spring = static_cast<const Drawing::Spring2D*>(selectedShape);
      UIState::springViewer.render(spring, viewportSize);
    } else {
      ImGui::Text("No Spring2D selected. Please select a Spring2D to view in 3D.");
    }
  }
  ImGui::End();
}

void UI::Viewers3DUI::RenderShockAbsorber3DViewWindow(Drawing::Canvas &canvas, Core::ApplicationContext& appContext) {
  if (!UIState::shockAbsorberViewerInitialized) {
    UIState::shockAbsorberViewer.initialize();
    UIState::shockAbsorberViewerInitialized = true;
  }
  
  if (ImGui::Begin("Shock Absorber 3D View", &appContext.showShockAbsorber3DView)) {
    // Get available content region size for dynamic viewport
    glm::dvec2 availableSize = Drawing::Math::toDVec2(ImGui::GetContentRegionAvail());
    glm::dvec2 viewportSize = glm::dvec2(availableSize.x, availableSize.y - 20); // Leave some margin
    
    // Find complete shock absorber assemblies
    auto assemblies = canvas.findShockAbsorberAssemblies();
    if (!assemblies.empty()) {
      // Render the first complete assembly
      const auto& assembly = assemblies[0];
      UIState::shockAbsorberViewer.render(assembly.spring, assembly.topEnd, assembly.bottomEnd, viewportSize);
    } else {
      ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No complete shock absorber assembly found.");
      ImGui::Spacing();
      ImGui::TextWrapped("To create a shock absorber assembly:");
      ImGui::BulletText("1. Draw a Spring (use Spring tool)");
      ImGui::BulletText("2. Select the Spring");
      ImGui::BulletText("3. Click 'Add Top End' button");
      ImGui::BulletText("4. Click 'Add Bottom End' button");
    }
  }
  ImGui::End();
}

void UI::Viewers3DUI::RenderBellows3DViewWindow(Drawing::Canvas &canvas) {
  if (!UIState::bellows3DViewInitialized) {
    UIState::bellowsViewer.initialize();
    UIState::bellows3DViewInitialized = true;
  }
  
  if (ImGui::Begin("Bellows 3D View", &UIState::showBellows3DView)) {
    // Get available content region size for dynamic viewport
    glm::dvec2 availableSize = Drawing::Math::toDVec2(ImGui::GetContentRegionAvail());
    glm::dvec2 viewportSize = glm::dvec2(availableSize.x, availableSize.y - 20); // Leave some margin
    
    // Get the currently selected Bellows from the canvas
    const Drawing::Shape* selectedShape = canvas.getSelectedShape();
    if (selectedShape && selectedShape->type == Drawing::ShapeType::BELLOWS) {
      const Drawing::Bellows* bellows = static_cast<const Drawing::Bellows*>(selectedShape);
      UIState::bellowsViewer.render(bellows, viewportSize);
    } else {
      ImGui::Text("No Bellows selected. Please select a Bellows to view in 3D.");
    }
  }
  ImGui::End();
}

void UI::Viewers3DUI::RenderBallBearing3DViewWindow(Drawing::Canvas &canvas) {
  if (!UIState::ballBearing3DViewInitialized) {
    UIState::ballBearingViewer.initialize();
    UIState::ballBearing3DViewInitialized = true;
  }
  
  if (ImGui::Begin("Ball Bearing 3D View", &UIState::showBallBearing3DView)) {
    // Get available content region size for dynamic viewport
    glm::dvec2 availableSize = Drawing::Math::toDVec2(ImGui::GetContentRegionAvail());
    glm::dvec2 viewportSize = glm::dvec2(availableSize.x, availableSize.y - 20); // Leave some margin
    
    // Get the currently selected Ball Bearing from the canvas
    const Drawing::Shape* selectedShape = canvas.getSelectedShape();
    if (selectedShape && selectedShape->type == Drawing::ShapeType::BALL_BEARING) {
      const Drawing::BallBearing* ballBearing = static_cast<const Drawing::BallBearing*>(selectedShape);
      UIState::ballBearingViewer.render(ballBearing, viewportSize);
    } else {
      ImGui::Text("No Ball Bearing selected. Please select a Ball Bearing to view in 3D.");
    }
  }
  ImGui::End();
}

