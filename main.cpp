#include "Canvas.hpp"
#include <SDL2/SDL.h>
#include "ui/Viewers3DUI.hpp"
#include "ui/CanvasView.hpp"
#include "ui/PropertyPanel.hpp"
#include "ui/StatusBar.hpp"
#include "ui/UIState.hpp"
#include "ui/UIHelpers.hpp"
#include "ui/UIColors.hpp"
#include "ui/TopRibbon.hpp"
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




// Window dimensions
const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;

// UI Colors - Professional Light Theme

// Global state for active tool and settings
// Moved to ApplicationContext.hpp
// We will keep the 3D viewers and UI-specific state here for now, 
// but the core state is moved to ApplicationContext.

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

// Forward declare the remaining 3D view functions (unified system handles others)


// Implementation of missing 3D view rendering functions




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






// Handle keyboard shortcuts

// Function to load a PNG icon into an OpenGL texture

// Function to load all icon textures

// Function to create an image button with fallback to text

// Function to cleanup icon textures

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
        UI::CanvasView::HandleKeyboardShortcuts(canvas, appContext);

        // Main UI rendering
        UI::TopRibbon::Render(canvas, appContext);
        UI::CanvasView::Render(canvas, appContext);
        UI::PropertyPanel::Render(canvas, appContext);
        UI::StatusBar::Render(canvas, appContext);

        // Render 3D views if enabled
        if (UIState::showBellows3DView) {
          UI::Viewers3DUI::RenderBellows3DViewWindow(canvas);
        }
        
        if (UIState::showBallBearing3DView) {
          UI::Viewers3DUI::RenderBallBearing3DViewWindow(canvas);
        }
        
        // Note: The 3D views need to be implemented to work with the canvas shapes
        // For now, the 3D view flags are being set but the rendering needs shape data
        if (appContext.showSpring3DView) {
            UI::Viewers3DUI::RenderSpring3DViewWindow(canvas, appContext);
        }
        if (appContext.showShockAbsorber3DView) {
            UI::Viewers3DUI::RenderShockAbsorber3DViewWindow(canvas, appContext);
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
