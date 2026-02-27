#include "Canvas.hpp"
#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>
#include <iostream>
#include <cstdlib>  // For getenv, atof
#include <algorithm> // For std::clamp
#include <unordered_map>
#include <string>
#ifdef HAVE_SDL2_IMAGE
#include <SDL2/SDL_image.h>  // For PNG loading
#endif
#include "BellowsViewer3D.hpp"
#include "BallBearingViewer3D.hpp"
#include "SpringViewer3D.hpp"
#include "shapes/ShockAbsorberBottomEnd.hpp"
#include "ShockAbsorberViewer3D.hpp"
#include "ComplexShape3DManager.hpp"
#include "ApplicationContext.hpp"

// Icon texture storage
static std::unordered_map<std::string, GLuint> iconTextures;
static std::unordered_map<std::string, ImVec2> iconSizes;

// Fallback text icons if texture loading fails
static const char* FALLBACK_LINE = "-";
static const char* FALLBACK_CIRCLE = "O";
static const char* FALLBACK_RECTANGLE = "[]";
static const char* FALLBACK_POINT = "•";
static const char* FALLBACK_TRIANGLE = "△";
static const char* FALLBACK_SQUARE = "□";
static const char* FALLBACK_SPLINE = "~";
static const char* FALLBACK_BEZIER = "S";
static const char* FALLBACK_SELECT = "→";
static const char* FALLBACK_UNDO = "↶";
static const char* FALLBACK_REDO = "↷";
static const char* FALLBACK_CLEAR = "×";
static const char* FALLBACK_BELLOWS = "B";
static const char* FALLBACK_BEARING = "⚙";
static const char* FALLBACK_SUSPENSION = "S";


// Window dimensions
const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;

// UI Colors - Professional Light Theme
namespace UIColors {
// Professional light theme with teal/blue-green accents
const ImVec4 BACKGROUND = ImVec4(0.98f, 0.98f, 0.98f, 1.0f);             // Almost white background
const ImVec4 PANEL = ImVec4(0.96f, 0.96f, 0.96f, 1.0f);                  // Very light gray panels
const ImVec4 DARK_PANEL = ImVec4(0.92f, 0.92f, 0.92f, 1.0f);             // Subtle darker gray for contrast
const ImVec4 HEADER = ImVec4(0.18f, 0.69f, 0.69f, 1.0f);                 // Teal header
const ImVec4 BORDER = ImVec4(0.82f, 0.82f, 0.82f, 1.0f);                 // Light gray borders
const ImVec4 TEXT = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);                   // Near-black text for contrast
const ImVec4 TEXT_DIM = ImVec4(0.50f, 0.50f, 0.50f, 1.0f);               // Medium gray dimmed text
const ImVec4 BUTTON = ImVec4(0.94f, 0.94f, 0.94f, 1.0f);                 // Light gray buttons
const ImVec4 BUTTON_HOVERED = ImVec4(0.90f, 0.90f, 0.90f, 1.0f);         // Slightly darker when hovered
const ImVec4 BUTTON_ACTIVE = ImVec4(0.18f, 0.69f, 0.69f, 1.0f);          // Teal when active
const ImVec4 BUTTON_TEXT = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);            // Dark button text
const ImVec4 ACCENT = ImVec4(0.18f, 0.69f, 0.69f, 1.0f);                 // Teal accent color
const ImVec4 TAB_ACTIVE = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);                // White active tab background

const ImVec4 GRID_BACKGROUND = ImVec4(0.98f, 0.95f, 0.88f, 1.0f);        // Warm cream background for abacus theme
const ImVec4 COMMAND_BG = ImVec4(0.90f, 0.90f, 0.90f, 1.0f);             // Light gray command line background
const ImVec4 COMMAND_TEXT = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);           // Dark command line text
const ImVec4 SUCCESS = ImVec4(0.20f, 0.70f, 0.50f, 1.0f);                // Green-teal for success/confirmation
const ImVec4 WARNING = ImVec4(0.90f, 0.65f, 0.20f, 1.0f);                // Orange for warnings
const ImVec4 ERROR = ImVec4(0.85f, 0.25f, 0.20f, 1.0f);                  // Red for errors
} // namespace UIColors

// Global state for active tool and settings
// Moved to ApplicationContext.hpp
// We will keep the 3D viewers and UI-specific state here for now, 
// but the core state is moved to ApplicationContext.
namespace UIState {
// Resizable property panel width (user can drag to resize)
static float userPropertyPanelWidth = 280.0f;

// Layer system
static int activeLayer = 0;
static std::vector<std::string> layerNames = {"Layer 0"};
static std::vector<bool> layerVisibility = {true};
static std::vector<ImU32> layerColors = {IM_COL32(255, 255, 255, 255)};

// Unit system
static Drawing::UnitSystem units = Drawing::UnitSystem::Millimeters;
static float unitScale = 1.0f;

// Recent files
static std::vector<std::string> recentFiles;

// Workspace settings
static std::string currentWorkspace = "2D Drafting";
static bool darkMode = false;

// Add a global or static instance for the spring 3D viewer
static SpringViewer3D springViewer;
static bool spring3DViewInitialized = false;
// Add a flag for the new 3D Shock Absorber viewer
static bool shockAbsorber3DViewInitialized = false;

// 3D Viewers for Bellows, Ball Bearing, and Shock Absorber
static BellowsViewer3D bellowsViewer;
static bool bellows3DViewInitialized = false;
static BallBearingViewer3D ballBearingViewer;
static bool ballBearing3DViewInitialized = false;
static ShockAbsorberViewer3D shockAbsorberViewer;
static bool shockAbsorberViewerInitialized = false;

// Unified 3D Manager for all complex shapes
static ComplexShape3DManager shape3DManager;
static bool showBellows3DView = false;
static bool showBallBearing3DView = false;
static bool showShockAbsorber3DViewUnified = false;
} // namespace UIState

// Function to setup high-DPI rendering for Wayland/Hyprland
void SetupHighDPIRendering(SDL_Window* window, ImGuiIO& io) {
  // Get window size and drawable size
  int window_width, window_height;
  int drawable_width, drawable_height;
  
  SDL_GetWindowSize(window, &window_width, &window_height);
  SDL_GL_GetDrawableSize(window, &drawable_width, &drawable_height);
  
  // Calculate the scale factor (how much bigger the framebuffer is)
  float scale_x = (float)drawable_width / (float)window_width;
  float scale_y = (float)drawable_height / (float)window_height;
  
  // Check if we're on Wayland/Hyprland
  const char* waylandDisplay = getenv("WAYLAND_DISPLAY");
  const char* hyprlandInstance = getenv("HYPRLAND_INSTANCE_SIGNATURE");
  
  std::cout << "High-DPI Setup:" << std::endl;
  std::cout << "  Window size: " << window_width << "x" << window_height << std::endl;
  std::cout << "  Drawable size: " << drawable_width << "x" << drawable_height << std::endl;
  std::cout << "  Scale factors: " << scale_x << "x" << scale_y << std::endl;
  if (waylandDisplay) std::cout << "  Running on Wayland" << std::endl;
  if (hyprlandInstance) std::cout << "  Running on Hyprland" << std::endl;
  
  // Set ImGui display size to the window size (logical pixels)
  io.DisplaySize = ImVec2((float)window_width, (float)window_height);
  
  // Set the display framebuffer scale for high-DPI rendering
  io.DisplayFramebufferScale = ImVec2(scale_x, scale_y);
  
  // Don't use FontGlobalScale - this causes blur
  io.FontGlobalScale = 1.0f;
}

// Function to load fonts with high-DPI support
// Function to load fonts (simplified - no icon fonts needed)
void LoadFonts() {
  ImGuiIO& io = ImGui::GetIO();
  
  // Clear existing fonts
  io.Fonts->Clear();
  
  // For high-DPI, we load fonts at their native size
  // ImGui will handle the scaling via DisplayFramebufferScale
  float baseFontSize = 16.0f;  // Standard size
  
  // Load default font with high-DPI optimizations
  ImFontConfig fontConfig;
  fontConfig.SizePixels = baseFontSize;
  fontConfig.OversampleH = 3;  // Higher oversampling for better quality
  fontConfig.OversampleV = 3;
  fontConfig.PixelSnapH = true;
  fontConfig.RasterizerMultiply = 1.2f;  // Slightly bolder for better readability
  
  // Load default font
  io.Fonts->AddFontDefault(&fontConfig);
  
  // Build the font atlas with high-DPI settings
  io.Fonts->TexDesiredWidth = 2048; // Larger texture for better quality
  io.Fonts->Build();
  
  // Note: LoadIconTextures() will be called after OpenGL context is ready
}

// Function declaration prototypes
void LoadFonts();
void LoadIconTextures();
bool IconButton(const std::string& iconName, const char* fallbackText, const char* tooltip, const ImVec2& size);
void CleanupIconTextures();
void SetupHighDPIRendering(SDL_Window* window, ImGuiIO& io);
void SelectTool(Drawing::DrawingMode mode, Drawing::Canvas &canvas, Core::ApplicationContext& appContext, const std::string &message);
void SetupImGuiStyle();
void RenderTopRibbon(Drawing::Canvas &canvas, Core::ApplicationContext& appContext);
void RenderPropertyPanel(Drawing::Canvas &canvas, Core::ApplicationContext& appContext);
void RenderStatusBar(Drawing::Canvas &canvas, Core::ApplicationContext& appContext);
void RenderCanvas(Drawing::Canvas &canvas, Core::ApplicationContext& appContext);
void HandleKeyboardShortcuts(Drawing::Canvas &canvas, Core::ApplicationContext& appContext);

// Forward declare the remaining 3D view functions (unified system handles others)
void RenderSpring3DViewWindow(Drawing::Canvas &canvas, Core::ApplicationContext& appContext);
void RenderShockAbsorber3DViewWindow(Drawing::Canvas &canvas, Core::ApplicationContext& appContext);
void RenderBellows3DViewWindow(Drawing::Canvas &canvas);
void RenderBallBearing3DViewWindow(Drawing::Canvas &canvas);

// Helper function to handle tool selection
void SelectTool(Drawing::DrawingMode mode, Drawing::Canvas &canvas, Core::ApplicationContext& appContext,
                const std::string &message) {
  appContext.activeMode = mode;
  canvas.setDrawingMode(mode);
  appContext.consoleMessage = message;
}

// Implementation of missing 3D view rendering functions
void RenderSpring3DViewWindow(Drawing::Canvas &canvas, Core::ApplicationContext& appContext) {
  if (!UIState::spring3DViewInitialized) {
    UIState::springViewer.initialize();
    UIState::spring3DViewInitialized = true;
  }
  
  if (ImGui::Begin("Spring 3D View", &appContext.showSpring3DView)) {
    // Get available content region size for dynamic viewport
    ImVec2 availableSize = ImGui::GetContentRegionAvail();
    ImVec2 viewportSize = ImVec2(availableSize.x, availableSize.y - 20); // Leave some margin
    
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

void RenderShockAbsorber3DViewWindow(Drawing::Canvas &canvas, Core::ApplicationContext& appContext) {
  if (!UIState::shockAbsorberViewerInitialized) {
    UIState::shockAbsorberViewer.initialize();
    UIState::shockAbsorberViewerInitialized = true;
  }
  
  if (ImGui::Begin("Shock Absorber 3D View", &appContext.showShockAbsorber3DView)) {
    // Get available content region size for dynamic viewport
    ImVec2 availableSize = ImGui::GetContentRegionAvail();
    ImVec2 viewportSize = ImVec2(availableSize.x, availableSize.y - 20); // Leave some margin
    
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

void RenderBellows3DViewWindow(Drawing::Canvas &canvas) {
  if (!UIState::bellows3DViewInitialized) {
    UIState::bellowsViewer.initialize();
    UIState::bellows3DViewInitialized = true;
  }
  
  if (ImGui::Begin("Bellows 3D View", &UIState::showBellows3DView)) {
    // Get available content region size for dynamic viewport
    ImVec2 availableSize = ImGui::GetContentRegionAvail();
    ImVec2 viewportSize = ImVec2(availableSize.x, availableSize.y - 20); // Leave some margin
    
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

void RenderBallBearing3DViewWindow(Drawing::Canvas &canvas) {
  if (!UIState::ballBearing3DViewInitialized) {
    UIState::ballBearingViewer.initialize();
    UIState::ballBearing3DViewInitialized = true;
  }
  
  if (ImGui::Begin("Ball Bearing 3D View", &UIState::showBallBearing3DView)) {
    // Get available content region size for dynamic viewport
    ImVec2 availableSize = ImGui::GetContentRegionAvail();
    ImVec2 viewportSize = ImVec2(availableSize.x, availableSize.y - 20); // Leave some margin
    
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

void SetupImGuiStyle() {
  ImGuiStyle &style = ImGui::GetStyle();

  // Colors - Professional Light Theme
  ImVec4 *colors = style.Colors;
  colors[ImGuiCol_Text] = UIColors::TEXT;
  colors[ImGuiCol_TextDisabled] = UIColors::TEXT_DIM;
  colors[ImGuiCol_WindowBg] = UIColors::PANEL;
  colors[ImGuiCol_ChildBg] = UIColors::PANEL;
  colors[ImGuiCol_PopupBg] = UIColors::PANEL;
  colors[ImGuiCol_Border] = UIColors::BORDER;
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_FrameBg] = UIColors::DARK_PANEL;
  colors[ImGuiCol_FrameBgHovered] = UIColors::BUTTON_HOVERED;
  colors[ImGuiCol_FrameBgActive] = UIColors::BUTTON_ACTIVE;
  colors[ImGuiCol_TitleBg] = UIColors::HEADER;
  colors[ImGuiCol_TitleBgActive] = UIColors::HEADER;
  colors[ImGuiCol_TitleBgCollapsed] = UIColors::HEADER;
  colors[ImGuiCol_MenuBarBg] = UIColors::DARK_PANEL;
  colors[ImGuiCol_ScrollbarBg] = UIColors::DARK_PANEL;
  colors[ImGuiCol_ScrollbarGrab] = UIColors::BUTTON;
  colors[ImGuiCol_ScrollbarGrabHovered] = UIColors::BUTTON_HOVERED;
  colors[ImGuiCol_ScrollbarGrabActive] = UIColors::BUTTON_ACTIVE;
  colors[ImGuiCol_CheckMark] = UIColors::ACCENT;
  colors[ImGuiCol_SliderGrab] = UIColors::ACCENT;
  colors[ImGuiCol_SliderGrabActive] = UIColors::ACCENT;
  colors[ImGuiCol_Button] = UIColors::BUTTON;
  colors[ImGuiCol_ButtonHovered] = UIColors::BUTTON_HOVERED;
  colors[ImGuiCol_ButtonActive] = UIColors::BUTTON_ACTIVE;
  colors[ImGuiCol_Header] = UIColors::HEADER;
  colors[ImGuiCol_HeaderHovered] = UIColors::BUTTON_HOVERED;
  colors[ImGuiCol_HeaderActive] = UIColors::BUTTON_ACTIVE;
  colors[ImGuiCol_Separator] = UIColors::BORDER;
  colors[ImGuiCol_SeparatorHovered] = UIColors::BORDER;
  colors[ImGuiCol_SeparatorActive] = UIColors::BORDER;
  colors[ImGuiCol_Tab] = UIColors::BUTTON;
  colors[ImGuiCol_TabHovered] = UIColors::BUTTON_HOVERED;
  colors[ImGuiCol_TabActive] = UIColors::BUTTON_ACTIVE;
  colors[ImGuiCol_TabUnfocused] = UIColors::DARK_PANEL;
  colors[ImGuiCol_TabUnfocusedActive] = UIColors::BUTTON;
  colors[ImGuiCol_PlotLines] = UIColors::ACCENT;
  colors[ImGuiCol_PlotLinesHovered] = UIColors::ACCENT;
  colors[ImGuiCol_PlotHistogram] = UIColors::ACCENT;
  colors[ImGuiCol_PlotHistogramHovered] = UIColors::ACCENT;
  colors[ImGuiCol_TableHeaderBg] = UIColors::DARK_PANEL;
  colors[ImGuiCol_TableBorderStrong] = UIColors::BORDER;
  colors[ImGuiCol_TableBorderLight] = UIColors::BORDER;
  colors[ImGuiCol_TableRowBg] = UIColors::PANEL;
  colors[ImGuiCol_TableRowBgAlt] = UIColors::DARK_PANEL;

  // Styles - Modern Light Theme
  style.WindowPadding = ImVec2(10, 10);
  // Adjust frame padding for better icon centering in buttons
  style.FramePadding = ImVec2(6, 6);
  style.CellPadding = ImVec2(6, 4);
  style.ItemSpacing = ImVec2(10, 8);
  style.ItemInnerSpacing = ImVec2(8, 6);
  style.TouchExtraPadding = ImVec2(2, 2);
  style.IndentSpacing = 22;
  style.ScrollbarSize = 14;
  style.GrabMinSize = 14;

  // Borders and rounding
  style.WindowBorderSize = 1;
  style.ChildBorderSize = 1;
  style.PopupBorderSize = 1;
  style.FrameBorderSize = 1;
  style.TabBorderSize = 0;

  style.WindowRounding = 6;
  style.ChildRounding = 4;
  style.FrameRounding = 4;
  style.PopupRounding = 4;
  style.ScrollbarRounding = 4;
  style.GrabRounding = 4;
  style.TabRounding = 4;
  
  // Window title alignment
  style.WindowTitleAlign = ImVec2(0.5f, 0.5f); // Center window titles
  
  // Extra style tweaks
  style.DisplaySafeAreaPadding = ImVec2(8, 8);
  style.DisplayWindowPadding = ImVec2(8, 8);
  
  // More button-specific style adjustments
  style.ButtonTextAlign = ImVec2(0.5f, 0.5f); // Center text in buttons
}

void RenderTopRibbon(Drawing::Canvas &canvas, Core::ApplicationContext& appContext) {
  // Professional style ribbon at the top
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, 145)); // Increased to fit View panel with slider

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 6));
  ImGui::Begin("Ribbon", nullptr,
               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

  // Calculate adaptive panel and button widths based on screen size
  const float availWidth = ImGui::GetContentRegionAvail().x;
  const float panelSpacing = 8.0f;
  
  // Total width minus space for 3D button (120px) divided by 3 panels
  const float panelWidth = (availWidth - 130 - panelSpacing*2) / 3;
  
  // Calculate button sizes based on panel width and content
  const float panelHeight = 125.0f; // Height to fit View panel contents
  
  // For icon buttons, calculate size based on icon size and ensure proper centering
  const float iconSize = 20.0f; // Size of the icon font
  const float iconButtonPadding = 12.0f; // More padding to center icons properly
  const float iconButtonSize = iconSize + iconButtonPadding; // Square buttons for icons (32x32)
  
  // Calculate how many icon buttons per row based on available width
  const int iconsPerRow = std::max(2, static_cast<int>((panelWidth - 16.0f) / (iconButtonSize + 6.0f)));
  // Button spacing between icon buttons
  const float iconButtonSpacing = 6.0f;
  
  // For text buttons (like "Bellows", "Ball Bearing"), use larger size
  const float textWidth = ImGui::CalcTextSize("Ball Bearing").x;
  const float textButtonWidth = textWidth + 16.0f;
  const float textButtonHeight = 30.0f;
  
  // Draw Panel
  ImGui::BeginGroup();
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.98f, 0.98f, 0.98f, 1.0f));
  // Enable vertical scrolling for the drawing tools panel when content overflows
  ImGui::BeginChild("DrawPanel", ImVec2(panelWidth, panelHeight), true, 
                    ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NavFlattened);
  
  ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
  ImGui::PushStyleColor(ImGuiCol_Text, UIColors::TEXT_DIM);
  ImGui::Text("Draw");
  ImGui::PopStyleColor();
  ImGui::PopFont();
  
  ImGui::Dummy(ImVec2(0, 2));
  
  // Row 1: Basic drawing tools - use compact icon buttons
  float y = ImGui::GetCursorPosY();
  bool selected;
  
  // Push style for icon buttons to ensure proper centering
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 6));
  
  selected = appContext.activeMode == Drawing::DrawingMode::Point;
  if (IconButton("point", FALLBACK_POINT, "Point Tool", ImVec2(iconButtonSize, iconButtonSize))) {
    SelectTool(Drawing::DrawingMode::Point, canvas, appContext, "Point Tool: Click to place points");
  }
  
  ImGui::SameLine(0, iconButtonSpacing);
  selected = appContext.activeMode == Drawing::DrawingMode::Line;
  if (IconButton("line", FALLBACK_LINE, "Line Tool", ImVec2(iconButtonSize, iconButtonSize))) {
    SelectTool(Drawing::DrawingMode::Line, canvas, appContext, "Line Tool: Click to set start and end points");
  }
  
  if (iconsPerRow > 2) {
    ImGui::SameLine(0, iconButtonSpacing);
  } else {
    ImGui::Dummy(ImVec2(0, 3));
  }
  
  selected = appContext.activeMode == Drawing::DrawingMode::Circle;
  if (IconButton("circle", FALLBACK_CIRCLE, "Circle Tool", ImVec2(iconButtonSize, iconButtonSize))) {
    SelectTool(Drawing::DrawingMode::Circle, canvas, appContext, "Circle Tool: Click to set center and radius");
  }

  if (iconsPerRow > 3) {
    ImGui::SameLine(0, iconButtonSpacing);
  } else {
    ImGui::Dummy(ImVec2(0, 3));
  }
  
  selected = appContext.activeMode == Drawing::DrawingMode::Triangle;
  if (IconButton("triangle", FALLBACK_TRIANGLE, "Triangle Tool", ImVec2(iconButtonSize, iconButtonSize))) {
    SelectTool(Drawing::DrawingMode::Triangle, canvas, appContext, "Triangle Tool: Click to set point and direction (dimensions in properties)");
  }

  if (iconsPerRow > 4) {
    ImGui::SameLine(0, iconButtonSpacing);
  } else {
    ImGui::Dummy(ImVec2(0, 3));
  }
  
  selected = appContext.activeMode == Drawing::DrawingMode::Square;
  if (IconButton("square", FALLBACK_SQUARE, "Square Tool", ImVec2(iconButtonSize, iconButtonSize))) {
    SelectTool(Drawing::DrawingMode::Square, canvas, appContext, "Square Tool: Click to set corner and direction (side length in properties)");
  }

  if (iconsPerRow > 5) {
    ImGui::SameLine(0, iconButtonSpacing);
  } else {
    ImGui::Dummy(ImVec2(0, 3));
  }
  
  selected = appContext.activeMode == Drawing::DrawingMode::Rectangle;
  if (IconButton("rectangle", FALLBACK_RECTANGLE, "Rectangle Tool", ImVec2(iconButtonSize, iconButtonSize))) {
    SelectTool(Drawing::DrawingMode::Rectangle, canvas, appContext, "Rectangle Tool: Click to set corner and direction (dimensions in properties)");
  }
  
  // Pop the icon button style
  ImGui::PopStyleVar();

  // Start new row for additional tools
  ImGui::Dummy(ImVec2(0, 2));
  
  // Row 2: Additional drawing tools
  // Push style for icon buttons to ensure proper centering
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 6));
  
  selected = appContext.activeMode == Drawing::DrawingMode::Spline;
  if (IconButton("spline", FALLBACK_SPLINE, "Spline Tool", ImVec2(iconButtonSize, iconButtonSize))) {
    SelectTool(Drawing::DrawingMode::Spline, canvas, appContext, "Spline Tool: Click to add control points, double-click to finish");
  }
  
  ImGui::SameLine(0, iconButtonSpacing);
  selected = appContext.activeMode == Drawing::DrawingMode::BezierCurve;
  if (IconButton("bezier", FALLBACK_BEZIER, "Bezier Curve Tool", ImVec2(iconButtonSize, iconButtonSize))) {
    SelectTool(Drawing::DrawingMode::BezierCurve, canvas, appContext, "Bezier Tool: Click to add control points, double-click to finish");
  }
  
  // Pop the icon button style
  ImGui::PopStyleVar();

  if (iconsPerRow > 2) {
    ImGui::SameLine(0, iconButtonSpacing);
  } else {
    ImGui::Dummy(ImVec2(0, 3));
  }
  
  selected = appContext.activeMode == Drawing::DrawingMode::Bellows;
  if (IconButton("bellows", "Bellows", "Bellows Tool: Click to create a parametric bellows", ImVec2(textButtonWidth, textButtonHeight))) {
    SelectTool(Drawing::DrawingMode::Bellows, canvas, appContext, "Bellows Tool: Click to create a parametric bellows");
  }
  
  if (iconsPerRow > 2) {
    ImGui::SameLine(0, iconButtonSpacing);
  } else {
    ImGui::Dummy(ImVec2(0, 3));
  }
  
  selected = appContext.activeMode == Drawing::DrawingMode::BallBearing;
  if (IconButton("bearing", "Ball Bearing", "Ball Bearing Tool: Click to create a parametric ball bearing", ImVec2(textButtonWidth, textButtonHeight))) {
    SelectTool(Drawing::DrawingMode::BallBearing, canvas, appContext, "Ball Bearing Tool: Click to create a parametric ball bearing");
  }
  
  ImGui::SameLine(0, iconButtonSpacing);
  selected = appContext.activeMode == Drawing::DrawingMode::Spring2D;
  if (IconButton("suspension", "Spring", "Spring Tool: Set parameters in properties panel", ImVec2(textButtonWidth, textButtonHeight))) {
    SelectTool(Drawing::DrawingMode::Spring2D, canvas, appContext, "Spring Tool: Set parameters in properties panel");
  }
  
  ImGui::EndChild();
  ImGui::PopStyleColor();
  ImGui::EndGroup();
  
  // Edit Panel
  ImGui::SameLine(0, panelSpacing);
  ImGui::BeginGroup();
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.98f, 0.98f, 0.98f, 1.0f));
  ImGui::BeginChild("EditPanel", ImVec2(panelWidth, panelHeight), true);
  
  ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
  ImGui::PushStyleColor(ImGuiCol_Text, UIColors::TEXT_DIM);
  ImGui::Text("Edit");
  ImGui::PopStyleColor();
  ImGui::PopFont();
  
  ImGui::Dummy(ImVec2(0, 2));
  
  // Edit tools - use compact icon buttons
  // Push style for icon buttons to ensure proper centering
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 6));
  
  selected = appContext.activeMode == Drawing::DrawingMode::Select;
  if (IconButton("select", FALLBACK_SELECT, "Select Tool", ImVec2(iconButtonSize, iconButtonSize))) {
    SelectTool(Drawing::DrawingMode::Select, canvas, appContext, "Select Tool: Click to select objects");
  }
  
  ImGui::SameLine(0, iconButtonSpacing);
  if (IconButton("undo", FALLBACK_UNDO, "Undo", ImVec2(iconButtonSize, iconButtonSize))) {
    canvas.undo();
    appContext.consoleMessage = "Undo";
  }
  
  // Pop the icon button style
  ImGui::PopStyleVar();
  
  if (iconsPerRow > 2) {
    ImGui::SameLine(0, iconButtonSpacing);
  } else {
    ImGui::Dummy(ImVec2(0, 3));
  }
  
  if (IconButton("redo", FALLBACK_REDO, "##redo_tool", ImVec2(iconButtonSize, iconButtonSize))) {
    canvas.redo();
    appContext.consoleMessage = "Redo";
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Redo");
  }
  
  if (iconsPerRow > 2) {
    ImGui::SameLine(0, iconButtonSpacing);
  } else {
    ImGui::Dummy(ImVec2(0, 3));
  }
  
  // Add Clear All button using text button size
  if (IconButton("clear", "Clear All", "Clear All: Remove all shapes from canvas", ImVec2(textButtonWidth, textButtonHeight))) {
    canvas.clearAll();
    appContext.consoleMessage = "All shapes cleared";
  }
  
  ImGui::EndChild();
  ImGui::PopStyleColor();
  ImGui::EndGroup();
  
  // View Panel
  ImGui::SameLine(0, panelSpacing);
  ImGui::BeginGroup();
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.98f, 0.98f, 0.98f, 1.0f));
  ImGui::BeginChild("ViewPanel", ImVec2(panelWidth, panelHeight), true);
  
  ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
  ImGui::PushStyleColor(ImGuiCol_Text, UIColors::TEXT_DIM);
  ImGui::Text("View");
  ImGui::PopStyleColor();
  ImGui::PopFont();
  
  ImGui::Dummy(ImVec2(0, 2));
  
  // Grid and snap toggles with reduced width elements
  bool showGrid = canvas.isGridVisible();
  ImGui::PushItemWidth(80.0f); // Fixed width for slider that fits in panel
  if (ImGui::Checkbox("Grid", &showGrid)) {
    canvas.setShowGrid(showGrid);
    appContext.consoleMessage = showGrid ? "Grid: ON" : "Grid: OFF";
  }
  
  bool snapToGrid = canvas.isSnapToGridEnabled();
  if (ImGui::Checkbox("Snap", &snapToGrid)) {
    canvas.setSnapToGrid(snapToGrid);
    appContext.snapEnabled = snapToGrid;
    appContext.consoleMessage = snapToGrid ? "Snap to Grid: ON" : "Snap to Grid: OFF";
  }
  
  // Grid size slider
  float gridSize = appContext.gridSize;
  if (ImGui::SliderFloat("Size", &gridSize, 5.0f, 50.0f, "%.0f")) {
    appContext.gridSize = gridSize;
    canvas.setGridSpacing(gridSize);
    appContext.consoleMessage = "Grid Size Updated";
  }
  ImGui::PopItemWidth();
  
  ImGui::EndChild();
  ImGui::PopStyleColor();
  ImGui::EndGroup();
  
  // Right-aligned section for 3D view button
  ImGui::SameLine();
  
  // Shape-specific 3D view buttons
  ImGui::PushStyleColor(ImGuiCol_Button, UIColors::ACCENT);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.73f, 0.73f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
  
  // Check if we have any complex shapes to show 3D views for
  Drawing::Shape* selectedShape = canvas.getSelectedShape();
  bool hasComplexShapes = false;
  
  if (selectedShape) {
    if (selectedShape->type == Drawing::ShapeType::BELLOWS) {
      if (ImGui::Button("3D Bellows", ImVec2(100, 30))) {
        UIState::showBellows3DView = true;
        appContext.consoleMessage = "Opening 3D Bellows View";
      }
      hasComplexShapes = true;
    }
    else if (selectedShape->type == Drawing::ShapeType::BALL_BEARING) {
      if (ImGui::Button("3D Ball Bearing", ImVec2(100, 30))) {
        UIState::showBallBearing3DView = true;
        appContext.consoleMessage = "Opening 3D Ball Bearing View";
      }
      hasComplexShapes = true;
    }
  }
  
  // If no complex shape selected, show a disabled button with tooltip
  if (!hasComplexShapes) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    ImGui::Button("3D View", ImVec2(100, 30));
    ImGui::PopStyleColor(1);
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Select a complex shape (Bellows, Ball Bearing, etc.) to view in 3D");
    }
  }
  
  ImGui::PopStyleColor(3);
  
  ImGui::End();
  ImGui::PopStyleVar();
}

void RenderStatusBar(Drawing::Canvas &canvas, Core::ApplicationContext& appContext) {
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
  switch (UIState::units) {
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
  
  float scaledX = mousePos.x / UIState::unitScale;
  float scaledY = mousePos.y / UIState::unitScale;
  
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

void RenderPropertyPanel(Drawing::Canvas &canvas, Core::ApplicationContext& appContext) {
  // Calculate property panel width - user can resize horizontally
  const float screenWidth = ImGui::GetIO().DisplaySize.x;
  const float propertyPanelMinWidth = 220.0f; // Minimum width
  const float propertyPanelMaxWidth = screenWidth * 0.4f; // Maximum 40% of screen
  const float ribbonHeight = 145.0f; // Match ribbon height
  const float statusBarHeight = 28.0f;
  
  // Clamp user width to valid range
  UIState::userPropertyPanelWidth = std::max(propertyPanelMinWidth, std::min(propertyPanelMaxWidth, UIState::userPropertyPanelWidth));
  
  ImGui::SetNextWindowPos(ImVec2(screenWidth - UIState::userPropertyPanelWidth, ribbonHeight), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(UIState::userPropertyPanelWidth, ImGui::GetIO().DisplaySize.y - ribbonHeight - statusBarHeight), ImGuiCond_Always);
  
  // Allow horizontal resize only (NoMove keeps vertical position fixed)
  ImGui::Begin("Properties", nullptr,
               ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
  
  // Update stored width if user resized the window
  ImVec2 currentSize = ImGui::GetWindowSize();
  if (currentSize.x != UIState::userPropertyPanelWidth) {
    UIState::userPropertyPanelWidth = currentSize.x;
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
          UIState::showBellows3DView = true;
        }
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
          UIState::showBallBearing3DView = true;
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
      UIState::showShockAbsorber3DViewUnified = true;
    }
  }
}

void RenderCanvas(Drawing::Canvas &canvas, Core::ApplicationContext& appContext) {
  // Adjust canvas position and size to account for the ribbon and status bar
  // Using simplified UI layout (no tabs, single row of tools)
  const float ribbonHeight = 145.0f; // Updated height to match ribbon with View slider
  const float statusBarHeight = 28.0f; // Just status bar now, no command line
  
  // Use the user-resizable property panel width
  float canvasX = 0.0f;
  float canvasY = ribbonHeight;
  float canvasWidth = ImGui::GetIO().DisplaySize.x - UIState::userPropertyPanelWidth;
  float canvasHeight = ImGui::GetIO().DisplaySize.y - ribbonHeight - statusBarHeight;

  ImGui::SetNextWindowPos(ImVec2(canvasX, canvasY));
  ImGui::SetNextWindowSize(ImVec2(canvasWidth, canvasHeight));

  // Set canvas background color for the light theme
  ImGui::PushStyleColor(ImGuiCol_WindowBg, UIColors::GRID_BACKGROUND);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  
  ImGui::Begin("Canvas", nullptr,
               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
                   ImGuiWindowFlags_NoTitleBar);

  // Get the canvas window position for proper coordinate transformation
  ImVec2 canvasPos = ImGui::GetWindowPos();
  ImVec2 canvasSize = ImGui::GetWindowSize();

  // Update canvas with window information for proper coordinate transformation
  canvas.setWindowInfo(canvasPos.x, canvasPos.y, canvasSize.x, canvasSize.y);

  // Create clipping rectangle for the drawing area
  ImGui::PushClipRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), true);
  
  // Handle canvas input and rendering
  canvas.handleInput();

  ImDrawList *drawList = ImGui::GetWindowDrawList();
  canvas.render(drawList);
  
  // Add ruler indicators if enabled (AutoCAD has rulers)
  if (appContext.showRulers) {
    const float rulerSize = 20.0f;
    const float majorTickHeight = 10.0f;
    const float minorTickHeight = 5.0f;
    const float tickSpacing = appContext.gridSize * appContext.zoomLevel;
    
    // Background for rulers
    drawList->AddRectFilled(
      ImVec2(canvasPos.x, canvasPos.y),
      ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + rulerSize),
      IM_COL32(45, 45, 48, 255) // Dark gray background for rulers
    );
    
    drawList->AddRectFilled(
      ImVec2(canvasPos.x, canvasPos.y),
      ImVec2(canvasPos.x + rulerSize, canvasPos.y + canvasSize.y),
      IM_COL32(45, 45, 48, 255) // Dark gray background for rulers
    );
    
    // Horizontal ruler ticks
    float originX = canvasPos.x + appContext.panOffset.x * appContext.zoomLevel;
    for (float x = originX; x < canvasPos.x + canvasSize.x; x += tickSpacing) {
      int majorTick = (int)std::round((x - originX) / tickSpacing);
      if (majorTick % 5 == 0) {
        // Major tick
        drawList->AddLine(
          ImVec2(x, canvasPos.y),
          ImVec2(x, canvasPos.y + majorTickHeight),
          IM_COL32(200, 200, 200, 255), // Light color for ticks
          1.0f
        );
        
        // Label for major ticks
        char label[16];
        int value = majorTick * appContext.gridSize / UIState::unitScale;
        sprintf(label, "%d", value);
        drawList->AddText(
          ImVec2(x + 2, canvasPos.y + 2),
          IM_COL32(200, 200, 200, 255), // Light color for text
          label
        );
      } else {
        // Minor tick
        drawList->AddLine(
          ImVec2(x, canvasPos.y),
          ImVec2(x, canvasPos.y + minorTickHeight),
          IM_COL32(150, 150, 150, 255), // Lighter color for minor ticks
          1.0f
        );
      }
    }
    
    // Vertical ruler ticks
    float originY = canvasPos.y + appContext.panOffset.y * appContext.zoomLevel;
    for (float y = originY; y < canvasPos.y + canvasSize.y; y += tickSpacing) {
      int majorTick = (int)std::round((y - originY) / tickSpacing);
      if (majorTick % 5 == 0) {
        // Major tick
        drawList->AddLine(
          ImVec2(canvasPos.x, y),
          ImVec2(canvasPos.x + majorTickHeight, y),
          IM_COL32(200, 200, 200, 255), // Light color for ticks
          1.0f
        );
        
        // Label for major ticks
        char label[16];
        int value = majorTick * appContext.gridSize / UIState::unitScale;
        sprintf(label, "%d", value);
        drawList->AddText(
          ImVec2(canvasPos.x + 2, y + 2),
          IM_COL32(200, 200, 200, 255), // Light color for text
          label
        );
      } else {
        // Minor tick
        drawList->AddLine(
          ImVec2(canvasPos.x, y),
          ImVec2(canvasPos.x + minorTickHeight, y),
          IM_COL32(150, 150, 150, 255), // Lighter color for minor ticks
          1.0f
        );
      }
    }
    
    // Corner square
    drawList->AddRectFilled(
      ImVec2(canvasPos.x, canvasPos.y),
      ImVec2(canvasPos.x + rulerSize, canvasPos.y + rulerSize),
      IM_COL32(50, 50, 55, 255) // Slightly darker for corner
    );
    
    // Origin indicator
    float originScreenX = canvasPos.x + appContext.panOffset.x * appContext.zoomLevel;
    float originScreenY = canvasPos.y + appContext.panOffset.y * appContext.zoomLevel;
    if (originScreenX >= canvasPos.x && originScreenX <= canvasPos.x + canvasSize.x &&
        originScreenY >= canvasPos.y && originScreenY <= canvasPos.y + canvasSize.y) {
      // Draw origin indicator
      drawList->AddCircleFilled(
        ImVec2(originScreenX, originScreenY),
        3.0f,
        IM_COL32(0, 170, 255, 255) // Blue dot for origin
      );
    }
  }
  
  // Mouse coordinates display at cursor if enabled
  if (appContext.showCoordinates) {
    ImVec2 mousePos = ImGui::GetMousePos();
    if (ImGui::IsWindowHovered() && 
        mousePos.x > canvasPos.x && mousePos.x < canvasPos.x + canvasSize.x &&
        mousePos.y > canvasPos.y && mousePos.y < canvasPos.y + canvasSize.y) {
      
      // Calculate world coordinates
      float worldX = (mousePos.x - canvasPos.x - appContext.panOffset.x * appContext.zoomLevel) / appContext.zoomLevel;
      float worldY = (mousePos.y - canvasPos.y - appContext.panOffset.y * appContext.zoomLevel) / appContext.zoomLevel;
      
      // Apply unit scaling
      worldX /= UIState::unitScale;
      worldY /= UIState::unitScale;
      
      // Format the coordinates
      char coordinates[64];
      std::string unitSuffix;
      switch (UIState::units) {
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
      
      sprintf(coordinates, "X: %.2f%s  Y: %.2f%s", worldX, unitSuffix.c_str(), worldY, unitSuffix.c_str());
      
      // Draw coordinates near cursor
      ImVec2 textSize = ImGui::CalcTextSize(coordinates);
      ImVec2 textPos = ImVec2(mousePos.x + 15, mousePos.y + 10);
      
      // Ensure text stays within canvas
      if (textPos.x + textSize.x > canvasPos.x + canvasSize.x)
        textPos.x = mousePos.x - textSize.x - 10;
      if (textPos.y + textSize.y > canvasPos.y + canvasSize.y)
        textPos.y = mousePos.y - textSize.y - 10;
      
      // Background for better readability
      drawList->AddRectFilled(
        ImVec2(textPos.x - 4, textPos.y - 2),
        ImVec2(textPos.x + textSize.x + 4, textPos.y + textSize.y + 2),
        IM_COL32(40, 40, 45, 220) // Dark background
      );
      
      // Text
      drawList->AddText(
        textPos,
        IM_COL32(220, 220, 220, 255), // Light text color
        coordinates
      );
    }
  }
  
  ImGui::PopClipRect();
  ImGui::End();
  ImGui::PopStyleVar();
  ImGui::PopStyleColor();
}


// Handle keyboard shortcuts
void HandleKeyboardShortcuts(Drawing::Canvas &canvas, Core::ApplicationContext& appContext) {
  ImGuiIO& io = ImGui::GetIO();
  
  // AutoCAD-like keyboard shortcuts
  
  // ESC to cancel current operation or clear selection
  if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
    SelectTool(Drawing::DrawingMode::Select, canvas, appContext, "Ready");
    canvas.clearSelection();
  }
  
  // Drawing tools
  if (!io.KeyCtrl && !io.KeyShift && !io.KeyAlt) {
    // Simple key presses for common tools
    if (ImGui::IsKeyPressed(ImGuiKey_L)) {
      SelectTool(Drawing::DrawingMode::Line, canvas, appContext, "Line Tool: Click to set start point, second click sets direction");
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_C)) {
      SelectTool(Drawing::DrawingMode::Circle, canvas, appContext, "Circle Tool: Click to set center and direction (radius in properties)");
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_R)) {
      SelectTool(Drawing::DrawingMode::Rectangle, canvas, appContext, "Rectangle Tool: Click to set corner and direction (dimensions in properties)");
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_P)) {
      SelectTool(Drawing::DrawingMode::Point, canvas, appContext, "Point Tool: Click to place points");
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_T)) {
      SelectTool(Drawing::DrawingMode::Triangle, canvas, appContext, "Triangle Tool: Click to set point and direction (dimensions in properties)");
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_S)) {
      SelectTool(Drawing::DrawingMode::Square, canvas, appContext, "Square Tool: Click to set corner and direction (side length in properties)");
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_G)) {
      bool currentGridState = canvas.isGridVisible();
      canvas.setShowGrid(!currentGridState);
      appContext.consoleMessage = !currentGridState ? "Grid turned ON" : "Grid turned OFF";
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
      canvas.deleteSelectedShape();
      appContext.consoleMessage = "Deleted selected shape";
    }
  }
  
  // Modifier key combinations
  if (io.KeyCtrl) {
    if (ImGui::IsKeyPressed(ImGuiKey_Z)) {
      canvas.undo();
      appContext.consoleMessage = "Undo last action";
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_A)) {
      // Select all - would need implementation
      appContext.consoleMessage = "Select All (not implemented)";
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_S)) {
      // Save - would need implementation
      appContext.consoleMessage = "Save Drawing (not implemented)";
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_O)) {
      // Open - would need implementation
      appContext.consoleMessage = "Open Drawing (not implemented)";
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_N)) {
      // New drawing - would need implementation
      canvas.clearAll();
      appContext.consoleMessage = "New Drawing - All shapes cleared";
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_Equal)) {
      // Zoom in
      appContext.zoomLevel = std::min(appContext.zoomLevel * 1.2f, 5.0f);
      appContext.consoleMessage = "Zoom In";
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_Minus)) {
      // Zoom out
      appContext.zoomLevel = std::max(appContext.zoomLevel / 1.2f, 0.2f);
      appContext.consoleMessage = "Zoom Out";
    }
  }
  
  if (io.KeyShift) {
    if (ImGui::IsKeyPressed(ImGuiKey_L)) {
      // AutoCAD-like: Shift+L for polyline
      appContext.consoleMessage = "Polyline command (not implemented)";
    }
  }
}

// Function to load a PNG icon into an OpenGL texture
GLuint LoadIconTexture(const std::string& iconPath, ImVec2& outSize) {
#ifdef HAVE_SDL2_IMAGE
  // Try to load the image using SDL_image
  SDL_Surface* surface = IMG_Load(iconPath.c_str());
  if (!surface) {
    std::cout << "Failed to load icon: " << iconPath << " - " << IMG_GetError() << std::endl;
    return 0;
  }
#else
  std::cout << "SDL2_image not available - cannot load icon: " << iconPath << std::endl;
  return 0;
#endif
  
#ifdef HAVE_SDL2_IMAGE
  // Convert to RGBA if necessary
  SDL_Surface* rgba_surface = nullptr;
  if (surface->format->format != SDL_PIXELFORMAT_RGBA32) {
    rgba_surface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surface);
    surface = rgba_surface;
    if (!surface) {
      std::cout << "Failed to convert icon to RGBA: " << iconPath << std::endl;
      return 0;
    }
  }
  
  // Store the size
  outSize = ImVec2(static_cast<float>(surface->w), static_cast<float>(surface->h));
  
  // Create OpenGL texture
  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  
  // Set texture parameters for crisp icons
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  
  // Upload the texture data
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
  
  SDL_FreeSurface(surface);
  
  std::cout << "Successfully loaded icon: " << iconPath << " (" << outSize.x << "x" << outSize.y << ")" << std::endl;
  return texture;
#endif
}

// Function to load all icon textures
void LoadIconTextures() {
#ifdef HAVE_SDL2_IMAGE
  // Initialize SDL_image if not already done
  static bool sdl_image_initialized = false;
  if (!sdl_image_initialized) {
    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
      std::cout << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
      return;
    }
    sdl_image_initialized = true;
  }
#else
  std::cout << "SDL2_image not available - icon loading disabled" << std::endl;
  return;
#endif
  
  // Define icon files and their names based on converted PNG files
  std::vector<std::pair<std::string, std::string>> iconFiles = {
    {"line", "../icons/png/line-segment.png"},
    {"circle", "../icons/png/circle.png"},
    {"rectangle", "../icons/png/rectangle.png"},
    {"point", "../icons/png/dot.png"},
    {"triangle", "../icons/png/triangle.png"},
    {"square", "../icons/png/square.png"},
    {"spline", "../icons/png/spline-curve.png"},
    {"bezier", "../icons/png/bezier-curve.png"},
    {"select", "../icons/png/select.png"},
    {"undo", "../icons/png/undo.png"},
    {"redo", "../icons/png/redo.png"},
    {"clear", "../icons/png/clear.png"},
    {"bearing", "../icons/png/bearing.png"},
    {"bellows", "../icons/png/bellows.png"},
    {"suspension", "../icons/png/suspension.png"}
  };
  
  // Load each icon
  for (const auto& iconPair : iconFiles) {
    const std::string& name = iconPair.first;
    const std::string& path = iconPair.second;
    
    ImVec2 size;
    GLuint texture = LoadIconTexture(path, size);
    
    if (texture != 0) {
      iconTextures[name] = texture;
      iconSizes[name] = size;
    }
  }
  
  std::cout << "Loaded " << iconTextures.size() << " icon textures" << std::endl;
}

// Function to create an image button with fallback to text
bool IconButton(const std::string& iconName, const char* fallbackText, const char* tooltip, const ImVec2& size) {
  bool pressed = false;
  
  // Try to use texture icon first
  auto textureIt = iconTextures.find(iconName);
  if (textureIt != iconTextures.end()) {
    GLuint texture = textureIt->second;
    ImVec2 iconSize = iconSizes[iconName];
    
    // Scale icon to fit button while maintaining aspect ratio
    float scale = std::min(size.x / iconSize.x, size.y / iconSize.y) * 0.8f; // 80% of button size
    ImVec2 scaledSize = ImVec2(iconSize.x * scale, iconSize.y * scale);
    
    // Create image button with proper styling
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0)); // Transparent background
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, UIColors::BUTTON_HOVERED);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, UIColors::BUTTON_ACTIVE);
    
    pressed = ImGui::ImageButton(("##" + iconName).c_str(), (ImTextureID)(uintptr_t)texture, scaledSize);
    
    ImGui::PopStyleColor(3);
  } else {
    // Fallback to text button
    pressed = ImGui::Button((std::string(fallbackText) + "##" + iconName).c_str(), size);
  }
  
  // Add tooltip
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s", tooltip);
  }
  
  return pressed;
}

// Function to cleanup icon textures
void CleanupIconTextures() {
  for (auto& pair : iconTextures) {
    glDeleteTextures(1, &pair.second);
  }
  iconTextures.clear();
  iconSizes.clear();
}

int main(int argc, char* argv[]) {
    Core::ApplicationContext appContext;
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        std::cerr << "Error: SDL_Init(): " << SDL_GetError() << std::endl;
        return -1;
    }

    // Setup OpenGL context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // Create window
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("NAVIX - Engineering Drawing Tool", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
        WINDOW_WIDTH, WINDOW_HEIGHT, window_flags);
    
    if (window == nullptr) {
        std::cerr << "Error: SDL_CreateWindow(): " << SDL_GetError() << std::endl;
        return -1;
    }

    // Create OpenGL context
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (gl_context == nullptr) {
        std::cerr << "Error: SDL_GL_CreateContext(): " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Error: Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigWindowsMoveFromTitleBarOnly = true;  // Only allow window dragging from title bar

    // Setup Dear ImGui style
    SetupImGuiStyle();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Load fonts and icons after OpenGL context is ready
    LoadFonts();
    LoadIconTextures();

    // Initialize canvas
    Drawing::Canvas canvas;

    // Main loop
    bool done = false;
    while (!done) {
        // Poll and handle events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Skip rendering when minimized
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
            SDL_Delay(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Handle keyboard shortcuts
        HandleKeyboardShortcuts(canvas, appContext);

        // Main UI rendering
        RenderTopRibbon(canvas, appContext);
        RenderCanvas(canvas, appContext);
        RenderPropertyPanel(canvas, appContext);
        RenderStatusBar(canvas, appContext);

        // Render 3D views if enabled
        if (UIState::showBellows3DView) {
          RenderBellows3DViewWindow(canvas);
        }
        
        if (UIState::showBallBearing3DView) {
          RenderBallBearing3DViewWindow(canvas);
        }
        
        // Note: The 3D views need to be implemented to work with the canvas shapes
        // For now, the 3D view flags are being set but the rendering needs shape data
        if (appContext.showSpring3DView) {
            RenderSpring3DViewWindow(canvas, appContext);
        }
        if (appContext.showShockAbsorber3DView) {
            RenderShockAbsorber3DViewWindow(canvas, appContext);
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(UIColors::BACKGROUND.x, UIColors::BACKGROUND.y, UIColors::BACKGROUND.z, UIColors::BACKGROUND.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    CleanupIconTextures();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
