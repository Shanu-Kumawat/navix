#pragma once

#include <imgui.h>

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
const ImVec4 ERROR_COLOR = ImVec4(0.80f, 0.20f, 0.20f, 1.0f);            // Red for errors
} // namespace UIColors
