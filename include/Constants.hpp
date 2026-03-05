#pragma once

#include <imgui.h>

namespace Drawing {

// Common color definitions
namespace Colors {
    const ImU32 POINT = IM_COL32(220, 225, 240, 255);       // Crisp white for dark canvas
    const ImU32 LINE = IM_COL32(220, 225, 240, 255);        // Crisp white
    const ImU32 CIRCLE = IM_COL32(220, 225, 240, 255);      // Crisp white
    const ImU32 TRIANGLE = IM_COL32(220, 225, 240, 255);    // Crisp white
    const ImU32 SQUARE = IM_COL32(220, 225, 240, 255);      // Crisp white
    const ImU32 RECTANGLE = IM_COL32(220, 225, 240, 255);   // Crisp white
    const ImU32 SPLINE = IM_COL32(220, 225, 240, 255);      // Crisp white
    const ImU32 BEZIER = IM_COL32(220, 225, 240, 255);      // Crisp white
    const ImU32 PREVIEW = IM_COL32(80, 170, 255, 230);        // Bright blue preview
    const ImU32 PREVIEW_LIGHT = IM_COL32(110, 180, 255, 200); // Lighter blue secondary preview
    const ImU32 CONTROL_POINT = IM_COL32(60, 200, 200, 255);  // Teal control points
    const ImU32 CONTROL_LINE = IM_COL32(100, 110, 140, 160);  // Muted blue-gray control lines
    const ImU32 GRID_MINOR = IM_COL32(40, 44, 62, 55);        // Subtle cool blue-gray minor lines
    const ImU32 GRID_MAJOR = IM_COL32(52, 58, 82, 95);        // Slightly brighter major lines
    const ImU32 GRID = IM_COL32(46, 50, 70, 75);              // General grid color
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