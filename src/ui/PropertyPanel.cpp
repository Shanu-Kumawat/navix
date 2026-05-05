#include "ui/PropertyPanel.hpp"

#include "ui/UIHelpers.hpp"
#include "ui/UIColors.hpp"
#include "export/BellowsExporter.hpp"
#include <imgui.h>
#include <iostream>

void UI::PropertyPanel::Render(Drawing::Canvas &canvas, Core::ApplicationContext& appContext) {
  // Calculate property panel width - user can resize horizontally
  const float screenWidth = ImGui::GetIO().DisplaySize.x;
  const float propertyPanelMinWidth = 220.0f; // Minimum width
  const float propertyPanelMaxWidth = screenWidth * 0.4f; // Maximum 40% of screen
  const float ribbonHeight = 80.0f;
  const float statusBarHeight = 28.0f;
  
  // Clamp user width to valid range
  appContext.userPropertyPanelWidth = std::max(propertyPanelMinWidth, std::min(propertyPanelMaxWidth, appContext.userPropertyPanelWidth));
  
  ImGui::SetNextWindowPos(ImVec2(screenWidth - appContext.userPropertyPanelWidth, ribbonHeight), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(appContext.userPropertyPanelWidth, ImGui::GetIO().DisplaySize.y - ribbonHeight - statusBarHeight), ImGuiCond_Always);
  
  // Allow horizontal resize only (NoMove keeps vertical position fixed)
  ImGui::Begin("Properties", nullptr,
               ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
  
  // Update stored width if user resized the window
  ImVec2 currentSize = ImGui::GetWindowSize();
  if (currentSize.x != appContext.userPropertyPanelWidth) {
    appContext.userPropertyPanelWidth = currentSize.x;
  }

  // Get the selected shape
  const Drawing::Shape* selectedShape = canvas.getSelectedShape();

  // Show Shock Absorber Assembly Builder when a Spring2D is selected
  if (selectedShape && selectedShape->type == Drawing::ShapeType::SPRING2D) {
    auto* spring = static_cast<const Drawing::Spring2D*>(selectedShape);
    
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.4f, 0.6f, 1.0f));
    if (ImGui::CollapsingHeader("Shock Absorber Assembly", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::PopStyleColor();
      ImGui::Indent(10);
      
      // Show current assembly status
      auto assemblies = canvas.findShockAbsorberAssemblies();
      bool hasTopEnd = false;
      bool hasBottomEnd = false;
      const Drawing::ShockAbsorberEnd2D* foundTopEnd = nullptr;
      const Drawing::ShockAbsorberBottomEnd* foundBottomEnd = nullptr;
      
      // Check if this spring has associated ends
      for (const auto& assembly : assemblies) {
        if (assembly.spring == spring) {
          hasTopEnd = (assembly.topEnd != nullptr);
          hasBottomEnd = (assembly.bottomEnd != nullptr);
          if (assembly.topEnd) foundTopEnd = assembly.topEnd;
          if (assembly.bottomEnd) foundBottomEnd = assembly.bottomEnd;
          break;
        }
      }
      
      // Also check all shapes for ends associated with this spring
      for (const auto& shape : canvas.getShapes()) {
        if (shape->type == Drawing::ShapeType::SHOCK_ABSORBER_END_2D) {
          auto* end = static_cast<const Drawing::ShockAbsorberEnd2D*>(shape.get());
          if (end->parentSpring == spring) {
            hasTopEnd = true;
            if (!foundTopEnd) foundTopEnd = end;
          }
        } else if (shape->type == Drawing::ShapeType::SHOCK_ABSORBER_BOTTOM_END) {
          auto* bottomEnd = static_cast<const Drawing::ShockAbsorberBottomEnd*>(shape.get());
          if (bottomEnd->parentSpring == spring) {
            hasBottomEnd = true;
            if (!foundBottomEnd) foundBottomEnd = bottomEnd;
          }
        }
      }
      
      ImGui::Text("Assembly Status:");
      ImGui::Spacing();
      
      // Spring status (always present since we selected it)
      ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.0f, 1.0f), "  [OK] Spring");
      
      // Top End status
      if (hasTopEnd) {
        ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.0f, 1.0f), "  [OK] Top End (Piston Rod)");
      } else {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "  [ ] Top End (Piston Rod)");
      }
      
      // Bottom End status
      if (hasBottomEnd) {
        ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.0f, 1.0f), "  [OK] Bottom End (Mount)");
      } else {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "  [ ] Bottom End (Mount)");
      }
      
      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();
      
      // Add component buttons
      ImGui::Text("Add Components:");
      ImGui::Spacing();
      
      float buttonWidth = ImGui::GetContentRegionAvail().x - 10;
      
      // Add Top End button
      if (hasTopEnd) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
      }
      if (ImGui::Button("Add Top End (Piston Rod)", ImVec2(buttonWidth, 0)) && !hasTopEnd) {
        auto end = std::make_unique<Drawing::ShockAbsorberEnd2D>(spring);
        canvas.addShape(std::move(end));
        appContext.consoleMessage = "Added piston rod (top end)";
      }
      if (hasTopEnd) {
        ImGui::PopStyleColor(2);
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("Top end already added");
        }
      }
      
      // Add Bottom End button
      if (hasBottomEnd) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
      }
      if (ImGui::Button("Add Bottom End (Mount)", ImVec2(buttonWidth, 0)) && !hasBottomEnd) {
        auto bottomEnd = std::make_unique<Drawing::ShockAbsorberBottomEnd>(spring);
        canvas.addShape(std::move(bottomEnd));
        appContext.consoleMessage = "Added mounting plate (bottom end)";
      }
      if (hasBottomEnd) {
        ImGui::PopStyleColor(2);
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("Bottom end already added");
        }
      }
      
      // Create Complete Assembly button (adds both if missing)
      if (!hasTopEnd || !hasBottomEnd) {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 0.3f, 1.0f));
        if (ImGui::Button("Create Complete Assembly", ImVec2(buttonWidth, 30))) {
          if (!hasTopEnd) {
            auto end = std::make_unique<Drawing::ShockAbsorberEnd2D>(spring);
            canvas.addShape(std::move(end));
          }
          if (!hasBottomEnd) {
            auto bottomEnd = std::make_unique<Drawing::ShockAbsorberBottomEnd>(spring);
            canvas.addShape(std::move(bottomEnd));
          }
          appContext.consoleMessage = "Created complete shock absorber assembly";
        }
        ImGui::PopStyleColor(2);
      }
      
      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();
      
      // 3D View buttons
      ImGui::Text("3D Visualization:");
      ImGui::Spacing();
      
      if (ImGui::Button("View Spring in 3D", ImVec2(buttonWidth, 0))) {
        appContext.showSpring3DView = true;
      }
      
      bool hasCompleteAssembly = hasTopEnd && hasBottomEnd;
      if (!hasCompleteAssembly) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
      } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.6f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.5f, 0.7f, 1.0f));
      }
      
      if (ImGui::Button("View Shock Absorber in 3D", ImVec2(buttonWidth, 30)) && hasCompleteAssembly) {
        appContext.showShockAbsorber3DView = true;
        appContext.consoleMessage = "Opening 3D Shock Absorber View";
      }
      
      ImGui::PopStyleColor(2);
      if (!hasCompleteAssembly && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Add both top and bottom ends to enable 3D view");
      }
      

      // FEM Surface Meshing controls for Shock Absorber
      if (hasCompleteAssembly) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.15f, 0.3f, 0.15f, 0.7f));
        if (ImGui::CollapsingHeader("Surface Meshing##shockAbsorber", ImGuiTreeNodeFlags_DefaultOpen)) {
          if (!appContext.shockAbsorberViewerInitialized) {
            appContext.shockAbsorberViewer.initialize();
            appContext.shockAbsorberViewerInitialized = true;
          }

          ShockAbsorberModel3D* saModel = appContext.shockAbsorberViewer.getModel();
          if (saModel) {
            float saAvailWidth = ImGui::GetContentRegionAvail().x;
            ImGui::PushItemWidth(saAvailWidth);

            float elemSize = saModel->getFEMElementSize();
            if (ImGui::SliderFloat("Element Size##shockAbsorberMesh", &elemSize, 0.01f, 0.5f, "%.3f")) {
              saModel->setFEMElementSize(elemSize);
            }
            ImGui::PopItemWidth();

            float meshBtnWidth = saAvailWidth / 2 - 4;
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 0.3f, 1.0f));
            if (ImGui::Button("Generate Mesh##shockAbsorber", ImVec2(meshBtnWidth, 28))) {
              saModel->generateMesh(spring, foundTopEnd, foundBottomEnd);
              saModel->generateFEMMesh(saModel->getFEMElementSize());
            }
            ImGui::PopStyleColor(2);

            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.3f, 0.3f, 1.0f));
            if (ImGui::Button("Clear Mesh##shockAbsorber", ImVec2(meshBtnWidth, 28))) {
              saModel->clearFEMMesh();
            }
            ImGui::PopStyleColor(2);

            bool showMesh = saModel->getShowFEMMesh();
            if (ImGui::Checkbox("Show Surface Mesh##shockAbsorber", &showMesh)) {
              saModel->setShowFEMMesh(showMesh);
            }

            if (saModel->hasFEMMesh()) {
              const auto& mesh = saModel->getFEMMesh();
              ImGui::TextColored(ImVec4(0.5f, 0.9f, 0.5f, 1.0f),
                "Nodes: %zu  |  Elements: %zu", mesh.getNodes().size(), mesh.getElements().size());
            } else {
              ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No mesh generated");
            }
          }
        }
        ImGui::PopStyleColor();
      }

      ImGui::Unindent(10);
    } else {
      ImGui::PopStyleColor();
    }
    
    ImGui::Separator();
  }

  // Add some spacing before the scrollable content
  ImGui::Spacing();
  
  // Create a scrollable region for all the properties content
  // Use the remaining space after the Spring2D actions section
  float remainingHeight = ImGui::GetContentRegionAvail().y - 10.0f; // Leave some padding
  ImGui::BeginChild("PropertiesScrollRegion", ImVec2(0, remainingHeight), false, 
                    ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NavFlattened);

  ImGui::PushStyleColor(ImGuiCol_Header, UIColors::HEADER);
  
  // Tool-specific properties - use the available width effectively
  const float availableWidth = ImGui::GetContentRegionAvail().x;
  const float inputWidth = availableWidth * 0.65f;
  
  if (appContext.activeMode == Drawing::DrawingMode::Line) {
    if (ImGui::CollapsingHeader("Line Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Indent(10);
      
      // Toggle for fixed/dynamic line length
      bool fixedLength = appContext.fixedLineLength;
      if (ImGui::Checkbox("Fixed Length", &fixedLength)) {
        appContext.fixedLineLength = fixedLength;
        canvas.setFixedLineLength(fixedLength);
        appContext.consoleMessage = fixedLength ? "Line length: Fixed" : "Line length: Dynamic (use mouse to draw)";
      }
      
      if (appContext.fixedLineLength) {
        // Get current line length from canvas
        static float lineLength = canvas.getLineLength();
        
        // Input field for line length
        ImGui::Text("Length:");
        ImGui::SameLine(70);
        ImGui::PushItemWidth(inputWidth);
        if (ImGui::InputFloat("##LineLength", &lineLength, 1.0f, 10.0f, "%.1f")) {
          // Ensure length is positive
          if (lineLength < 0.1f) lineLength = 0.1f;
          // Update canvas with new line length
          canvas.setLineLength(lineLength);
        }
        ImGui::PopItemWidth();
      } else {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Click and drag to draw");
      }
      
      ImGui::Unindent(10);
    }
  }
  else if (appContext.activeMode == Drawing::DrawingMode::Circle) {
    if (ImGui::CollapsingHeader("Circle Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Indent(10);
      
      // Toggle for fixed/dynamic circle radius
      bool fixedRadius = appContext.fixedCircleRadius;
      if (ImGui::Checkbox("Fixed Radius", &fixedRadius)) {
        appContext.fixedCircleRadius = fixedRadius;
        canvas.setFixedCircleRadius(fixedRadius);
        appContext.consoleMessage = fixedRadius ? "Circle radius: Fixed" : "Circle radius: Dynamic (use mouse to draw)";
      }
      
      if (appContext.fixedCircleRadius) {
        // Get current circle radius from canvas
        static float circleRadius = canvas.getCircleRadius();
        
        // Input field for circle radius
        ImGui::Text("Radius:");
        ImGui::SameLine(70);
        ImGui::PushItemWidth(inputWidth);
        if (ImGui::InputFloat("##CircleRadius", &circleRadius, 1.0f, 10.0f, "%.1f")) {
          // Ensure radius is positive
          if (circleRadius < 0.1f) circleRadius = 0.1f;
          // Update canvas with new radius
          canvas.setCircleRadius(circleRadius);
        }
        ImGui::PopItemWidth();
      } else {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Click and drag to draw circle");
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Release to complete");
      }
      
      ImGui::Unindent(10);
    }
  }
  else if (appContext.activeMode == Drawing::DrawingMode::Square) {
    if (ImGui::CollapsingHeader("Square Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Indent(10);
      
      // Toggle for fixed/dynamic square size
      bool fixedSize = appContext.fixedSquareSize;
      if (ImGui::Checkbox("Fixed Size", &fixedSize)) {
        appContext.fixedSquareSize = fixedSize;
        canvas.setFixedSquareSize(fixedSize);
        appContext.consoleMessage = fixedSize ? "Square size: Fixed" : "Square size: Dynamic (use mouse to draw)";
      }
      
      if (appContext.fixedSquareSize) {
        // Get current square size from canvas
        static float squareSize = canvas.getSquareSize();
        
        // Input field for square size
        ImGui::Text("Side Length:");
        ImGui::SameLine(70);
        ImGui::PushItemWidth(inputWidth);
        if (ImGui::InputFloat("##SquareSize", &squareSize, 1.0f, 10.0f, "%.1f")) {
          // Ensure size is positive
          if (squareSize < 0.1f) squareSize = 0.1f;
          // Update canvas with new size
          canvas.setSquareSize(squareSize);
        }
        ImGui::PopItemWidth();
      } else {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Click and drag to draw square");
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Release to complete");
      }
      
      ImGui::Unindent(10);
    }
  }
  else if (appContext.activeMode == Drawing::DrawingMode::Rectangle) {
    if (ImGui::CollapsingHeader("Rectangle Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Indent(10);
      
      // Toggle for fixed/dynamic rectangle size
      bool fixedSize = appContext.fixedRectangleSize;
      if (ImGui::Checkbox("Fixed Size", &fixedSize)) {
        appContext.fixedRectangleSize = fixedSize;
        canvas.setFixedRectangleSize(fixedSize);
        appContext.consoleMessage = fixedSize ? "Rectangle size: Fixed" : "Rectangle size: Dynamic (use mouse to draw)";
      }
      
      if (appContext.fixedRectangleSize) {
        // Get current rectangle dimensions from canvas
        static float rectangleWidth = canvas.getRectangleWidth();
        static float rectangleHeight = canvas.getRectangleHeight();
        
        // Input field for rectangle width
        ImGui::Text("Width:");
        ImGui::SameLine(70);
        ImGui::PushItemWidth(inputWidth);
        if (ImGui::InputFloat("##RectWidth", &rectangleWidth, 1.0f, 10.0f, "%.1f")) {
          // Ensure width is positive
          if (rectangleWidth < 0.1f) rectangleWidth = 0.1f;
          // Update canvas with new width
          canvas.setRectangleWidth(rectangleWidth);
        }
        ImGui::PopItemWidth();
        
        // Input field for rectangle height
        ImGui::Text("Height:");
        ImGui::SameLine(70);
        ImGui::PushItemWidth(inputWidth);
        if (ImGui::InputFloat("##RectHeight", &rectangleHeight, 1.0f, 10.0f, "%.1f")) {
          // Ensure height is positive
          if (rectangleHeight < 0.1f) rectangleHeight = 0.1f;
          // Update canvas with new height
          canvas.setRectangleHeight(rectangleHeight);
        }
        ImGui::PopItemWidth();
      } else {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Click and drag to draw rectangle");
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Release to complete");
      }
      
      ImGui::Unindent(10);
    }
  }
  else if (appContext.activeMode == Drawing::DrawingMode::Triangle) {
    if (ImGui::CollapsingHeader("Triangle Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Indent(10);
      
      // Toggle for fixed/dynamic triangle size
      bool fixedSize = appContext.fixedTriangleSize;
      if (ImGui::Checkbox("Fixed Size", &fixedSize)) {
        appContext.fixedTriangleSize = fixedSize;
        canvas.setFixedTriangleSize(fixedSize);
        appContext.consoleMessage = fixedSize ? "Triangle size: Fixed" : "Triangle size: Dynamic (use mouse to draw)";
      }
      
      if (appContext.fixedTriangleSize) {
        // Get current triangle side length from canvas
        static float triangleSide = canvas.getTriangleSide();
        
        // Input field for triangle side length
        ImGui::Text("Side Length:");
        ImGui::SameLine(70);
        ImGui::PushItemWidth(inputWidth);
        if (ImGui::InputFloat("##TriangleSide", &triangleSide, 1.0f, 10.0f, "%.1f")) {
          // Ensure side length is positive
          if (triangleSide < 0.1f) triangleSide = 0.1f;
          // Update canvas with new side length
          canvas.setTriangleSide(triangleSide);
        }
        ImGui::PopItemWidth();
      } else {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Click and drag to draw triangle");
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Release to complete");
      }
      
      ImGui::Unindent(10);
    }
  }
  else if (appContext.activeMode == Drawing::DrawingMode::Bellows) {
    if (ImGui::CollapsingHeader("Bellows Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Indent(10);
      
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Click to set start point");
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Click again to set length");
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Adjust parameters in properties");
      ImGui::Separator();
      
      ImGui::Text("Select an existing bellows to configure its parameters");
      
      ImGui::Unindent(10);
    }
  }
  else if (appContext.activeMode == Drawing::DrawingMode::Spring2D) {
    if (ImGui::CollapsingHeader("Spring Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(10);

        // Use Canvas member variables for live update
        ImGui::Text("Outer Diameter:");
        ImGui::SameLine(120);
        ImGui::PushItemWidth(inputWidth);
        if (ImGui::InputFloat("##SpringOuterDiameter", &canvas.springOuterDiameter, 0.1f, 1.0f, "%.2f")) {
            if (canvas.springOuterDiameter < 1.0f) canvas.springOuterDiameter = 1.0f;
            if (selectedShape && selectedShape->type == Drawing::ShapeType::SPRING2D) {
                canvas.updateShockAbsorberEndsForSpring(static_cast<const Drawing::Spring2D*>(selectedShape));
            }
        }
        ImGui::PopItemWidth();

        ImGui::Text("Wire Diameter:");
        ImGui::SameLine(120);
        ImGui::PushItemWidth(inputWidth);
        if (ImGui::InputFloat("##SpringWireDiameter", &canvas.springWireDiameter, 0.1f, 1.0f, "%.2f")) {
            if (canvas.springWireDiameter < 0.1f) canvas.springWireDiameter = 0.1f;
            if (selectedShape && selectedShape->type == Drawing::ShapeType::SPRING2D) {
                canvas.updateShockAbsorberEndsForSpring(static_cast<const Drawing::Spring2D*>(selectedShape));
            }
        }
        ImGui::PopItemWidth();

        ImGui::Text("Free Length:");
        ImGui::SameLine(120);
        ImGui::PushItemWidth(inputWidth);
        if (ImGui::InputFloat("##SpringFreeLength", &canvas.springFreeLength, 0.1f, 1.0f, "%.2f")) {
            if (canvas.springFreeLength < 1.0f) canvas.springFreeLength = 1.0f;
            if (selectedShape && selectedShape->type == Drawing::ShapeType::SPRING2D) {
                canvas.updateShockAbsorberEndsForSpring(static_cast<const Drawing::Spring2D*>(selectedShape));
            }
        }
        ImGui::PopItemWidth();

        ImGui::Text("Number of Coils:");
        ImGui::SameLine(120);
        ImGui::PushItemWidth(inputWidth);
        if (ImGui::InputInt("##SpringNumCoils", &canvas.springNumCoils)) {
            if (canvas.springNumCoils < 1) canvas.springNumCoils = 1;
            if (selectedShape && selectedShape->type == Drawing::ShapeType::SPRING2D) {
                canvas.updateShockAbsorberEndsForSpring(static_cast<const Drawing::Spring2D*>(selectedShape));
            }
        }
        ImGui::PopItemWidth();

        ImGui::Unindent(10);
    }
  }
  else if (appContext.activeMode == Drawing::DrawingMode::BallBearing) {
    if (ImGui::CollapsingHeader("Ball Bearing Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Indent(10);
      
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Click to set center point");
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Click again to set size");
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Adjust parameters in properties");
      ImGui::Separator();
      
      ImGui::Text("Select an existing ball bearing to configure its parameters");
      
      ImGui::Unindent(10);
    }
  }

  
  // Selected object properties
  if (ImGui::CollapsingHeader("Object Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Indent(10);
    
    // Get selected shape
    Drawing::Shape *selectedShape = canvas.getSelectedShape();

    if (selectedShape) {
      // Add Bellows shape properties
      if (selectedShape->type == Drawing::ShapeType::BELLOWS) {
        Drawing::Bellows* bellows = static_cast<Drawing::Bellows*>(selectedShape);
        
        // Ensure the bellows is properly selected
        bellows->isSelected = true;
        
        ImGui::SeparatorText("Bellows Design Module");
        
        // Overall dimensions with validation
        float overallLength = bellows->calculateOverallLength();
        
        // Calculate available width for each control - use 55% to leave room for labels
        const float availWidth = ImGui::GetContentRegionAvail().x * 0.55f;
        ImGui::PushItemWidth(availWidth);
        
        if (ImGui::DragFloat("Overall Length (mm)", &overallLength, 1.0f, 20.0f, 1000.0f, "%.1f")) {
          bellows->updateFromOverallLength(overallLength);
        }
        ImGui::PopItemWidth();
        
        ImGui::Separator();
        
        // Cuff parameters section with validation feedback
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.2f, 0.3f, 0.7f));
        if (ImGui::CollapsingHeader("Cuff Dimensions", ImGuiTreeNodeFlags_DefaultOpen)) {
          ImGui::PushItemWidth(availWidth);
          
          bool validCuffDiameters = bellows->cuffAInnerDiameter <= bellows->baseConvolutionDiameter &&
                                   bellows->cuffBInnerDiameter <= bellows->baseConvolutionDiameter;
          
          if (!validCuffDiameters) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
            ImGui::TextWrapped("Warning: Cuff diameters should be less than Base Diameter");
            ImGui::PopStyleColor();
          }
          
          // Ensure min/max limits are enforced
          if (ImGui::DragFloat("Cuff A Inner Diam", &bellows->cuffAInnerDiameter, 0.5f, 5.0f, 200.0f, "%.1f")) {
            // Validate input - ensure it's positive
            if (bellows->cuffAInnerDiameter < 5.0f) bellows->cuffAInnerDiameter = 5.0f;
            bellows->invalidateCache(); // Invalidate cache when parameter changes
          }
          
          if (ImGui::DragFloat("Cuff B Inner Diam", &bellows->cuffBInnerDiameter, 0.5f, 5.0f, 200.0f, "%.1f")) {
            // Validate input - ensure it's positive
            if (bellows->cuffBInnerDiameter < 5.0f) bellows->cuffBInnerDiameter = 5.0f;
            bellows->invalidateCache(); // Invalidate cache when parameter changes
          }
          
          if (ImGui::DragFloat("Cuff A Length", &bellows->cuffALength, 0.5f, 5.0f, 100.0f, "%.1f")) {
            // Validate input - ensure it's positive
            if (bellows->cuffALength < 5.0f) bellows->cuffALength = 5.0f;
            
            // Update overall length
            overallLength = bellows->calculateOverallLength();
            bellows->invalidateCache(); // Invalidate cache when parameter changes
          }
          
          if (ImGui::DragFloat("Cuff B Length", &bellows->cuffBLength, 0.5f, 5.0f, 100.0f, "%.1f")) {
            // Validate input - ensure it's positive
            if (bellows->cuffBLength < 5.0f) bellows->cuffBLength = 5.0f;
            
            // Update overall length
            overallLength = bellows->calculateOverallLength();
            bellows->invalidateCache(); // Invalidate cache when parameter changes
          }
          
          ImGui::PopItemWidth();
        }
        ImGui::PopStyleColor();
        
        // Convolution parameters section
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.2f, 0.3f, 0.7f));
        if (ImGui::CollapsingHeader("Convolution Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
          ImGui::PushItemWidth(availWidth);
          
          bool validDiameters = bellows->baseConvolutionDiameter < bellows->peakConvolutionDiameter;
          if (!validDiameters) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
            ImGui::TextWrapped("Warning: Peak Diameter must be greater than Base Diameter");
            ImGui::PopStyleColor();
          }
          
          // Calculate minimum base diameter based on cuff diameters
          float minBaseDiameter = std::max(bellows->cuffAInnerDiameter, bellows->cuffBInnerDiameter);
          if (minBaseDiameter < 10.0f) minBaseDiameter = 10.0f;
          
          if (ImGui::DragFloat("Base Diameter", &bellows->baseConvolutionDiameter, 0.5f, 
                           minBaseDiameter, 300.0f, "%.1f")) {
            bellows->invalidateCache();
            // Ensure peak diameter is always greater than base
            if (bellows->peakConvolutionDiameter <= bellows->baseConvolutionDiameter) {
              bellows->peakConvolutionDiameter = bellows->baseConvolutionDiameter + 5.0f;
            }
          }
          
          if (ImGui::DragFloat("Peak Diameter", &bellows->peakConvolutionDiameter, 0.5f, 
                           bellows->baseConvolutionDiameter + 0.1f, 350.0f, "%.1f")) {
            bellows->invalidateCache();
            // Enforce the constraint directly in the UI
            if (bellows->peakConvolutionDiameter <= bellows->baseConvolutionDiameter) {
              bellows->peakConvolutionDiameter = bellows->baseConvolutionDiameter + 0.1f;
            }
          }
          
          // Prevent negative or zero convolution section length
          if (ImGui::DragFloat("Section Length", &bellows->convolutedSectionLength, 1.0f, 10.0f, 500.0f, "%.1f")) {
            bellows->invalidateCache();
            if (bellows->convolutedSectionLength < 10.0f) {
              bellows->convolutedSectionLength = 10.0f;
            }
            
            // Update overall length
            overallLength = bellows->calculateOverallLength();
          }
          
          // Enforce reasonable limits for number of convolutions
          if (ImGui::DragInt("Convolutions", &bellows->numConvolutions, 1, 1, 30)) {
            bellows->invalidateCache();
            // Keep numConvolutions at least 1
            bellows->numConvolutions = std::max(1, bellows->numConvolutions);
            
            // Limit based on convoluted section length - prevent too many convolutions
            float minSpacing = 5.0f; // Minimum spacing between convolutions
            int maxConvolutions = static_cast<int>(bellows->convolutedSectionLength / minSpacing);
            if (maxConvolutions < 1) maxConvolutions = 1;
            
            if (bellows->numConvolutions > maxConvolutions) {
              bellows->numConvolutions = maxConvolutions;
            }
          }
          
          // Calculate and display convolution spacing
          float convolutionSpacing = bellows->convolutedSectionLength / bellows->numConvolutions;
          ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Spacing: %.1f mm", convolutionSpacing);
          
          ImGui::PopItemWidth();
        }
        ImGui::PopStyleColor();
        
        // Wall thickness with validation
        ImGui::PushItemWidth(availWidth);
        
        // Calculate reasonable limits for wall thickness
        float maxWallThickness = std::min(bellows->baseConvolutionDiameter / 4.0f, 
                                       (bellows->peakConvolutionDiameter - bellows->baseConvolutionDiameter) / 2.0f);
        if (maxWallThickness < 0.5f) maxWallThickness = 0.5f;
        
        if (ImGui::DragFloat("Wall Thickness", &bellows->wallThickness, 0.1f, 0.5f, maxWallThickness, "%.1f")) {
          bellows->invalidateCache();
          // Ensure wall thickness stays within reasonable limits
          if (bellows->wallThickness < 0.5f) bellows->wallThickness = 0.5f;
          if (bellows->wallThickness > maxWallThickness) bellows->wallThickness = maxWallThickness;
        }
        
        bool validWallThickness = bellows->wallThickness <= bellows->baseConvolutionDiameter / 4.0f &&
                                 bellows->wallThickness <= (bellows->peakConvolutionDiameter - bellows->baseConvolutionDiameter) / 2.0f;
        
        if (!validWallThickness) {
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
          ImGui::TextWrapped("Warning: Wall thickness too large");
          ImGui::PopStyleColor();
        }
        
        ImGui::PopItemWidth();
        
        // Display options
        ImGui::Checkbox("Show Dimensions", &bellows->showDimensions);
        
        ImGui::Separator();
        
        // Action buttons - arrange in two rows if panel is narrow
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.6f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.5f, 0.7f, 1.0f));
        
        const float btnWidth = availWidth / 2 - 4;
        
        // Reset parameters button
        if (ImGui::Button("Reset Parameters", ImVec2(btnWidth, 30))) {
          bellows->resetParameters();
        }
        
        // Fit to view button
        if (ImGui::Button("Fit to View", ImVec2(btnWidth, 30))) {
          canvas.fitBellowsToView();
        }
        
        ImGui::PopStyleColor(2);
        
        // Unified 3D View button for Bellows
        ImGui::Separator();
        if (ComplexShape3DManager::render3DViewButton("3D Bellows View", "Open 3D visualization of the bellows")) {
          appContext.showBellows3DView = true;
        }
        
        // FEM Surface Meshing controls for Bellows
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.15f, 0.3f, 0.15f, 0.7f));
        if (ImGui::CollapsingHeader("Surface Meshing", ImGuiTreeNodeFlags_DefaultOpen)) {
          // Ensure the 3D viewer is initialized so we have a model to mesh
          if (!appContext.bellows3DViewInitialized) {
            appContext.bellowsViewer.initialize();
            appContext.bellows3DViewInitialized = true;
          }
          
          BellowsModel3D* model = appContext.bellowsViewer.getModel();
          if (model) {
            ImGui::PushItemWidth(availWidth);
            
            // Element size control
            float elemSize = model->getFEMElementSize();
            if (ImGui::SliderFloat("Element Size##bellowsMesh", &elemSize, 0.01f, 0.5f, "%.3f")) {
              model->setFEMElementSize(elemSize);
            }
            ImGui::PopItemWidth();
            
            // Generate / Clear buttons
            float meshBtnWidth = availWidth / 2 - 4;
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 0.3f, 1.0f));
            if (ImGui::Button("Generate Mesh##bellows", ImVec2(meshBtnWidth, 28))) {
              // Update the model geometry from the current bellows shape first
              model->generateMesh(bellows);
              model->generateFEMMesh(model->getFEMElementSize());
            }
            ImGui::PopStyleColor(2);
            
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.3f, 0.3f, 1.0f));
            if (ImGui::Button("Clear Mesh##bellows", ImVec2(meshBtnWidth, 28))) {
              model->clearFEMMesh();
            }
            ImGui::PopStyleColor(2);
            
            // Show/Hide toggle
            bool showMesh = model->getShowFEMMesh();
            if (ImGui::Checkbox("Show Surface Mesh##bellows", &showMesh)) {
              model->setShowFEMMesh(showMesh);
            }
            
            // Mesh statistics
            if (model->hasFEMMesh()) {
              const auto& mesh = model->getFEMMesh();
              ImGui::TextColored(ImVec4(0.5f, 0.9f, 0.5f, 1.0f), 
                "Nodes: %zu  |  Elements: %zu", mesh.getNodes().size(), mesh.getElements().size());

              // ── Export to ANSYS button ──
              ImGui::Spacing();
              ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.35f, 0.55f, 1.0f));
              ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.45f, 0.65f, 1.0f));
              ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.30f, 0.50f, 1.0f));
              float exportBtnWidth = ImGui::GetContentRegionAvail().x;
              if (ImGui::Button("Export to ANSYS (.inp)##bellows", ImVec2(exportBtnWidth, 30))) {
                // Export to project root
                std::string exportPath = "bellows_export.inp";
                
                const Core::FEM::FEMResult* femRes = model->hasFEMResult() ? &model->getFEMResult() : nullptr;
                
                auto result = Export::BellowsExporter::exportAbaqusINP(
                    model->getFEMMesh(),
                    *bellows,
                    model->getFEMMaterial(),
                    exportPath,
                    femRes
                );
                
                if (result.success) {
                  appContext.consoleMessage = "Exported to " + result.filePath + 
                    " (" + std::to_string(result.nodeCount) + " nodes, " + 
                    std::to_string(result.elementCount) + " elements)";
                  appContext.isConsoleMessageError = false;
                } else {
                  appContext.consoleMessage = "Export failed: " + result.errorMessage;
                  appContext.isConsoleMessageError = true;
                }
              }
              ImGui::PopStyleColor(3);
              if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Export FEM mesh as Abaqus Input Deck for ANSYS Mechanical import");
              }
            } else {
              ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No mesh generated");
            }
          }
        }
        ImGui::PopStyleColor();

        // ── FEM Structural Analysis section ──────────────────────────
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.15f, 0.35f, 0.7f));
        if (ImGui::CollapsingHeader("FEM Analysis##bellows", ImGuiTreeNodeFlags_DefaultOpen)) {
          if (!appContext.bellows3DViewInitialized) {
            appContext.bellowsViewer.initialize();
            appContext.bellows3DViewInitialized = true;
          }
          BellowsModel3D* model2 = appContext.bellowsViewer.getModel();
          if (model2) {
            // ── Material ──
            ImGui::PushItemWidth(availWidth);
            static int matChoice = 0;
            const char* matNames[] = { "Steel", "Stainless Steel", "Inconel 718" };
            if (ImGui::Combo("Material##fem", &matChoice, matNames, IM_ARRAYSIZE(matNames))) {
              Core::FEM::Material m;
              switch (matChoice) {
                default:
                case 0: m = Core::FEM::Material(0, "Steel",           200e9, 0.30, 7850, 250e6); break;
                case 1: m = Core::FEM::Material(1, "Stainless Steel", 193e9, 0.29, 8000, 215e6); break;
                case 2: m = Core::FEM::Material(2, "Inconel 718",     205e9, 0.29, 8190, 1034e6); break;
              }
              model2->setFEMMaterial(m);
            }
            ImGui::PopItemWidth();

            const auto& mat = model2->getFEMMaterial();
            ImGui::Text("  E = %.0f GPa, v = %.2f", mat.youngsModulus / 1e9, mat.poissonsRatio);
            ImGui::Text("  Wall: %.2f mm", bellows->wallThickness);

            // ── Load configuration ──
            ImGui::Spacing();
            static float pressureKPa = 100.0f;
            static float axialForceN = 0.0f;
            ImGui::PushItemWidth(availWidth);
            ImGui::DragFloat("Pressure (kPa)##fem", &pressureKPa, 1.0f, 0.0f, 10000.0f, "%.1f");
            ImGui::DragFloat("Axial Force (N)##fem", &axialForceN, 1.0f, -100000.0f, 100000.0f, "%.1f");
            ImGui::PopItemWidth();

            // ── Run / Clear buttons ──
            ImGui::Spacing();
            float femFullW = ImGui::GetContentRegionAvail().x;
            float femBtnW = femFullW / 2 - 4;

            bool hasMesh2 = model2->hasFEMMesh();
            if (!hasMesh2) {
              ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
              ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
            } else {
              ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.35f, 0.6f, 1.0f));
              ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.45f, 0.7f, 1.0f));
            }
            if (ImGui::Button("Run FEM##bel", ImVec2(femBtnW, 30)) && hasMesh2) {
              Core::FEM::AnalysisConfig cfg;
              cfg.pressure = static_cast<double>(pressureKPa) * 1e3;
              cfg.axialForce = static_cast<double>(axialForceN);
              model2->runFEMAnalysis(cfg);
            }
            ImGui::PopStyleColor(2);

            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.3f, 0.3f, 1.0f));
            if (ImGui::Button("Clear##fem", ImVec2(femBtnW, 30))) {
              model2->clearFEMResult();
            }
            ImGui::PopStyleColor(2);

            if (!hasMesh2) {
              ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.2f, 1.0f), "Generate mesh first");
            }

            // ── Results display ──
            if (model2->hasFEMResult()) {
              const auto& res = model2->getFEMResult();
              ImGui::Spacing();
              ImGui::SeparatorText("Results");
              ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f), "Status: %s", res.statusMessage.c_str());
              ImGui::Text("Max Disp:  %.4f mm", res.maxDisplacement * 1e3);
              double maxMPa = res.maxVonMises / 1e6;
              double yieldMPa = mat.yieldStrength / 1e6;
              if (maxMPa > yieldMPa) {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                  "Max Stress: %.1f MPa (> yield %.0f)", maxMPa, yieldMPa);
              } else {
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f),
                  "Max Stress: %.1f MPa", maxMPa);
              }
              ImGui::Text("Safety Factor: %.2f", yieldMPa / std::max(maxMPa, 1e-6));

              // Stress contour toggle
              bool showContours = model2->getShowStressContours();
              if (ImGui::Checkbox("Show Stress Contours##bel", &showContours)) {
                model2->setShowStressContours(showContours);
              }

              // Colour legend
              if (showContours) {
                ImGui::Text("%.1f MPa", res.minVonMises / 1e6);
                ImGui::SameLine();
                ImVec2 p = ImGui::GetCursorScreenPos();
                float legW = ImGui::GetContentRegionAvail().x - 60;
                ImDrawList* dl = ImGui::GetWindowDrawList();
                for (int i = 0; i < static_cast<int>(legW); ++i) {
                  float t = static_cast<float>(i) / legW;
                  ImU32 col;
                  if (t < 0.25f) { float s = t/0.25f; col = IM_COL32(0, (int)(s*255), 255, 255); }
                  else if (t < 0.5f) { float s = (t-0.25f)/0.25f; col = IM_COL32(0, 255, (int)((1-s)*255), 255); }
                  else if (t < 0.75f) { float s = (t-0.5f)/0.25f; col = IM_COL32((int)(s*255), 255, 0, 255); }
                  else { float s = (t-0.75f)/0.25f; col = IM_COL32(255, (int)((1-s)*255), 0, 255); }
                  dl->AddRectFilled(ImVec2(p.x + i, p.y), ImVec2(p.x + i + 1, p.y + 14), col);
                }
                ImGui::Dummy(ImVec2(legW, 14));
                ImGui::SameLine();
                ImGui::Text("%.1f", res.maxVonMises / 1e6);
              }

              // ── Deformed shape overlay ──
              ImGui::Spacing();
              bool showDef = model2->getShowDeformed();
              if (ImGui::Checkbox("Show Deformed Shape##bel", &showDef)) {
                model2->setShowDeformed(showDef);
                if (showDef) model2->setDeformScale(model2->getDeformScale());
              }
              if (showDef) {
                float dScale = model2->getDeformScale();
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::DragFloat("Deform Scale##fem", &dScale, 1.0f, 1.0f, 5000.0f, "%.0fx")) {
                  model2->setDeformScale(dScale);
                }
                ImGui::PopItemWidth();
              }

              // ── Reaction forces ──
              if (!res.reactionForces.empty()) {
                ImGui::Spacing();
                ImGui::SeparatorText("Reaction Forces");
                double rfx = 0, rfy = 0, rfz = 0;
                for (const auto& rf : res.reactionForces) {
                  rfx += rf.force.x(); rfy += rf.force.y(); rfz += rf.force.z();
                }
                ImGui::Text("Total |R| = %.2f N", res.totalReactionMagnitude);
                ImGui::Text("  Rx = %.2f N", rfx);
                ImGui::Text("  Ry = %.2f N", rfy);
                ImGui::Text("  Rz = %.2f N", rfz);
                ImGui::Text("  (%d support nodes)", static_cast<int>(res.reactionForces.size()));
              }
            }

            // ── Modal Analysis ──
            ImGui::Spacing();
            ImGui::SeparatorText("Modal Analysis");
            static int numModes = 6;
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
            ImGui::DragInt("Num Modes##fem", &numModes, 0.1f, 1, 20);
            ImGui::PopItemWidth();

            bool canModal = model2->hasFEMMesh();
            if (!canModal) ImGui::BeginDisabled();
            if (ImGui::Button("Run Modal Analysis##bel", ImVec2(femFullW, 28))) {
              model2->runModalAnalysis(numModes);
            }
            if (!canModal) ImGui::EndDisabled();

            if (model2->hasModalResult()) {
              const auto& mr = model2->getModalResult();
              ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f), "%s", mr.statusMessage.c_str());
              if (!mr.modes.empty()) {
                int activeM = model2->getActiveMode();
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
                if (ImGui::SliderInt("View Mode##fem", &activeM, 0, static_cast<int>(mr.modes.size()) - 1)) {
                  model2->setActiveMode(activeM);
                }
                ImGui::PopItemWidth();
                if (ImGui::BeginTable("##modes", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit)) {
                  ImGui::TableSetupColumn("Mode", ImGuiTableColumnFlags_WidthFixed, 50);
                  ImGui::TableSetupColumn("Frequency (Hz)", ImGuiTableColumnFlags_WidthStretch);
                  ImGui::TableHeadersRow();
                  for (size_t i = 0; i < mr.modes.size(); ++i) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    if (static_cast<int>(i) == model2->getActiveMode())
                      ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, 1.0f), "%d", static_cast<int>(i + 1));
                    else
                      ImGui::Text("%d", static_cast<int>(i + 1));
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.2f", mr.modes[i].frequency);
                  }
                  ImGui::EndTable();
                }
              }
            }

            // ── Mesh Convergence Study ──
            ImGui::Spacing();
            ImGui::SeparatorText("Convergence Study");
            static int convPoints = 5;
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
            ImGui::DragInt("Refinements##conv", &convPoints, 0.1f, 3, 6);
            ImGui::PopItemWidth();

            bool canConv = model2->hasFEMMesh();
            if (!canConv) ImGui::BeginDisabled();
            if (ImGui::Button("Run Convergence##bel", ImVec2(femFullW, 28))) {
              Core::FEM::AnalysisConfig cfg;
              cfg.pressure = static_cast<double>(pressureKPa) * 1e3;
              cfg.axialForce = static_cast<double>(axialForceN);
              model2->runConvergenceStudy(cfg, convPoints);
            }
            if (!canConv) ImGui::EndDisabled();

            if (model2->hasConvergenceData()) {
              const auto& cd = model2->getConvergenceData();
              if (ImGui::BeginTable("##conv", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit)) {
                ImGui::TableSetupColumn("h", ImGuiTableColumnFlags_WidthFixed, 40);
                ImGui::TableSetupColumn("Nodes", ImGuiTableColumnFlags_WidthFixed, 50);
                ImGui::TableSetupColumn("s MPa", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("u mm", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();
                for (const auto& pt : cd) {
                  ImGui::TableNextRow();
                  ImGui::TableSetColumnIndex(0); ImGui::Text("%.3f", pt.elementSize);
                  ImGui::TableSetColumnIndex(1); ImGui::Text("%d", pt.numNodes);
                  ImGui::TableSetColumnIndex(2); ImGui::Text("%.1f", pt.maxStress);
                  ImGui::TableSetColumnIndex(3); ImGui::Text("%.4f", pt.maxDisplacement);
                }
                ImGui::EndTable();
              }
            }
          }
        }
        ImGui::PopStyleColor();

      }
      // Add Ball Bearing shape properties
      else if (selectedShape->type == Drawing::ShapeType::BALL_BEARING) {
        Drawing::BallBearing* ballBearing = static_cast<Drawing::BallBearing*>(selectedShape);
        
        // Ensure the ball bearing is properly selected
        ballBearing->isSelected = true;
        
        ImGui::SeparatorText("Ball Bearing Design Module");
        
        // Calculate available width for each control - use 55% to leave room for labels
        const float availWidth = ImGui::GetContentRegionAvail().x * 0.55f;
        ImGui::PushItemWidth(availWidth);
        
        // Overall dimensions with validation
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.2f, 0.3f, 0.7f));
        if (ImGui::CollapsingHeader("Main Dimensions", ImGuiTreeNodeFlags_DefaultOpen)) {
          ImGui::PushItemWidth(availWidth * 0.95f);
          
          bool validDiameters = ballBearing->innerDiameter < ballBearing->outerDiameter &&
                               ballBearing->ballDiameter < (ballBearing->outerDiameter - ballBearing->innerDiameter) / 2.0f;
          
          if (!validDiameters) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
            ImGui::TextWrapped("Warning: Check diameter relationships");
            ImGui::PopStyleColor();
          }
          
          if (ImGui::DragFloat("Outer Diameter (mm)", &ballBearing->outerDiameter, 1.0f, 10.0f, 500.0f, "%.1f")) {
            if (ballBearing->outerDiameter <= ballBearing->innerDiameter) {
              ballBearing->outerDiameter = ballBearing->innerDiameter + 10.0f;
            }
          }
          
          if (ImGui::DragFloat("Inner Diameter (mm)", &ballBearing->innerDiameter, 1.0f, 5.0f, ballBearing->outerDiameter - 5.0f, "%.1f")) {
            if (ballBearing->innerDiameter >= ballBearing->outerDiameter) {
              ballBearing->innerDiameter = ballBearing->outerDiameter - 5.0f;
            }
          }
          
          ImGui::DragFloat("Width (mm)", &ballBearing->width, 0.5f, 1.0f, 100.0f, "%.1f");
          
          ImGui::PopItemWidth();
        }
        ImGui::PopStyleColor();
        
        // Ball parameters
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.3f, 0.2f, 0.7f));
        if (ImGui::CollapsingHeader("Ball Configuration", ImGuiTreeNodeFlags_DefaultOpen)) {
          ImGui::PushItemWidth(availWidth * 0.95f);
          
          float maxBallDiameter = (ballBearing->outerDiameter - ballBearing->innerDiameter) / 2.5f;
          if (ImGui::DragFloat("Ball Diameter (mm)", &ballBearing->ballDiameter, 0.5f, 1.0f, maxBallDiameter, "%.1f")) {
            if (ballBearing->ballDiameter > maxBallDiameter) {
              ballBearing->ballDiameter = maxBallDiameter;
            }
          }
          
          int numBalls = ballBearing->numBalls;
          if (ImGui::InputInt("Number of Balls", &numBalls, 1, 2)) {
            ballBearing->numBalls = std::max(3, std::min(50, numBalls));
          }
          
          ImGui::DragFloat("Race Radius (mm)", &ballBearing->raceRadius, 0.1f, 0.5f, 10.0f, "%.1f");
          ImGui::DragFloat("Contact Angle (°)", &ballBearing->contactAngle, 1.0f, 0.0f, 45.0f, "%.1f");
          
          ImGui::PopItemWidth();
        }
        ImGui::PopStyleColor();
        
        // Display options
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.1f, 0.3f, 0.7f));
        if (ImGui::CollapsingHeader("Display Options", ImGuiTreeNodeFlags_DefaultOpen)) {
          ImGui::Checkbox("Show Balls", &ballBearing->showBalls);
          ImGui::Checkbox("Show Cage", &ballBearing->showCage);
          ImGui::Checkbox("Show Dimensions", &ballBearing->showDimensions);
        }
        ImGui::PopStyleColor();
        
        ImGui::PopItemWidth();
        
        ImGui::Separator();
        
        // Action buttons
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.6f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.5f, 0.7f, 1.0f));
        
        const float btnWidth = availWidth / 2 - 4;
        
        // Reset parameters button
        if (ImGui::Button("Reset Parameters", ImVec2(btnWidth, 30))) {
          ballBearing->resetParameters();
        }
        
        ImGui::SameLine();
        
        // Fit to view button
        if (ImGui::Button("Fit to View", ImVec2(btnWidth, 30))) {
          // TODO: Implement fitBallBearingToView similar to fitBellowsToView
        }
        
        ImGui::PopStyleColor(2);
        
        // Unified 3D View button for Ball Bearing
        ImGui::Separator();
        if (ComplexShape3DManager::render3DViewButton("3D Ball Bearing View", "Open 3D visualization of the ball bearing")) {
          appContext.showBallBearing3DView = true;
        }

        // FEM Surface Meshing controls for Ball Bearing
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.15f, 0.3f, 0.15f, 0.7f));
        if (ImGui::CollapsingHeader("Surface Meshing##ballBearing", ImGuiTreeNodeFlags_DefaultOpen)) {
          if (!appContext.ballBearing3DViewInitialized) {
            appContext.ballBearingViewer.initialize();
            appContext.ballBearing3DViewInitialized = true;
          }

          BallBearingModel3D* bbModel = appContext.ballBearingViewer.getModel();
          if (bbModel) {
            ImGui::PushItemWidth(availWidth);

            float elemSize = bbModel->getFEMElementSize();
            if (ImGui::SliderFloat("Element Size##ballBearingMesh", &elemSize, 0.01f, 0.5f, "%.3f")) {
              bbModel->setFEMElementSize(elemSize);
            }
            ImGui::PopItemWidth();

            float meshBtnWidth = availWidth / 2 - 4;
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 0.3f, 1.0f));
            if (ImGui::Button("Generate Mesh##ballBearing", ImVec2(meshBtnWidth, 28))) {
              bbModel->generateMesh(ballBearing);
              bbModel->generateFEMMesh(bbModel->getFEMElementSize());
            }
            ImGui::PopStyleColor(2);

            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.3f, 0.3f, 1.0f));
            if (ImGui::Button("Clear Mesh##ballBearing", ImVec2(meshBtnWidth, 28))) {
              bbModel->clearFEMMesh();
            }
            ImGui::PopStyleColor(2);

            bool showMesh = bbModel->getShowFEMMesh();
            if (ImGui::Checkbox("Show Surface Mesh##ballBearing", &showMesh)) {
              bbModel->setShowFEMMesh(showMesh);
            }

            if (bbModel->hasFEMMesh()) {
              const auto& mesh = bbModel->getFEMMesh();
              ImGui::TextColored(ImVec4(0.5f, 0.9f, 0.5f, 1.0f),
                "Nodes: %zu  |  Elements: %zu", mesh.getNodes().size(), mesh.getElements().size());
            } else {
              ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No mesh generated");
            }
          }
        }
        ImGui::PopStyleColor();
      }
    }
    
    ImGui::Unindent(10);
  }
  
  ImGui::PopStyleColor(); // Header color
  
  ImGui::EndChild();
  
  ImGui::End();
}
