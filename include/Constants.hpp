#pragma once

#include <imgui.h>

namespace Drawing {

// Common color definitions
namespace Colors {
    const ImU32 POINT = IM_COL32(255, 255, 255, 255);
    const ImU32 LINE = IM_COL32(255, 255, 255, 255);
    const ImU32 CIRCLE = IM_COL32(255, 255, 255, 255);
    const ImU32 TRIANGLE = IM_COL32(255, 255, 255, 255);
    const ImU32 SQUARE = IM_COL32(255, 255, 255, 255);
    const ImU32 RECTANGLE = IM_COL32(255, 255, 255, 255);
    const ImU32 SPLINE = IM_COL32(0, 255, 128, 255);  // Green
    const ImU32 BEZIER = IM_COL32(255, 128, 0, 255);  // Orange
    const ImU32 PREVIEW = IM_COL32(128, 128, 128, 128);
    const ImU32 PREVIEW_LIGHT = IM_COL32(128, 128, 128, 128);  // Light gray
    const ImU32 CONTROL_POINT = IM_COL32(255, 255, 0, 255);  // Yellow
    const ImU32 CONTROL_LINE = IM_COL32(128, 128, 128, 128);  // Gray
    const ImU32 GRID = IM_COL32(50, 50, 50, 255);
}

// Common constants
namespace Constants {
    const float MIN_ZOOM = 0.1f;
    const float MAX_ZOOM = 10.0f;
    const float ZOOM_SPEED = 1.1f;
    const float DEFAULT_POINT_SIZE = 5.0f;
    const float DEFAULT_LINE_THICKNESS = 1.0f;
    const float DEFAULT_GRID_SPACING = 20.0f;
    const float SNAP_THRESHOLD = 10.0f;        // Distance in pixels for snapping
    const float MIN_SHAPE_SIZE = 5.0f;         // Minimum size for shapes
    const int CIRCLE_SEGMENTS = 64;            // Number of segments for circle rendering
}

} // namespace Drawing 