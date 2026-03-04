#pragma once

#include <imgui.h>

namespace Drawing {

// Common color definitions
namespace Colors {
    const ImU32 POINT = IM_COL32(30, 30, 30, 255);         // Dark gray
    const ImU32 LINE = IM_COL32(30, 30, 30, 255);          // Dark gray
    const ImU32 CIRCLE = IM_COL32(30, 30, 30, 255);        // Dark gray
    const ImU32 TRIANGLE = IM_COL32(30, 30, 30, 255);      // Dark gray
    const ImU32 SQUARE = IM_COL32(30, 30, 30, 255);        // Dark gray
    const ImU32 RECTANGLE = IM_COL32(30, 30, 30, 255);     // Dark gray
    const ImU32 SPLINE = IM_COL32(30, 30, 30, 255);        // Dark gray
    const ImU32 BEZIER = IM_COL32(30, 30, 30, 255);        // Dark gray
    const ImU32 PREVIEW = IM_COL32(0, 120, 255, 220);        // Bright blue for clear preview visibility
    const ImU32 PREVIEW_LIGHT = IM_COL32(80, 160, 255, 200); // Lighter blue for secondary preview
    const ImU32 CONTROL_POINT = IM_COL32(45, 175, 175, 255);  // Teal control points
    const ImU32 CONTROL_LINE = IM_COL32(100, 100, 100, 180);  // Medium gray for control lines
    const ImU32 GRID_MINOR = IM_COL32(205, 180, 140, 120);    // Light wood/beige for minor grid lines
    const ImU32 GRID_MAJOR = IM_COL32(160, 120, 80, 180);     // Medium brown for major grid lines (abacus frame)
    const ImU32 GRID = IM_COL32(180, 140, 100, 150);          // General warm brown grid color
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

enum class DrawingMode {
    None,
    Select,
    Point,
    Line,
    Circle,
    Triangle,
    Square,
    Rectangle,
    Spline,
    BezierCurve,
    Bellows,
    BallBearing,
    Spring2D
};

} // namespace Drawing 