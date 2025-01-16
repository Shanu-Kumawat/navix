#include "Canvas.hpp"
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <iostream>

// Icon definitions using simple ASCII characters
#define ICON_POINT "P"      // Point
#define ICON_LINE "L"       // Line
#define ICON_CIRCLE "C"     // Circle
#define ICON_TRIANGLE "T"   // Triangle
#define ICON_SQUARE "S"     // Square
#define ICON_RECTANGLE "R"  // Rectangle
#define ICON_SPLINE "SP"    // Spline
#define ICON_BEZIER "BZ"    // Bezier
#define ICON_SELECT "S"     // Select

// Window dimensions
const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;

// UI Colors
namespace UIColors {
    const ImVec4 BACKGROUND = ImVec4(0.15f, 0.16f, 0.17f, 1.0f);
    const ImVec4 PANEL = ImVec4(0.18f, 0.20f, 0.22f, 1.0f);
    const ImVec4 HEADER = ImVec4(0.20f, 0.22f, 0.24f, 1.0f);
    const ImVec4 BORDER = ImVec4(0.25f, 0.26f, 0.27f, 1.0f);
    const ImVec4 TEXT = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
    const ImVec4 TEXT_DIM = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    const ImVec4 BUTTON = ImVec4(0.22f, 0.24f, 0.27f, 1.0f);
    const ImVec4 BUTTON_HOVERED = ImVec4(0.30f, 0.32f, 0.35f, 1.0f);
    const ImVec4 BUTTON_ACTIVE = ImVec4(0.35f, 0.38f, 0.40f, 1.0f);
}

// Global state for active tool and settings
namespace UIState {
    static Drawing::DrawingMode activeMode = Drawing::DrawingMode::Point;
    static bool snapEnabled = true;
    static float gridSize = 10.0f;
    static std::string consoleMessage = "Ready";
}

// Helper function to handle tool selection
void SelectTool(Drawing::DrawingMode mode, Drawing::Canvas& canvas, const std::string& message) {
    UIState::activeMode = mode;
    canvas.setDrawingMode(mode);
    UIState::consoleMessage = message;
}

void SetupImGuiStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Colors
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = UIColors::TEXT;
    colors[ImGuiCol_TextDisabled] = UIColors::TEXT_DIM;
    colors[ImGuiCol_WindowBg] = UIColors::BACKGROUND;
    colors[ImGuiCol_ChildBg] = UIColors::PANEL;
    colors[ImGuiCol_PopupBg] = UIColors::PANEL;
    colors[ImGuiCol_Border] = UIColors::BORDER;
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = UIColors::PANEL;
    colors[ImGuiCol_FrameBgHovered] = UIColors::BUTTON_HOVERED;
    colors[ImGuiCol_FrameBgActive] = UIColors::BUTTON_ACTIVE;
    colors[ImGuiCol_TitleBg] = UIColors::HEADER;
    colors[ImGuiCol_TitleBgActive] = UIColors::HEADER;
    colors[ImGuiCol_TitleBgCollapsed] = UIColors::HEADER;
    colors[ImGuiCol_MenuBarBg] = UIColors::HEADER;
    colors[ImGuiCol_ScrollbarBg] = UIColors::PANEL;
    colors[ImGuiCol_ScrollbarGrab] = UIColors::BUTTON;
    colors[ImGuiCol_ScrollbarGrabHovered] = UIColors::BUTTON_HOVERED;
    colors[ImGuiCol_ScrollbarGrabActive] = UIColors::BUTTON_ACTIVE;
    colors[ImGuiCol_Button] = UIColors::BUTTON;
    colors[ImGuiCol_ButtonHovered] = UIColors::BUTTON_HOVERED;
    colors[ImGuiCol_ButtonActive] = UIColors::BUTTON_ACTIVE;
    colors[ImGuiCol_Header] = UIColors::HEADER;
    colors[ImGuiCol_HeaderHovered] = UIColors::BUTTON_HOVERED;
    colors[ImGuiCol_HeaderActive] = UIColors::BUTTON_ACTIVE;
    colors[ImGuiCol_Separator] = UIColors::BORDER;
    colors[ImGuiCol_Tab] = UIColors::BUTTON;
    colors[ImGuiCol_TabHovered] = UIColors::BUTTON_HOVERED;
    colors[ImGuiCol_TabActive] = UIColors::BUTTON_ACTIVE;
    
    // Styles
    style.WindowPadding = ImVec2(8, 8);
    style.FramePadding = ImVec2(4, 3);
    style.CellPadding = ImVec2(4, 2);
    style.ItemSpacing = ImVec2(8, 4);
    style.ItemInnerSpacing = ImVec2(4, 4);
    style.TouchExtraPadding = ImVec2(0, 0);
    style.IndentSpacing = 21;
    style.ScrollbarSize = 14;
    style.GrabMinSize = 10;
    
    style.WindowBorderSize = 1;
    style.ChildBorderSize = 1;
    style.PopupBorderSize = 1;
    style.FrameBorderSize = 1;
    style.TabBorderSize = 0;
    
    style.WindowRounding = 0;
    style.ChildRounding = 0;
    style.FrameRounding = 2;
    style.PopupRounding = 0;
    style.ScrollbarRounding = 2;
    style.GrabRounding = 2;
    style.TabRounding = 2;
}

void RenderToolPanel(Drawing::Canvas& canvas) {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(250, ImGui::GetIO().DisplaySize.y - 150));
    
    ImGui::Begin("Tools", nullptr, 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoCollapse
    );
    
    // Select mode
    if (ImGui::Button(ICON_SELECT "##select", ImVec2(30, 30))) {
        SelectTool(Drawing::DrawingMode::Select, canvas, "Select Mode");
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select (S)");
    
    if (ImGui::CollapsingHeader("Drawing Tools", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        
        // Add Clear button at the top
        if (ImGui::Button("Clear All", ImVec2(ImGui::GetContentRegionAvail().x, 30))) {
            canvas.clearAll();
            UIState::consoleMessage = "All shapes cleared";
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Style for selected tool
        ImGui::PushStyleColor(ImGuiCol_Button, UIColors::BUTTON_ACTIVE);
        
        float buttonSize = 32.0f;
        float windowWidth = ImGui::GetContentRegionAvail().x;
        float padding = 8.0f;
        float columnWidth = (windowWidth - 3 * padding) / 2;
        
        for (int i = 0; i < 6; i++) {
            if (i % 2 != 0) ImGui::SameLine();
            
            bool isActive = false;
            const char* icon = "";
            const char* tooltip = "";
            Drawing::DrawingMode mode;
            
            switch (i) {
                case 0:
                    icon = ICON_POINT;
                    tooltip = "Point Tool";
                    mode = Drawing::DrawingMode::Point;
                    isActive = UIState::activeMode == mode;
                    break;
                case 1:
                    icon = ICON_LINE;
                    tooltip = "Line Tool";
                    mode = Drawing::DrawingMode::Line;
                    isActive = UIState::activeMode == mode;
                    break;
                case 2:
                    icon = ICON_CIRCLE;
                    tooltip = "Circle Tool";
                    mode = Drawing::DrawingMode::Circle;
                    isActive = UIState::activeMode == mode;
                    break;
                case 3:
                    icon = ICON_TRIANGLE;
                    tooltip = "Triangle Tool";
                    mode = Drawing::DrawingMode::Triangle;
                    isActive = UIState::activeMode == mode;
                    break;
                case 4:
                    icon = ICON_SQUARE;
                    tooltip = "Square Tool";
                    mode = Drawing::DrawingMode::Square;
                    isActive = UIState::activeMode == mode;
                    break;
                case 5:
                    icon = ICON_RECTANGLE;
                    tooltip = "Rectangle Tool";
                    mode = Drawing::DrawingMode::Rectangle;
                    isActive = UIState::activeMode == mode;
                    break;
            }
            
            if (isActive) {
                ImGui::PushStyleColor(ImGuiCol_Button, UIColors::BUTTON_ACTIVE);
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, UIColors::BUTTON);
            }
            
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));
            if (ImGui::Button(icon, ImVec2(columnWidth, buttonSize))) {
                SelectTool(mode, canvas, tooltip);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", tooltip);
            }
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
            
            if (i % 2 == 1) {
                ImGui::Spacing();
            }
        }
        
        // Add Spline and Bezier buttons
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Text("Curves");
        ImGui::Spacing();
        
        // Spline button
        bool isSplineActive = UIState::activeMode == Drawing::DrawingMode::Spline;
        if (isSplineActive) {
            ImGui::PushStyleColor(ImGuiCol_Button, UIColors::BUTTON_ACTIVE);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, UIColors::BUTTON);
        }
        if (ImGui::Button(ICON_SPLINE, ImVec2(columnWidth, buttonSize))) {
            SelectTool(Drawing::DrawingMode::Spline, canvas, "Click to add control points, right-click to finish spline");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Spline Tool");
        }
        ImGui::PopStyleColor();
        
        // Bezier button
        ImGui::SameLine();
        bool isBezierActive = UIState::activeMode == Drawing::DrawingMode::BezierCurve;
        if (isBezierActive) {
            ImGui::PushStyleColor(ImGuiCol_Button, UIColors::BUTTON_ACTIVE);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, UIColors::BUTTON);
        }
        if (ImGui::Button(ICON_BEZIER, ImVec2(columnWidth, buttonSize))) {
            SelectTool(Drawing::DrawingMode::BezierCurve, canvas, "Click to place 4 control points for Bezier curve");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Bezier Curve Tool");
        }
        ImGui::PopStyleColor();
        
        ImGui::PopStyleColor();
        ImGui::Unindent();
    }
    
    // Curve controls
    if (ImGui::CollapsingHeader("Curve Controls")) {
        ImGui::Indent();
        
        // Spline closure toggle
        if (ImGui::Button("Toggle Spline Closure")) {
            canvas.toggleSplineClosure();
            UIState::consoleMessage = canvas.isSplineClosed() ? "Spline mode: Closed" : "Spline mode: Open";
        }
        ImGui::SameLine();
        ImGui::Text(canvas.isSplineClosed() ? "(Closed)" : "(Open)");
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Bezier curve truncation
        ImGui::Text("Bezier Truncation");
        auto [startT, endT] = canvas.getTruncationPoints();
        float start = startT;
        float end = endT;
        if (ImGui::SliderFloat("Start", &start, 0.0f, 1.0f) ||
            ImGui::SliderFloat("End", &end, 0.0f, 1.0f)) {
            canvas.setTruncationPoints(start, end);
            UIState::consoleMessage = "Bezier curve truncation updated";
        }
        
        ImGui::Unindent();
    }
    
    ImGui::End();
}

void RenderConsole() {
    ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetIO().DisplaySize.y - 150));
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, 150));
    
    ImGui::Begin("Console", nullptr, 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoCollapse
    );
    
    ImGui::TextColored(UIColors::TEXT_DIM, "%s", UIState::consoleMessage.c_str());
    
    ImGui::End();
}

void RenderCanvas(Drawing::Canvas& canvas) {
    ImGui::SetNextWindowPos(ImVec2(250, 0));
    ImGui::SetNextWindowSize(ImVec2(
        ImGui::GetIO().DisplaySize.x - 250,
        ImGui::GetIO().DisplaySize.y - 150
    ));
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Canvas", nullptr, 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar
    );
    
    // Get the canvas window position for proper coordinate transformation
    ImVec2 canvasPos = ImGui::GetWindowPos();
    ImVec2 canvasSize = ImGui::GetWindowSize();
    
    // Update canvas with window information for proper coordinate transformation
    canvas.setWindowInfo(canvasPos.x, canvasPos.y, canvasSize.x, canvasSize.y);
    
    canvas.handleInput();
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    canvas.render(drawList);
    
    ImGui::End();
    ImGui::PopStyleVar();
}

void HandleKeyboardShortcuts(Drawing::Canvas& canvas) {
    if (ImGui::IsKeyPressed(ImGuiKey_S)) {
        SelectTool(Drawing::DrawingMode::Select, canvas, "Select Mode");
    }
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "Error: " << SDL_GetError() << std::endl;
    return -1;
  }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_Window* window = SDL_CreateWindow(
        "Drawing Application",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );

  if (!window) {
        std::cerr << "Error: " << SDL_GetError() << std::endl;
    return -1;
  }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

  if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
    std::cerr << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 130");

    SetupImGuiStyle();

    Drawing::Canvas canvas;
    bool running = true;

    // Initialize canvas settings
    canvas.setSnapToGrid(UIState::snapEnabled);
    canvas.setGridSpacing(UIState::gridSize);

    while (running) {
  SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      if (event.type == SDL_QUIT)
                running = false;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                running = false;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

        RenderToolPanel(canvas);
        RenderCanvas(canvas);
        RenderConsole();

    ImGui::Render();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(
            UIColors::BACKGROUND.x,
            UIColors::BACKGROUND.y,
            UIColors::BACKGROUND.z,
            UIColors::BACKGROUND.w
        );
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
