#include "Canvas.hpp"
#include <SDL2/SDL.h>
#include "ui/Viewers3DUI.hpp"
#include "ui/CanvasView.hpp"
#include "ui/PropertyPanel.hpp"
#include "ui/StatusBar.hpp"
#include "ui/TopRibbon.hpp"
#include "ui/UIHelpers.hpp"
#include "ui/UIColors.hpp"
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
#include "ui/Toolbar3D.hpp"
#include "ui/PropertyPanel3D.hpp"
#include "ui/FeatureTree3D.hpp"
#include "modeling3d/CommandManager3D.hpp"
#include <ImGuizmo/ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>




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
bool IconButton(const std::string& iconName, const char* fallbackText, const char* tooltip, const ImVec2& size, bool isActive);
void CleanupIconTextures();
void SetupHighDPIRendering(SDL_Window* window, ImGuiIO& io);
void SelectTool(Drawing::DrawingMode mode, Drawing::Canvas &canvas, Core::ApplicationContext& appContext, const std::string &message);
void SetupImGuiStyle();

// Forward declare the remaining 3D view functions (unified system handles others)


// Implementation of missing 3D view rendering functions




void SetupImGuiStyle() {
  ImGuiStyle &style = ImGui::GetStyle();

  // === NAVIX Professional Dark Theme ===
  ImVec4 *colors = style.Colors;
  colors[ImGuiCol_Text] = UIColors::TEXT;
  colors[ImGuiCol_TextDisabled] = UIColors::TEXT_DIM;
  colors[ImGuiCol_WindowBg] = UIColors::PANEL;
  colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f); // Transparent children inherit parent
  colors[ImGuiCol_PopupBg] = ImVec4(0.42f, 0.43f, 0.49f, 0.97f);
  colors[ImGuiCol_Border] = UIColors::BORDER;
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_FrameBg] = ImVec4(0.36f, 0.37f, 0.43f, 1.0f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.44f, 0.45f, 0.52f, 1.0f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.50f);
  colors[ImGuiCol_TitleBg] = UIColors::DARK_PANEL;
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.46f, 0.47f, 0.53f, 1.0f);
  colors[ImGuiCol_TitleBgCollapsed] = UIColors::DARK_PANEL;
  colors[ImGuiCol_MenuBarBg] = UIColors::TOOLBAR;
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.40f, 0.41f, 0.46f, 0.6f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.56f, 0.57f, 0.64f, 1.0f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.64f, 0.65f, 0.72f, 1.0f);
  colors[ImGuiCol_ScrollbarGrabActive] = UIColors::ACCENT;
  colors[ImGuiCol_CheckMark] = UIColors::ACCENT;
  colors[ImGuiCol_SliderGrab] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
  colors[ImGuiCol_SliderGrabActive] = UIColors::ACCENT;
  colors[ImGuiCol_Button] = UIColors::BUTTON;
  colors[ImGuiCol_ButtonHovered] = UIColors::BUTTON_HOVERED;
  colors[ImGuiCol_ButtonActive] = UIColors::BUTTON_ACTIVE;
  colors[ImGuiCol_Header] = UIColors::HEADER;
  colors[ImGuiCol_HeaderHovered] = UIColors::HEADER_HOVERED;
  colors[ImGuiCol_HeaderActive] = UIColors::ACCENT_DIM;
  colors[ImGuiCol_Separator] = ImVec4(0.55f, 0.56f, 0.62f, 1.0f);
  colors[ImGuiCol_SeparatorHovered] = UIColors::ACCENT_DIM;
  colors[ImGuiCol_SeparatorActive] = UIColors::ACCENT;
  colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.60f);
  colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.90f);
  colors[ImGuiCol_Tab] = ImVec4(0.44f, 0.45f, 0.51f, 1.0f);
  colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
  colors[ImGuiCol_TabActive] = UIColors::TAB_ACTIVE;
  colors[ImGuiCol_TabUnfocused] = UIColors::DARK_PANEL;
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.44f, 0.45f, 0.51f, 1.0f);
  colors[ImGuiCol_PlotLines] = UIColors::ACCENT;
  colors[ImGuiCol_PlotLinesHovered] = UIColors::ACCENT_HOVER;
  colors[ImGuiCol_PlotHistogram] = UIColors::ACCENT;
  colors[ImGuiCol_PlotHistogramHovered] = UIColors::ACCENT_HOVER;
  colors[ImGuiCol_TableHeaderBg] = ImVec4(0.48f, 0.49f, 0.55f, 1.0f);
  colors[ImGuiCol_TableBorderStrong] = UIColors::BORDER;
  colors[ImGuiCol_TableBorderLight] = ImVec4(0.52f, 0.53f, 0.58f, 1.0f);
  colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
  colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.42f, 0.43f, 0.48f, 0.40f);

  // Geometry - Modern rounded style
  style.WindowPadding = ImVec2(8, 8);
  style.FramePadding = ImVec2(6, 4);
  style.CellPadding = ImVec2(6, 3);
  style.ItemSpacing = ImVec2(8, 5);
  style.ItemInnerSpacing = ImVec2(6, 4);
  style.TouchExtraPadding = ImVec2(2, 2);
  style.IndentSpacing = 20;
  style.ScrollbarSize = 12;
  style.GrabMinSize = 10;

  // Borders - subtle
  style.WindowBorderSize = 1;
  style.ChildBorderSize = 1;
  style.PopupBorderSize = 1;
  style.FrameBorderSize = 0;
  style.TabBorderSize = 0;

  // Rounding - modern
  style.WindowRounding = 4;
  style.ChildRounding = 4;
  style.FrameRounding = 3;
  style.PopupRounding = 4;
  style.ScrollbarRounding = 6;
  style.GrabRounding = 3;
  style.TabRounding = 3;
  
  // Alignment
  style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
  style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
  style.DisplaySafeAreaPadding = ImVec2(4, 4);
  style.DisplayWindowPadding = ImVec2(4, 4);
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

    // Clear FEA/modal/convergence results when a shape is deleted
    canvas.setOnShapeDeleting([&appContext](Drawing::Shape* shape) {
        if (!shape) return;
        if (shape->type == Drawing::ShapeType::BELLOWS) {
            if (appContext.bellows3DViewInitialized) {
                auto* model = appContext.bellowsViewer.getModel();
                if (model) {
                    model->clearFEMResult();
                    model->clearModalResult();
                    model->clearConvergenceData();
                    model->clearFEMMesh();
                }
            }
            appContext.showBellows3DView = false;
        }
        else if (shape->type == Drawing::ShapeType::SPRING2D ||
                 shape->type == Drawing::ShapeType::SHOCK_ABSORBER_END_2D ||
                 shape->type == Drawing::ShapeType::SHOCK_ABSORBER_BOTTOM_END) {
            if (appContext.shockAbsorberViewerInitialized) {
                auto* model = appContext.shockAbsorberViewer.getModel();
                if (model) {
                    model->clearFEMMesh();
                }
            }
            appContext.showShockAbsorber3DView = false;
            appContext.showSpring3DView = false;
        }
        else if (shape->type == Drawing::ShapeType::BALL_BEARING) {
            if (appContext.ballBearing3DViewInitialized) {
                auto* model = appContext.ballBearingViewer.getModel();
                if (model) {
                    model->clearFEMMesh();
                }
            }
            appContext.showBallBearing3DView = false;
        }
    });

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

        // Main UI rendering — branch on workspace mode
        if (appContext.currentWorkspaceMode == Core::WorkspaceMode::Mode3D) {
            // ═══════ 3D MODE ═══════
            // Initialize viewport on first use
            if (!appContext.viewport3DInitialized) {
                appContext.viewport3D.initialize();
                appContext.viewport3DInitialized = true;
            }

            // 3D Toolbar (replaces TopRibbon)
            UI::Toolbar3D::Render(appContext.viewport3D, appContext);

            // Feature Tree (left panel)
            UI::FeatureTree3D::Render(appContext.viewport3D, appContext);

            // 3D Viewport canvas area (center, between feature tree and properties)
            {
                const float ribbonHeight = 48.0f;
                const float statusBarHeight = 28.0f;
                const float featureTreeWidth = 220.0f;
                float canvasX = featureTreeWidth;
                float canvasWidth = io.DisplaySize.x - featureTreeWidth - appContext.userPropertyPanelWidth;
                float canvasHeight = io.DisplaySize.y - ribbonHeight - statusBarHeight;

                // Render 3D scene to FBO
                appContext.viewport3D.render(
                    static_cast<int>(canvasWidth),
                    static_cast<int>(canvasHeight));

                // Display FBO texture in ImGui
                ImGui::SetNextWindowPos(ImVec2(canvasX, ribbonHeight));
                ImGui::SetNextWindowSize(ImVec2(canvasWidth, canvasHeight));
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.11f, 0.12f, 0.14f, 1.0f));
                ImGui::Begin("##3DCanvas", nullptr,
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoScrollbar);

                ImGui::Image(
                    (ImTextureID)(intptr_t)appContext.viewport3D.getRenderedTexture(),
                    ImVec2(canvasWidth, canvasHeight),
                    ImVec2(0, 1), ImVec2(1, 0)
                );

                // Handle mouse input in the 3D viewport
                if (ImGui::IsWindowHovered()) {
                    ImVec2 windowPos = ImGui::GetWindowPos();
                    ImVec2 mousePos = ImGui::GetMousePos();
                    float localX = mousePos.x - windowPos.x;
                    float localY = mousePos.y - windowPos.y;

                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                        appContext.viewport3D.handleMouseButton(0, true, localX, localY);
                    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                        appContext.viewport3D.handleMouseButton(0, false, localX, localY);
                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
                        appContext.viewport3D.handleMouseButton(1, true, localX, localY);
                    if (ImGui::IsMouseReleased(ImGuiMouseButton_Middle))
                        appContext.viewport3D.handleMouseButton(1, false, localX, localY);
                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                        appContext.viewport3D.handleMouseButton(2, true, localX, localY);
                    if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
                        appContext.viewport3D.handleMouseButton(2, false, localX, localY);

                    appContext.viewport3D.handleMouseMove(localX, localY);

                    float scrollY = io.MouseWheel;
                    if (scrollY != 0.0f)
                        appContext.viewport3D.handleMouseScroll(scrollY);

                    appContext.viewport3D.handleKey(340, io.KeyShift);
                    appContext.viewport3D.handleKey(341, io.KeyCtrl);
                }

                // ImGui Overlays: View Triad + ViewCube + Snap
                appContext.viewport3D.renderViewTriad(canvasX, ribbonHeight, canvasWidth, canvasHeight);
                appContext.viewport3D.renderViewCube(canvasX, ribbonHeight, canvasWidth, canvasHeight);
                appContext.viewport3D.renderSnapOverlay(canvasX, ribbonHeight);

                ImGui::End();
                ImGui::PopStyleColor();
                ImGui::PopStyleVar();

                // === ImGuizmo overlay ===
                auto tool3d = appContext.viewport3D.getActiveTool();
                auto* selectedBody = appContext.viewport3D.getScene()->getSelectedBody();
                if (selectedBody &&
                    (tool3d == Modeling3D::Tool3DType::Move ||
                     tool3d == Modeling3D::Tool3DType::Rotate ||
                     tool3d == Modeling3D::Tool3DType::Scale))
                {
                    ImGuizmo::BeginFrame();
                    ImGuizmo::SetOrthographic(false);
                    ImGuizmo::SetRect(canvasX, ribbonHeight, canvasWidth, canvasHeight);

                    float aspect = canvasWidth / canvasHeight;
                    glm::mat4 proj = glm::perspective(
                        glm::radians(appContext.viewport3D.getCamera()->Zoom),
                        aspect, 0.1f, 1000.0f);
                    glm::mat4 view = appContext.viewport3D.getCamera()->GetViewMatrix();
                    glm::mat4 modelMat = selectedBody->getTransform();
                    glm::mat4 prevMat = modelMat;

                    ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
                    if (tool3d == Modeling3D::Tool3DType::Rotate)
                        op = ImGuizmo::ROTATE;
                    else if (tool3d == Modeling3D::Tool3DType::Scale)
                        op = ImGuizmo::SCALE;

                    ImGuizmo::Manipulate(
                        glm::value_ptr(view),
                        glm::value_ptr(proj),
                        op,
                        ImGuizmo::WORLD,
                        glm::value_ptr(modelMat),
                        nullptr,
                        nullptr);

                    if (ImGuizmo::IsUsing()) {
                        selectedBody->setTransform(modelMat);
                    }

                    // Create undo command when gizmo interaction finishes
                    static bool wasUsingGizmo = false;
                    static glm::mat4 gizmoStartMatrix = glm::mat4(1.0f);
                    if (ImGuizmo::IsUsing() && !wasUsingGizmo) {
                        gizmoStartMatrix = prevMat;
                    }
                    if (!ImGuizmo::IsUsing() && wasUsingGizmo) {
                        auto cmd = std::make_unique<Modeling3D::TransformBodyCommand>(
                            selectedBody, gizmoStartMatrix, modelMat);
                        appContext.viewport3D.getScene()->getCommandManager().execute(std::move(cmd));
                    }
                    wasUsingGizmo = ImGuizmo::IsUsing();
                }
            }

            // Keyboard shortcuts for 3D mode
            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
                if (appContext.viewport3D.getScene()->undo())
                    appContext.consoleMessage = "Undo: " + appContext.viewport3D.getScene()->getCommandManager().getUndoDescription();
            }
            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) {
                if (appContext.viewport3D.getScene()->redo())
                    appContext.consoleMessage = "Redo: " + appContext.viewport3D.getScene()->getCommandManager().getRedoDescription();
            }

            // 3D Property Panel
            UI::PropertyPanel3D::Render(appContext.viewport3D, appContext);

            // Status bar (shows 3D info)
            UI::StatusBar::Render(canvas, appContext);

        } else {
            // ═══════ 2D MODE (existing) ═══════
            UI::CanvasView::HandleKeyboardShortcuts(canvas, appContext);
            UI::TopRibbon::Render(canvas, appContext);
            UI::CanvasView::Render(canvas, appContext);
            UI::PropertyPanel::Render(canvas, appContext);
            UI::StatusBar::Render(canvas, appContext);

            // Render 3D views if enabled
            if (appContext.showBellows3DView) {
              UI::Viewers3DUI::RenderBellows3DViewWindow(canvas, appContext);
            }
            
            if (appContext.showBallBearing3DView) {
              UI::Viewers3DUI::RenderBallBearing3DViewWindow(canvas, appContext);
            }
            
            if (appContext.showSpring3DView) {
                UI::Viewers3DUI::RenderSpring3DViewWindow(canvas, appContext);
            }
            if (appContext.showShockAbsorber3DView) {
                UI::Viewers3DUI::RenderShockAbsorber3DViewWindow(canvas, appContext);
            }
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
