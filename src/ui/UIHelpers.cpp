#include "ui/UIHelpers.hpp"
#include <glad/glad.h>
#ifdef HAVE_SDL2_IMAGE
#include <SDL2/SDL_image.h>
#endif
#include <iostream>
#include <unordered_map>
#include <vector>

// Icon texture storage
static std::unordered_map<std::string, GLuint> iconTextures;
static std::unordered_map<std::string, ImVec2> iconSizes;

// Helper function to handle tool selection
void SelectTool(Drawing::DrawingMode mode, Drawing::Canvas& canvas, Core::ApplicationContext& appContext, const std::string& message) {
  appContext.activeMode = mode;
  canvas.setDrawingMode(mode);
  appContext.consoleMessage = message;
}

bool IconButton(const std::string& iconName, const char* fallbackText, const char* tooltip, const ImVec2& size, bool isActive) {
  bool pressed = false;
  
  // Push active-tool styling
  if (isActive) {
    ImGui::PushStyleColor(ImGuiCol_Button, UIColors::TOOL_ACTIVE_BG);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, UIColors::ACCENT_DIM);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, UIColors::ACCENT);
  }
  
  // Try to use texture icon first
  auto textureIt = iconTextures.find(iconName);
  if (textureIt != iconTextures.end()) {
    GLuint texture = textureIt->second;
    ImVec2 iconSize = iconSizes[iconName];
    
    // Scale icon to fit button while maintaining aspect ratio
    float scale = std::min(size.x / iconSize.x, size.y / iconSize.y) * 0.8f; // 80% of button size
    ImVec2 scaledSize = ImVec2(iconSize.x * scale, iconSize.y * scale);
    
    if (!isActive) {
      ImGui::PushStyleColor(ImGuiCol_Button, UIColors::BUTTON);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, UIColors::BUTTON_HOVERED);
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, UIColors::BUTTON_ACTIVE);
    }
    
    pressed = ImGui::ImageButton(("##" + iconName).c_str(), (ImTextureID)(uintptr_t)texture, scaledSize);
    
    if (!isActive) {
      ImGui::PopStyleColor(3);
    }
  } else {
    // Fallback to text button
    pressed = ImGui::Button((std::string(fallbackText) + "##" + iconName).c_str(), size);
  }
  
  // Draw active indicator line under the button
  if (isActive) {
    ImGui::PopStyleColor(3);
    ImVec2 rMin = ImGui::GetItemRectMin();
    ImVec2 rMax = ImGui::GetItemRectMax();
    ImGui::GetWindowDrawList()->AddLine(
      ImVec2(rMin.x, rMax.y - 1),
      ImVec2(rMax.x, rMax.y - 1),
      IM_COL32(66, 150, 250, 220), 2.0f);
  }
  
  // Add tooltip
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s", tooltip);
  }
  
  return pressed;
}

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
  // Paths are relative to the project root (where the binary is launched from)
  std::vector<std::pair<std::string, std::string>> iconFiles = {
    // 2D tools
    {"line", "icons/png/line-segment.png"},
    {"circle", "icons/png/circle.png"},
    {"rectangle", "icons/png/rectangle.png"},
    {"point", "icons/png/dot.png"},
    {"triangle", "icons/png/triangle.png"},
    {"square", "icons/png/square.png"},
    {"spline", "icons/png/spline-curve.png"},
    {"bezier", "icons/png/bezier-curve.png"},
    {"select", "icons/png/select.png"},
    {"undo", "icons/png/undo.png"},
    {"redo", "icons/png/redo.png"},
    {"clear", "icons/png/clear.png"},
    {"bearing", "icons/png/bearing.png"},
    {"bellows", "icons/png/bellows.png"},
    {"suspension", "icons/png/suspension.png"},
    // 3D tools
    {"extrude",    "icons/png/extrude.png"},
    {"revolve",    "icons/png/revolve.png"},
    {"fillet3d",   "icons/png/fillet.png"},
    {"move3d",     "icons/png/move3d.png"},
    {"rotate3d",   "icons/png/rotate3d.png"},
    {"scale3d",    "icons/png/scale3d.png"},
    {"primitives", "icons/png/primitives.png"},
    {"spline3d",   "icons/png/spline3d.png"},
    {"plane3d",    "icons/png/plane3d.png"},
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

void CleanupIconTextures() {
  for (auto& pair : iconTextures) {
    glDeleteTextures(1, &pair.second);
  }
  iconTextures.clear();
  iconSizes.clear();
#ifdef HAVE_SDL2_IMAGE
  IMG_Quit();
#endif
}
