#include "ui/PropertyPanel.hpp"

#include "ui/UIHelpers.hpp"
#include "ui/UIColors.hpp"
#include <imgui.h>
#include <iostream>

void UI::PropertyPanel::Render(Drawing::Canvas &canvas, Core::ApplicationContext& appContext) {
  // Calculate property panel width - user can resize horizontally
  const float screenWidth = ImGui::GetIO().DisplaySize.x;
  const float propertyPanelMinWidth = 220.0f; // Minimum width
  const float propertyPanelMaxWidth = screenWidth * 0.4f; // Maximum 40% of screen
  const float ribbonHeight = 145.0f; // Match ribbon height
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
      
      // Check if this spring has associated ends
      for (const auto& assembly : assemblies) {
        if (assembly.spring == spring) {
          hasTopEnd = (assembly.topEnd != nullptr);
          hasBottomEnd = (assembly.bottomEnd != nullptr);
          break;
        }
      }
      
      // Also check all shapes for ends associated with this spring
      for (const auto& shape : canvas.getShapes()) {
        if (shape->type == Drawing::ShapeType::SHOCK_ABSORBER_END_2D) {
          auto* end = static_cast<const Drawing::ShockAbsorberEnd2D*>(shape.get());
          if (end->parentSpring == spring) {
            hasTopEnd = true;
          }
        } else if (shape->type == Drawing::ShapeType::SHOCK_ABSORBER_BOTTOM_END) {
          auto* bottomEnd = static_cast<const Drawing::ShockAbsorberBottomEnd*>(shape.get());
          if (bottomEnd->parentSpring == spring) {
            hasBottomEnd = true;
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
            } else {
              ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No mesh generated");
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
      }
    }
    
    ImGui::Unindent(10);
  }
  
  ImGui::PopStyleColor(); // Header color
  
  // Bellows section
  if (selectedShape && selectedShape->type == Drawing::ShapeType::BELLOWS) {
    ImGui::Separator();
    ImGui::Text("Bellows Properties:");
    ImGui::PushStyleColor(ImGuiCol_Header, UIColors::HEADER);
    
    // Cast to Bellows - we need to remove const and then cast
    const Drawing::Bellows* const_bellows = static_cast<const Drawing::Bellows*>(selectedShape);
    Drawing::Bellows* bellows = const_cast<Drawing::Bellows*>(const_bellows);
    
    if (bellows) {
      ImGui::Indent(15); // Changed from 10 to 15 to move the content a bit to the left
      
      // Position and angle
      float position[2] = {bellows->position.x, bellows->position.y};
      if (ImGui::DragFloat2("Position", position, 1.0f)) {
        bellows->position.x = position[0];
        bellows->position.y = position[1];
      }
      
      float angle = bellows->angle * 180.0f / M_PI; // Convert to degrees for display
      if (ImGui::SliderFloat("Rotation", &angle, 0.0f, 360.0f, "%.1f°")) {
        bellows->angle = angle * M_PI / 180.0f; // Convert back to radians
      }
      
      ImGui::Separator();
      
      // Dimension parameters
      if (ImGui::CollapsingHeader("Dimensions", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(10);
        
        // Overall length calculation
        float overallLength = bellows->calculateOverallLength();
        ImGui::Text("Overall Length: %.1f mm", overallLength);
        
        // Show dimensions toggle
        ImGui::Checkbox("Show Dimensions", &bellows->showDimensions);
        
        ImGui::Separator();
        
        // Cuffs
        ImGui::Text("Cuffs:");
        if (ImGui::DragFloat("Cuff A Inner Diameter", &bellows->cuffAInnerDiameter, 0.5f, 10.0f, 100.0f, "%.1f mm")) {
            bellows->invalidateCache();
        }
        if (ImGui::DragFloat("Cuff B Inner Diameter", &bellows->cuffBInnerDiameter, 0.5f, 10.0f, 100.0f, "%.1f mm")) {
            bellows->invalidateCache();
        }
        if (ImGui::DragFloat("Cuff A Length", &bellows->cuffALength, 0.5f, 5.0f, 50.0f, "%.1f mm")) {
            bellows->invalidateCache();
        }
        if (ImGui::DragFloat("Cuff B Length", &bellows->cuffBLength, 0.5f, 5.0f, 50.0f, "%.1f mm")) {
            bellows->invalidateCache();
        }
        
        ImGui::Separator();
        
        // Convolutions
        ImGui::Text("Convolutions:");
        if (ImGui::DragFloat("Base Diameter", &bellows->baseConvolutionDiameter, 0.5f, bellows->cuffAInnerDiameter, bellows->peakConvolutionDiameter, "%.1f mm")) {
            bellows->invalidateCache();
        }
        if (ImGui::DragFloat("Peak Diameter", &bellows->peakConvolutionDiameter, 0.5f, bellows->baseConvolutionDiameter, 150.0f, "%.1f mm")) {
            bellows->invalidateCache();
        }
        if (ImGui::DragFloat("Convoluted Section Length", &bellows->convolutedSectionLength, 1.0f, 10.0f, 300.0f, "%.1f mm")) {
            bellows->invalidateCache();
        }
        
        int numConvs = bellows->numConvolutions;
        if (ImGui::InputInt("Number of Convolutions", &numConvs, 1, 2)) {
          bellows->numConvolutions = std::max(1, numConvs);
          bellows->invalidateCache();
        }
        
        if (ImGui::DragFloat("Wall Thickness", &bellows->wallThickness, 0.1f, 0.5f, 5.0f, "%.1f mm")) {
            bellows->invalidateCache();
        }
        
        ImGui::Unindent(10);
      }
      
      // ... rest of the bellows properties section ...
    }
    
    // Ensure we pop the style color for header
    ImGui::PopStyleColor();
  }
  
  // End the scrollable child region
  ImGui::EndChild();
  
  ImGui::End();

  // Always show Add ShockAbsorberEnd2D and 3D View buttons for Spring2D selection
  if (selectedShape && selectedShape->type == Drawing::ShapeType::SPRING2D) {
    ImGui::Separator();
    ImGui::Text("Spring2D Actions:");
    if (ImGui::Button("Add ShockAbsorberEnd2D")) {
      auto* spring = static_cast<const Drawing::Spring2D*>(selectedShape);
      auto end = std::make_unique<Drawing::ShockAbsorberEnd2D>(spring);
      canvas.addShape(std::move(end));
    }
    if (ImGui::Button("Add ShockAbsorberBottomEnd")) {
      auto* spring = static_cast<const Drawing::Spring2D*>(selectedShape);
      auto bottomEnd = std::make_unique<Drawing::ShockAbsorberBottomEnd>(spring);
      canvas.addShape(std::move(bottomEnd));
    }
    if (ImGui::Button("3D View")) {
      appContext.showSpring3DView = true;
    }
    // Add the new 3D Shock Absorber button (only enabled if there's a complete assembly)
    bool hasCompleteAssembly = canvas.hasCompleteShockAbsorberAssembly();
    if (!hasCompleteAssembly) {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));  // Gray out the button
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));     // Gray out text
    }
    
    if (ImGui::Button("3D Shock Absorber") && hasCompleteAssembly) {
      appContext.showShockAbsorber3DView = true;
    }
    
    if (!hasCompleteAssembly) {
      ImGui::PopStyleColor(2);  // Pop both colors
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Create a complete shock absorber assembly (spring + top end + bottom end) to enable 3D view");
      }
    }
    
    // Unified 3D Shock Absorber button using the new manager
    ImGui::Separator();
    if (ComplexShape3DManager::render3DViewButton("3D Shock Absorber View (Unified)", "Open unified 3D visualization of the complete shock absorber assembly")) {
      appContext.showShockAbsorber3DViewUnified = true;
    }
  }
}
