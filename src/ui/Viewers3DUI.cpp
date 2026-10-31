#include "ui/Viewers3DUI.hpp"
#include "Base3DModel.hpp"
#include "ui/ExportUI.hpp"
#include "modeling3d/Exporter3D.hpp"
#include "export/ModelGeometryBuilder.hpp"
#include "export/FEMExporter.hpp"
#include "export/BellowsExporter.hpp"
#include <imgui.h>
#include <iostream>

namespace {

// Shared material for models that don't expose one (INP export)
Core::FEM::Material defaultExportMaterial() {
    return Core::FEM::Material(0, "Steel", 200e9, 0.30, 7850, 250e6);
}

} // namespace

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

      // ── Geometry export (exact CAD via OCCT + mesh/FEA formats) ──
      auto* model = appContext.shockAbsorberViewer.getModel();
      if (model && model->getCurrentSpring()) {
        static std::string exportMsg;
        auto* spring = model->getCurrentSpring();
        UI::renderExportSection(
            "shockabsorber3d", "shock_absorber",
            /*STL */ [&](const std::string& p) {
                return Modeling3D::Exporter3D::exportSTL(
                    model->getDisplayVertices(), model->getDisplayIndices(), p, "ShockAbsorber");
            },
            /*OBJ */ [&](const std::string& p) {
                return Modeling3D::Exporter3D::exportOBJ(
                    model->getDisplayVertices(), model->getDisplayIndices(), p, "ShockAbsorber");
            },
#ifdef USE_OCCT
            /*STEP*/ [&](const std::string& p) {
                return Modeling3D::Exporter3D::exportSTEPShape(
                    Export::ModelGeometryBuilder::buildShockAbsorber(
                        model->getCurrentSpring(), model->getCurrentEnd(), model->getCurrentBottomEnd()), p);
            },
            /*IGES*/ [&](const std::string& p) {
                return Modeling3D::Exporter3D::exportIGESShape(
                    Export::ModelGeometryBuilder::buildShockAbsorber(
                        model->getCurrentSpring(), model->getCurrentEnd(), model->getCurrentBottomEnd()), p);
            },
#else
            nullptr, nullptr,
#endif
            /*INP */ [&](const std::string& p) {
                return Export::FEMExporter::exportAbaqusINP(
                    model->getFEMMesh(), "SHOCK_ABSORBER", defaultExportMaterial(), p,
                    -1.0f /*solid surfaces — no shell section*/, 100.0).success;
            },
            exportMsg);
      }
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

      // ── Geometry export ──
      auto* model = appContext.bellowsViewer.getModel();
      if (model && model->getCurrentBellows()) {
        static std::string exportMsg;
        const Drawing::Bellows* cur = model->getCurrentBellows();
        UI::renderExportSection(
            "bellows3d", "bellows",
            /*STL */ [&](const std::string& p) {
                return Modeling3D::Exporter3D::exportSTL(
                    model->getDisplayVertices(), model->getDisplayIndices(), p, "Bellows");
            },
            /*OBJ */ [&](const std::string& p) {
                return Modeling3D::Exporter3D::exportOBJ(
                    model->getDisplayVertices(), model->getDisplayIndices(), p, "Bellows");
            },
#ifdef USE_OCCT
            /*STEP*/ [&](const std::string& p) {
                return Modeling3D::Exporter3D::exportSTEPShape(
                    Export::ModelGeometryBuilder::buildBellows(*cur), p);
            },
            /*IGES*/ [&](const std::string& p) {
                return Modeling3D::Exporter3D::exportIGESShape(
                    Export::ModelGeometryBuilder::buildBellows(*cur), p);
            },
#else
            nullptr, nullptr,
#endif
            /*INP */ [&](const std::string& p) {
                // Full bellows deck (BCs, shell section) from the dedicated exporter
                return Export::BellowsExporter::exportAbaqusINP(
                    model->getFEMMesh(), *cur, model->getFEMMaterial(), p).success;
            },
            exportMsg);
      }
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

      // ── Geometry export ──
      auto* model = appContext.ballBearingViewer.getModel();
      if (model && model->getCurrentBallBearing()) {
        static std::string exportMsg;
        const Drawing::BallBearing* cur = model->getCurrentBallBearing();
        UI::renderExportSection(
            "ballbearing3d", "ball_bearing",
            /*STL */ [&](const std::string& p) {
                return Modeling3D::Exporter3D::exportSTL(
                    model->getDisplayVertices(), model->getDisplayIndices(), p, "BallBearing");
            },
            /*OBJ */ [&](const std::string& p) {
                return Modeling3D::Exporter3D::exportOBJ(
                    model->getDisplayVertices(), model->getDisplayIndices(), p, "BallBearing");
            },
#ifdef USE_OCCT
            /*STEP*/ [&](const std::string& p) {
                return Modeling3D::Exporter3D::exportSTEPShape(
                    Export::ModelGeometryBuilder::buildBallBearing(*cur), p);
            },
            /*IGES*/ [&](const std::string& p) {
                return Modeling3D::Exporter3D::exportIGESShape(
                    Export::ModelGeometryBuilder::buildBallBearing(*cur), p);
            },
#else
            nullptr, nullptr,
#endif
            /*INP */ [&](const std::string& p) {
                // Bearing FEM mesh is normalized by outer radius; scale back to mm
                double sceneToMM = cur->outerDiameter / 2.0;
                return Export::FEMExporter::exportAbaqusINP(
                    model->getFEMMesh(), "BALL_BEARING", defaultExportMaterial(), p,
                    -1.0f, sceneToMM).success;
            },
            exportMsg);
      }
    } else {
      ImGui::Text("No Ball Bearing selected. Please select a Ball Bearing to view in 3D.");
    }
  }
  ImGui::End();
}
