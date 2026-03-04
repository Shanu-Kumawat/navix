#pragma once

#include <string>
#include <imgui.h>
#include "Canvas.hpp"
#include "ApplicationContext.hpp"
#include "ui/UIColors.hpp"

// Fallback text icons if texture loading fails
inline const char* FALLBACK_LINE = "-";
inline const char* FALLBACK_CIRCLE = "O";
inline const char* FALLBACK_RECTANGLE = "[]";
inline const char* FALLBACK_POINT = "•";
inline const char* FALLBACK_TRIANGLE = "△";
inline const char* FALLBACK_SQUARE = "□";
inline const char* FALLBACK_SPLINE = "~";
inline const char* FALLBACK_BEZIER = "S";
inline const char* FALLBACK_SELECT = "→";
inline const char* FALLBACK_UNDO = "↶";
inline const char* FALLBACK_REDO = "↷";
inline const char* FALLBACK_CLEAR = "×";
inline const char* FALLBACK_BELLOWS = "B";
inline const char* FALLBACK_BEARING = "⚙";
inline const char* FALLBACK_SUSPENSION = "S";

bool IconButton(const std::string& iconName, const char* fallbackText, const char* tooltip, const ImVec2& size);
void SelectTool(Drawing::DrawingMode mode, Drawing::Canvas& canvas, Core::ApplicationContext& appContext, const std::string& message);

void LoadIconTextures();
void CleanupIconTextures();
