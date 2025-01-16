#pragma once

#include <imgui.h>
#include <vector>
#include <array>
#include <cmath>
#include <memory>
#include "Constants.hpp"
#include "shapes/BasicShapes.hpp"
#include "utils/MathUtils.hpp"
#include "utils/VectorMath.hpp"

namespace Drawing {

struct Spline : public Shape {
    std::vector<ImVec2> controlPoints;
    bool isClosed{false};
    float tension{0.5f};     // Controls curve tightness (0 = linear, 1 = very curved)
    bool showControlPoints{true};
    int selectedPoint{-1};   // Index of selected control point for editing
    bool isSelected{false};  // Whether the entire spline is selected

    // New features
    float smoothness{1.0f};        // Controls how smooth the curve is (0 = sharp, 1 = smooth)
    bool autoSmooth{false};        // Automatically smooth the curve when points are moved
    bool uniformParametrization{true}; // Use uniform or chord-length parameterization
    std::vector<float> weights;    // Weight for each control point (for NURBS-like behavior)
    bool showTangents{false};      // Show tangent vectors at control points
    bool showCurvature{false};     // Show curvature visualization
    float adaptiveResolution{0.01f}; // For adaptive sampling based on curvature

    Spline(const std::vector<ImVec2>& points = {}, ImU32 color = Colors::SPLINE, float thickness = Constants::DEFAULT_LINE_THICKNESS)
        : Shape(ShapeType::SPLINE, color, thickness), controlPoints(points) {}

    bool isValid() const override {
        return controlPoints.size() >= 2;
    }

    std::unique_ptr<Shape> clone() const override {
        return std::make_unique<Spline>(*this);
    }

    // Helper methods
    std::vector<ImVec2> calculatePoints(float resolution = 0.1f) const;
    ImVec2 calculatePoint(float t) const;
    ImVec2 calculateDerivative(float t) const;
    ImVec2 calculateSecondDerivative(float t) const;
    ImVec2 catmullRomPoint(const ImVec2& p0, const ImVec2& p1, 
                          const ImVec2& p2, const ImVec2& p3, float t) const;
    void addControlPoint(const ImVec2& point);
    void removeControlPoint(size_t index);
    void moveControlPoint(size_t index, const ImVec2& newPos);
    void moveEntireSpline(const ImVec2& delta);
    void smoothen(float factor = 0.5f);  // Smooths the curve by adjusting control points
    void subdivide(float threshold = 10.0f);  // Adds more control points for finer control
    int findNearestControlPoint(const ImVec2& point, float threshold) const;
    bool isPointNear(const ImVec2& point, float threshold) const override;

    // New methods
    void reverseDirection();       // Reverse the direction of the curve
    void makeUniform();           // Distribute control points uniformly
    void adjustTension(float t);  // Adjust curve tension globally
    void optimizeControlPoints(); // Reduce number of control points while maintaining shape
    float calculateLength() const; // Calculate approximate curve length
    float calculateCurvature(float t) const; // Calculate curvature at parameter t
    std::vector<ImVec2> getTangents() const; // Get tangent vectors at control points
    void insertKnot(float t);     // Insert a new control point at parameter t
    void removeKnot(size_t index, float tolerance); // Remove control point while maintaining shape
    void fitToCurve(const std::vector<ImVec2>& points); // Fit spline to a set of points
};

class BezierCurve : public Shape {
public:
    std::vector<ImVec2> controlPoints;
    bool showControlPoints{true};
    bool showBoundingBox{false};
    bool showExtremities{false};
    bool showInflections{false};
    bool adaptiveRendering{true};
    float flatnessTolerance{0.1f};
    std::vector<float> weights;
    bool isSymmetrical{true};
    bool isSelected{false};
    int selectedPoint{-1};

    BezierCurve(const std::vector<ImVec2>& points = {}, ImU32 color = Colors::BEZIER, float thickness = Constants::DEFAULT_LINE_THICKNESS)
        : Shape(ShapeType::BEZIER, color, thickness), controlPoints(points) {}
    
    bool isValid() const override {
        return controlPoints.size() == 4;
    }

    std::unique_ptr<Shape> clone() const override {
        return std::make_unique<BezierCurve>(*this);
    }

    // Core functionality
    void decompose();
    void approximate(float tolerance);
    float calculateLength() const;
    
    // Analysis methods
    std::vector<ImVec2> findExtrema() const;
    std::vector<ImVec2> findInflections() const;
    ImVec2 findNearestPoint(const ImVec2& point) const;
    
    // Curve manipulation
    void generateOffsetCurve(float distance);
    void fitToCurve(const std::vector<ImVec2>& points);
    void moveEntireCurve(const ImVec2& delta);
    void moveControlPoint(size_t index, const ImVec2& newPos);
    void adjustSymmetrically(size_t index, const ImVec2& newPos);
    void splitCurve(float t);
    void elevateOrder();
    void reduceOrder();
    int findNearestControlPoint(const ImVec2& point, float threshold) const;
    bool isPointNear(const ImVec2& point, float threshold) const override;
    
    // Helper methods
    ImVec2 calculatePoint(float t) const;
    ImVec2 calculateDerivative(float t) const;
    float calculateCurvature(float t) const;
    std::vector<ImVec2> calculatePoints(float step = 0.01f) const;
};

// Enhanced UI helper functions
namespace CurveUI {
    void drawControlPoint(ImDrawList* drawList, const ImVec2& pos, bool isSelected, float size = 5.0f);
    void drawControlPolygon(ImDrawList* drawList, const std::vector<ImVec2>& points, bool isSelected);
    void drawTangentHandles(ImDrawList* drawList, const ImVec2& point, const ImVec2& tangent, bool isSelected);
    void drawCurveManipulator(ImDrawList* drawList, const ImVec2& pos, float size, bool isSelected);
    void drawTangentVector(ImDrawList* drawList, const ImVec2& point, const ImVec2& tangent, 
                          bool isSelected, float scale = 1.0f);
    void drawCurvature(ImDrawList* drawList, const ImVec2& point, float curvature, 
                      float scale = 1.0f);
    void drawExtremityPoint(ImDrawList* drawList, const ImVec2& pos, bool isSelected);
    void drawInflectionPoint(ImDrawList* drawList, const ImVec2& pos, bool isSelected);
    void drawBoundingBox(ImDrawList* drawList, const std::vector<ImVec2>& points, bool isSelected);
    void drawHodograph(ImDrawList* drawList, const std::vector<ImVec2>& velocities, bool isSelected);
    void drawWeightIndicator(ImDrawList* drawList, const ImVec2& pos, float weight, bool isSelected);
    void drawAdaptiveSampling(ImDrawList* drawList, const std::vector<ImVec2>& points, 
                             const std::vector<float>& curvatures);
    void drawOffsetCurve(ImDrawList* drawList, const std::vector<ImVec2>& points, 
                        float offset, bool isSelected);
}

} // namespace Drawing 