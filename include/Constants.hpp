#pragma once

#include <imgui.h>

namespace Drawing {

// Common color definitions
namespace Colors {
    const ImU32 POINT = IM_COL32(255, 255, 255, 255);    // White (was Black)
    const ImU32 LINE = IM_COL32(255, 255, 255, 255);     // White (was Black)
    const ImU32 CIRCLE = IM_COL32(255, 255, 255, 255);   // White (was Black)
    const ImU32 TRIANGLE = IM_COL32(255, 255, 255, 255); // White (was Black)
    const ImU32 SQUARE = IM_COL32(255, 255, 255, 255);   // White (was Black)
    const ImU32 RECTANGLE = IM_COL32(255, 255, 255, 255);// White (was Black)
    const ImU32 SPLINE = IM_COL32(255, 255, 255, 255);   // White (was Black)
    const ImU32 BEZIER = IM_COL32(255, 255, 255, 255);   // White (was Black)
    const ImU32 PREVIEW = IM_COL32(150, 150, 150, 200);  // Light gray with opacity
    const ImU32 PREVIEW_LIGHT = IM_COL32(180, 180, 180, 200); // Increased opacity
    const ImU32 CONTROL_POINT = IM_COL32(255, 255, 0, 255);   // Yellow
    const ImU32 CONTROL_LINE = IM_COL32(180, 180, 180, 200);  // Lighter gray for visibility
    const ImU32 GRID_MINOR = IM_COL32(60, 60, 70, 120);       // Dark gray for minor grid lines
    const ImU32 GRID_MAJOR = IM_COL32(90, 90, 100, 160);      // Medium gray for major grid lines
    const ImU32 GRID = IM_COL32(80, 80, 90, 140);             // Updated grid color for compatibility
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

enum class UnitSystem {
    Pixels,
    Millimeters,
    Centimeters,
    Inches
};

} // namespace Drawing 