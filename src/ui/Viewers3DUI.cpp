#include "ui/Viewers3DUI.hpp"
#include "Base3DModel.hpp"
#include <imgui.h>
#include <iostream>

// Helper: Render FEM mesh controls for a 3D model
static void RenderMeshControls(Base3DModel* model, const char* label) {
    if (!model) return;
    ImGui::Separator();
    ImGui::Text("FEM Mesh (%s)", label);
    
    static float meshElemSize = 0.05f;
    ImGui::SliderFloat("Element Size##mesh3d", &meshElemSize, 0.01f, 0.5f, "%.3f");
    
    if (ImGui::Button("Generate 3D Mesh")) {
        model->generateFEMMesh(meshElemSize);
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear Mesh##3d")) {
        model->clearFEMMesh();
    }
    
    if (model->hasFEMMesh()) {
        bool show = model->getShowFEMMesh();
        if (ImGui::Checkbox("Show Mesh##3d", &show)) {
            model->setShowFEMMesh(show);
        }
    }
}

void UI::Viewers3DUI::RenderSpring3DViewWindow(Drawing::Canvas &canvas, Core::ApplicationContext& appContext) {
  if (!appContext.spring3DViewInitialized) {
    appContext.springViewer.initialize();
    appContext.spring3DViewInitialized = true;
  }
  
  if (ImGui::Begin("Spring 3D View", &appContext.showSpring3DView)) {
    // Get available content region size for dynamic viewport
    glm::dvec2 availableSize = Drawing::Math::toDVec2(ImGui::GetContentRegionAvail());
    glm::dvec2 viewportSize = glm::dvec2(availableSize.x, availableSize.y - 20); // Leave some margin
    
    // Get the currently selected Spring2D from the canvas
    const Drawing::Shape* selectedShape = canvas.getSelectedShape();
    if (selectedShape && selectedShape->type == Drawing::ShapeType::SPRING2D) {
      const Drawing::Spring2D* spring = static_cast<const Drawing::Spring2D*>(selectedShape);
      appContext.springViewer.render(spring, viewportSize);
    } else {
      ImGui::Text("No Spring2D selected. Please select a Spring2D to view in 3D.");
    }
  }
  ImGui::End();
}

void UI::Viewers3DUI::RenderShockAbsorber3DViewWindow(Drawing::Canvas &canvas, Core::ApplicationContext& appContext) {
  if (!appContext.shockAbsorberViewerInitialized) {
    appContext.shockAbsorberViewer.initialize();
    appContext.shockAbsorberViewerInitialized = true;
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
      appContext.shockAbsorberViewer.render(assembly.spring, assembly.topEnd, assembly.bottomEnd, viewportSize);
      RenderMeshControls(appContext.shockAbsorberViewer.getModel(), "Shock Absorber");
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

void UI::Viewers3DUI::RenderBellows3DViewWindow(Drawing::Canvas &canvas, Core::ApplicationContext& appContext) {
  if (!appContext.bellows3DViewInitialized) {
    appContext.bellowsViewer.initialize();
    appContext.bellows3DViewInitialized = true;
  }
  
  if (ImGui::Begin("Bellows 3D View", &appContext.showBellows3DView)) {
    // Get available content region size for dynamic viewport
    glm::dvec2 availableSize = Drawing::Math::toDVec2(ImGui::GetContentRegionAvail());
    glm::dvec2 viewportSize = glm::dvec2(availableSize.x, availableSize.y - 20); // Leave some margin
    
    // Get the currently selected Bellows from the canvas
    const Drawing::Shape* selectedShape = canvas.getSelectedShape();
    if (selectedShape && selectedShape->type == Drawing::ShapeType::BELLOWS) {
      const Drawing::Bellows* bellows = static_cast<const Drawing::Bellows*>(selectedShape);
      appContext.bellowsViewer.render(bellows, viewportSize);
      RenderMeshControls(appContext.bellowsViewer.getModel(), "Bellows");
    } else {
      ImGui::Text("No Bellows selected. Please select a Bellows to view in 3D.");
    }
  }
  ImGui::End();
}

void UI::Viewers3DUI::RenderBallBearing3DViewWindow(Drawing::Canvas &canvas, Core::ApplicationContext& appContext) {
  if (!appContext.ballBearing3DViewInitialized) {
    appContext.ballBearingViewer.initialize();
    appContext.ballBearing3DViewInitialized = true;
  }
  
  if (ImGui::Begin("Ball Bearing 3D View", &appContext.showBallBearing3DView)) {
    // Get available content region size for dynamic viewport
    glm::dvec2 availableSize = Drawing::Math::toDVec2(ImGui::GetContentRegionAvail());
    glm::dvec2 viewportSize = glm::dvec2(availableSize.x, availableSize.y - 20); // Leave some margin
    
    // Get the currently selected Ball Bearing from the canvas
    const Drawing::Shape* selectedShape = canvas.getSelectedShape();
    if (selectedShape && selectedShape->type == Drawing::ShapeType::BALL_BEARING) {
      const Drawing::BallBearing* ballBearing = static_cast<const Drawing::BallBearing*>(selectedShape);
      appContext.ballBearingViewer.render(ballBearing, viewportSize);
      RenderMeshControls(appContext.ballBearingViewer.getModel(), "Ball Bearing");
    } else {
      ImGui::Text("No Ball Bearing selected. Please select a Ball Bearing to view in 3D.");
    }
  }
  ImGui::End();
}

