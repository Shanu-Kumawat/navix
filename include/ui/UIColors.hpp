#pragma once

#include <imgui.h>

namespace UIColors {
// === NAVIX Professional Dark Theme ===

// Core backgrounds — lighter panels for strong contrast against dark canvas
const ImVec4 BACKGROUND       = ImVec4(0.18f, 0.18f, 0.21f, 1.0f);  // Main clear color
const ImVec4 PANEL            = ImVec4(0.22f, 0.22f, 0.26f, 1.0f);  // Side panels
const ImVec4 TOOLBAR          = ImVec4(0.24f, 0.24f, 0.28f, 1.0f);  // Top toolbar
const ImVec4 DARK_PANEL       = ImVec4(0.19f, 0.19f, 0.23f, 1.0f);  // Status bar / darker regions

// Accent
const ImVec4 ACCENT           = ImVec4(0.26f, 0.59f, 0.98f, 1.0f);  // Primary blue
const ImVec4 ACCENT_HOVER     = ImVec4(0.36f, 0.66f, 1.00f, 1.0f);  // Lighter hover
const ImVec4 ACCENT_DIM       = ImVec4(0.20f, 0.40f, 0.70f, 1.0f);  // Subdued accent

// Text
const ImVec4 TEXT             = ImVec4(0.86f, 0.86f, 0.89f, 1.0f);
const ImVec4 TEXT_DIM         = ImVec4(0.50f, 0.50f, 0.56f, 1.0f);
const ImVec4 TEXT_BRIGHT      = ImVec4(1.00f, 1.00f, 1.00f, 1.0f);

// Borders
const ImVec4 BORDER           = ImVec4(0.30f, 0.30f, 0.35f, 1.0f);
const ImVec4 BORDER_HIGHLIGHT = ImVec4(0.40f, 0.40f, 0.46f, 1.0f);

// Buttons — clearly raised, high contrast against lighter panels
const ImVec4 BUTTON           = ImVec4(0.34f, 0.36f, 0.44f, 1.0f);
const ImVec4 BUTTON_HOVERED   = ImVec4(0.42f, 0.44f, 0.54f, 1.0f);
const ImVec4 BUTTON_ACTIVE    = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
const ImVec4 BUTTON_TEXT      = ImVec4(0.95f, 0.96f, 0.98f, 1.0f);

// Headers
const ImVec4 HEADER           = ImVec4(0.28f, 0.30f, 0.38f, 1.0f);
const ImVec4 HEADER_HOVERED   = ImVec4(0.34f, 0.37f, 0.47f, 1.0f);
const ImVec4 TAB_ACTIVE       = ImVec4(0.22f, 0.22f, 0.26f, 1.0f);

// Canvas — distinctly deeper blue-black, clearly separate from panels
const ImVec4 GRID_BACKGROUND  = ImVec4(0.078f, 0.086f, 0.125f, 1.0f);

// Status indicators
const ImVec4 SUCCESS          = ImVec4(0.31f, 0.79f, 0.69f, 1.0f);
const ImVec4 WARNING          = ImVec4(0.91f, 0.73f, 0.27f, 1.0f);
const ImVec4 ERROR_COLOR      = ImVec4(0.96f, 0.31f, 0.31f, 1.0f);

// Misc
const ImVec4 COMMAND_BG       = ImVec4(0.18f, 0.18f, 0.22f, 1.0f);
const ImVec4 COMMAND_TEXT     = ImVec4(0.88f, 0.88f, 0.91f, 1.0f);

// Active tool indicator
const ImVec4 TOOL_ACTIVE_BG     = ImVec4(0.26f, 0.59f, 0.98f, 0.18f);
const ImVec4 TOOL_ACTIVE_BORDER = ImVec4(0.26f, 0.59f, 0.98f, 0.85f);

} // namespace UIColors
