#include "ui/StatusBar.hpp"
#include "ui/UIHelpers.hpp"
#include "ui/UIColors.hpp"
#include <imgui.h>
#include <iostream>

void UI::StatusBar::Render(Drawing::Canvas &canvas, Core::ApplicationContext& appContext) {
  // AutoCAD-style status bar at the bottom (without command line)
  const float statusBarHeight = 28.0f;
  
  // Get screen width for better scaling
  const float screenWidth = ImGui::GetIO().DisplaySize.x;
  
  // Status bar - full width
  ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetIO().DisplaySize.y - statusBarHeight));
  ImGui::SetNextWindowSize(ImVec2(screenWidth, statusBarHeight));
  ImGui::PushStyleColor(ImGuiCol_WindowBg, UIColors::DARK_PANEL);
  ImGui::Begin("StatusBar", nullptr,
               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

  // Status sections with separators - calculate based on available width
  ImVec2 mousePos = ImGui::GetMousePos();
  float windowWidth = ImGui::GetWindowWidth();
  
  // Calculate section widths proportionally based on window width
  const float sectionPadding = 10.0f;
  
  // Calculate widths that better accommodate text (min 150px for coord section)
  float coordSectionWidth = std::max(150.0f, windowWidth * 0.20f);
  float statusSectionWidth = std::max(150.0f, windowWidth * 0.45f);
  float zoomSectionWidth = std::max(100.0f, windowWidth * 0.15f);
  float gridSectionWidth = std::max(100.0f, windowWidth * 0.15f);
  
  // Units-aware coordinate display
  std::string unitSuffix;
  switch (appContext.units) {
    case Drawing::UnitSystem::Pixels:
      unitSuffix = "px";
      break;
    case Drawing::UnitSystem::Millimeters:
      unitSuffix = "mm";
      break;
    case Drawing::UnitSystem::Centimeters:
      unitSuffix = "cm";
      break;
    case Drawing::UnitSystem::Inches:
      unitSuffix = "in";
      break;
  }
  
  float scaledX = mousePos.x / appContext.unitScale;
  float scaledY = mousePos.y / appContext.unitScale;
  
  // Section 1: Coordinates
  ImGui::Text("X: %.1f%s  Y: %.1f%s", scaledX, unitSuffix.c_str(), scaledY, unitSuffix.c_str());
  
  ImGui::SameLine(coordSectionWidth);
  ImGui::Text("|");
  ImGui::SameLine(coordSectionWidth + sectionPadding);
  
  // Section 2: Status message - clipped if too long
  // Calculate how much space we have for the status message
  float statusMsgWidth = statusSectionWidth - sectionPadding*2;
  std::string statusMsg = appContext.consoleMessage;
  ImVec2 textSize = ImGui::CalcTextSize(statusMsg.c_str());
  
  // If the text is too long, add ellipsis
  if (textSize.x > statusMsgWidth) {
    // Find index to truncate
    float ellipsisWidth = ImGui::CalcTextSize("...").x;
    int maxChars = 0;
    float accWidth = 0;
    for (int i = 0; i < statusMsg.size(); i++) {
      char tmp[2] = {statusMsg[i], '\0'};
      float charWidth = ImGui::CalcTextSize(tmp).x;
      if (accWidth + charWidth + ellipsisWidth > statusMsgWidth)
        break;
      accWidth += charWidth;
      maxChars = i + 1;
    }
    statusMsg = statusMsg.substr(0, maxChars) + "...";
  }
  
  ImGui::Text("%s", statusMsg.c_str());
  
  ImGui::SameLine(coordSectionWidth + statusSectionWidth);
  ImGui::Text("|");
  ImGui::SameLine(coordSectionWidth + statusSectionWidth + sectionPadding);
  
  // Section 3: Zoom level
  ImGui::Text("Zoom: %.0f%%", appContext.zoomLevel * 100.0f);
  
  ImGui::SameLine(coordSectionWidth + statusSectionWidth + zoomSectionWidth);
  ImGui::Text("|");
  ImGui::SameLine(coordSectionWidth + statusSectionWidth + zoomSectionWidth + sectionPadding);
  
  // Section 4: Grid size indicator
  ImGui::Text("Grid: %d%s", (int)appContext.gridSize, unitSuffix.c_str());

  ImGui::End();
  ImGui::PopStyleColor();
}
