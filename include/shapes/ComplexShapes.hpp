#include <glm/glm.hpp>
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
    std::vector<glm::dvec2> controlPoints;
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

    Spline(const std::vector<glm::dvec2>& points = {}, ImU32 color = Colors::SPLINE, float thickness = Constants::DEFAULT_LINE_THICKNESS)
        : Shape(ShapeType::SPLINE, color, thickness), controlPoints(points) {}

    bool isValid() const override {
        return controlPoints.size() >= 2;
    }

    std::unique_ptr<Shape> clone() const override {
        return std::make_unique<Spline>(*this);
    }

    // Helper methods
    std::vector<glm::dvec2> calculatePoints(float resolution = 0.1f) const;
    glm::dvec2 calculatePoint(float t) const;
    glm::dvec2 calculateDerivative(float t) const;
    glm::dvec2 calculateSecondDerivative(float t) const;
    glm::dvec2 catmullRomPoint(const glm::dvec2& p0, const glm::dvec2& p1, 
                          const glm::dvec2& p2, const glm::dvec2& p3, float t) const;
    void addControlPoint(const glm::dvec2& point);
    void removeControlPoint(size_t index);
    void moveControlPoint(size_t index, const glm::dvec2& newPos);
    void moveEntireSpline(const glm::dvec2& delta);
    void smoothen(float factor = 0.5f);  // Smooths the curve by adjusting control points
    void subdivide(float threshold = 10.0f);  // Adds more control points for finer control
    int findNearestControlPoint(const glm::dvec2& point, float threshold) const;
    bool isPointNear(const glm::dvec2& point, float threshold) const override;

    // New methods
    void reverseDirection();       // Reverse the direction of the curve
    void makeUniform();           // Distribute control points uniformly
    void adjustTension(float t);  // Adjust curve tension globally
    void optimizeControlPoints(); // Reduce number of control points while maintaining shape
    float calculateLength() const; // Calculate approximate curve length
    float calculateCurvature(float t) const; // Calculate curvature at parameter t
    std::vector<glm::dvec2> getTangents() const; // Get tangent vectors at control points
    void insertKnot(float t);     // Insert a new control point at parameter t
    void removeKnot(size_t index, float tolerance); // Remove control point while maintaining shape
    void fitToCurve(const std::vector<glm::dvec2>& points); // Fit spline to a set of points

    void getBounds(glm::dvec2& min, glm::dvec2& max) const override {
        if (controlPoints.empty()) {
            min = max = glm::dvec2(0, 0);
            return;
        }
        
        min = max = controlPoints[0];
        for (const auto& point : controlPoints) {
            min.x = std::min(min.x, point.x);
            min.y = std::min(min.y, point.y);
            max.x = std::max(max.x, point.x);
            max.y = std::max(max.y, point.y);
        }
    }
};

class BezierCurve : public Shape {
public:
    std::vector<glm::dvec2> controlPoints;
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

    BezierCurve(const std::vector<glm::dvec2>& points = {}, ImU32 color = Colors::BEZIER, float thickness = Constants::DEFAULT_LINE_THICKNESS)
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
    std::vector<glm::dvec2> findExtrema() const;
    std::vector<glm::dvec2> findInflections() const;
    glm::dvec2 findNearestPoint(const glm::dvec2& point) const;
    
    // Curve manipulation
    void generateOffsetCurve(float distance);
    void fitToCurve(const std::vector<glm::dvec2>& points);
    void moveEntireCurve(const glm::dvec2& delta);
    void moveControlPoint(size_t index, const glm::dvec2& newPos);
    void adjustSymmetrically(size_t index, const glm::dvec2& newPos);
    void splitCurve(float t);
    void elevateOrder();
    void reduceOrder();
    int findNearestControlPoint(const glm::dvec2& point, float threshold) const;
    bool isPointNear(const glm::dvec2& point, float threshold) const override;
    
    // Helper methods
    glm::dvec2 calculatePoint(float t) const;
    glm::dvec2 calculateDerivative(float t) const;
    float calculateCurvature(float t) const;
    std::vector<glm::dvec2> calculatePoints(float step = 0.01f) const;

    void getBounds(glm::dvec2& min, glm::dvec2& max) const override {
        if (controlPoints.empty()) {
            min = max = glm::dvec2(0, 0);
            return;
        }
        
        min = max = controlPoints[0];
        for (const auto& point : controlPoints) {
            min.x = std::min(min.x, point.x);
            min.y = std::min(min.y, point.y);
            max.x = std::max(max.x, point.x);
            max.y = std::max(max.y, point.y);
        }
    }
};

// Enhanced UI helper functions
namespace CurveUI {
    void drawControlPoint(ImDrawList* drawList, const ImVec2& pos, bool isSelected, float size = 5.0f);
    void drawControlPolygon(ImDrawList* drawList, const std::vector<ImVec2>& points, bool isSelected);
    void drawTangentHandles(ImDrawList* drawList, const ImVec2& point, const ImVec2& tangent, bool isSelected);
    void drawCurveManipulator(ImDrawList* drawList, const glm::dvec2& pos, float size, bool isSelected);
    void drawTangentVector(ImDrawList* drawList, const glm::dvec2& point, const glm::dvec2& tangent, 
                          bool isSelected, float scale = 1.0f);
    void drawCurvature(ImDrawList* drawList, const glm::dvec2& point, float curvature, 
                      float scale = 1.0f);
    void drawExtremityPoint(ImDrawList* drawList, const glm::dvec2& pos, bool isSelected);
    void drawInflectionPoint(ImDrawList* drawList, const glm::dvec2& pos, bool isSelected);
    void drawBoundingBox(ImDrawList* drawList, const std::vector<glm::dvec2>& points, bool isSelected);
    void drawHodograph(ImDrawList* drawList, const std::vector<glm::dvec2>& velocities, bool isSelected);
    void drawWeightIndicator(ImDrawList* drawList, const glm::dvec2& pos, float weight, bool isSelected);
    void drawAdaptiveSampling(ImDrawList* drawList, const std::vector<glm::dvec2>& points, 
                             const std::vector<float>& curvatures);
    void drawOffsetCurve(ImDrawList* drawList, const std::vector<glm::dvec2>& points, 
                        float offset, bool isSelected);
}

// Bellows design class for generating parameterized bellows profiles
class Bellows : public Shape {
public:
    // Bellows parameters
    float cuffAInnerDiameter = 50.0f;     // Inner diameter of cuff A (mm)
    float cuffBInnerDiameter = 50.0f;     // Inner diameter of cuff B (mm)
    float cuffALength = 20.0f;            // Length of cuff A (mm)
    float cuffBLength = 20.0f;            // Length of cuff B (mm)
    float baseConvolutionDiameter = 60.0f; // Base/valley diameter of convolutions (mm)
    float peakConvolutionDiameter = 80.0f; // Peak diameter of convolutions (mm)
    float convolutedSectionLength = 100.0f; // Length of convoluted section (mm)
    int numConvolutions = 6;              // Number of convolutions
    float wallThickness = 2.0f;           // Wall thickness (mm)
    bool showDimensions = true;           // Show dimension lines
    bool isSelected = false;              // Selection state
    
    // Position and orientation
    glm::dvec2 position = glm::dvec2(0, 0);       // Bellows origin position
    float angle = 0.0f;                   // Bellows rotation angle

    // Constructor
    Bellows(ImU32 color = Colors::LINE, float thickness = Constants::DEFAULT_LINE_THICKNESS)
        : Shape(ShapeType::BELLOWS, color, thickness) {}

    // Static method for template-based shape finding
    static ShapeType GetShapeType() { return ShapeType::BELLOWS; }

    // Shape interface implementation
    bool isValid() const override {
        return numConvolutions > 0 && 
               cuffALength > 0.0f && 
               cuffBLength > 0.0f &&
               convolutedSectionLength > 0.0f;
    }

    std::unique_ptr<Shape> clone() const override {
        return std::make_unique<Bellows>(*this);
    }

    bool isPointNear(const glm::dvec2& point, float threshold) const override;
    
    // Generate bellows profile points
    std::vector<glm::dvec2> generateProfile() const;
    
    // Generate dimension lines and labels
    std::vector<std::pair<glm::dvec2, glm::dvec2>> generateDimensionLines() const;
    
    // Calculate overall length
    float calculateOverallLength() const {
        return cuffALength + convolutedSectionLength + cuffBLength;
    }
    
    // Update convoluted section length based on overall length
    void updateFromOverallLength(float overallLength) {
        convolutedSectionLength = overallLength - cuffALength - cuffBLength;
        if (convolutedSectionLength < 0.0f) convolutedSectionLength = 0.0f;
    }
    
    // Validate parameter constraints
    bool validateParameters() const {
        // Check for positive values
        if (cuffAInnerDiameter <= 0.0f || cuffBInnerDiameter <= 0.0f ||
            cuffALength <= 0.0f || cuffBLength <= 0.0f ||
            baseConvolutionDiameter <= 0.0f || peakConvolutionDiameter <= 0.0f ||
            wallThickness <= 0.0f || numConvolutions <= 0) {
            return false;
        }
        
        // Check diameter relationships
        if (cuffAInnerDiameter > baseConvolutionDiameter ||
            cuffBInnerDiameter > baseConvolutionDiameter ||
            baseConvolutionDiameter > peakConvolutionDiameter) {
            return false;
        }
        
        // Check reasonable wall thickness (arbitrary limits)
        if (wallThickness > baseConvolutionDiameter / 4.0f ||
            wallThickness > (peakConvolutionDiameter - baseConvolutionDiameter) / 2.0f) {
            return false;
        }
        
        return true;
    }
    
    // Calculate bounding box for fit-to-view functionality
    ImVec4 calculateBoundingBox() const {
        // Get cached profile points to avoid expensive regeneration
        const std::vector<glm::dvec2>& profile = getCachedProfile();
        
        // Find min/max coordinates
        double minX = profile[0].x;
        double minY = profile[0].y;
        double maxX = profile[0].x;
        double maxY = profile[0].y;
        
        for (const auto& point : profile) {
            minX = std::min(minX, point.x);
            minY = std::min(minY, point.y);
            maxX = std::max(maxX, point.x);
            maxY = std::max(maxY, point.y);
        }
        
        // Add margin for dimensions
        minX -= peakConvolutionDiameter / 2.0f;
        minY -= peakConvolutionDiameter / 2.0f;
        maxX += peakConvolutionDiameter / 2.0f;
        maxY += peakConvolutionDiameter / 2.0f;
        
        return ImVec4(minX, minY, maxX, maxY);
    }
    
    // Reset parameters to defaults
    void resetParameters() {
        cuffAInnerDiameter = 50.0f;
        cuffBInnerDiameter = 50.0f;
        cuffALength = 20.0f;
        cuffBLength = 20.0f;
        baseConvolutionDiameter = 60.0f;
        peakConvolutionDiameter = 80.0f;
        convolutedSectionLength = 100.0f;
        numConvolutions = 6;
        wallThickness = 2.0f;
    }

    void getBounds(glm::dvec2& min, glm::dvec2& max) const override {
        // Get cached profile points to avoid expensive regeneration
        const std::vector<glm::dvec2>& profile = getCachedProfile();
        
        if (profile.empty()) {
            min = max = glm::dvec2(0, 0);
            return;
        }
        
        // Calculate bounds based on profile points
        min = max = profile[0];
        for (const auto& point : profile) {
            min.x = std::min(min.x, point.x);
            min.y = std::min(min.y, point.y);
            max.x = std::max(max.x, point.x);
            max.y = std::max(max.y, point.y);
        }
        
        // Add margin for dimensions if they are shown
        if (showDimensions) {
            float margin = peakConvolutionDiameter / 2.0f;
            min.x -= margin;
            min.y -= margin;
            max.x += margin;
            max.y += margin;
        }
    }

    // Get cached profile with efficient regeneration only when needed
    const std::vector<glm::dvec2>& getCachedProfile() const;
    
    // Force profile regeneration (call after parameter changes)
    void invalidateCache() const { profileCached = false; }

private:
    // Profile caching to prevent expensive regeneration every frame
    mutable std::vector<glm::dvec2> cachedProfile;
    mutable bool profileCached = false;
    
    // Cache validation - stores parameters used for last cached profile
    mutable float cachedCuffAInnerDiameter = -1.0f;
    mutable float cachedCuffBInnerDiameter = -1.0f;
    mutable float cachedCuffALength = -1.0f;
    mutable float cachedCuffBLength = -1.0f;
    mutable float cachedBaseConvolutionDiameter = -1.0f;
    mutable float cachedPeakConvolutionDiameter = -1.0f;
    mutable float cachedConvolutedSectionLength = -1.0f;
    mutable int cachedNumConvolutions = -1;
    mutable float cachedWallThickness = -1.0f;
    mutable glm::dvec2 cachedPosition = glm::dvec2(-9999, -9999);
    mutable float cachedAngle = -9999.0f;
    
    // Check if cache is still valid
    bool isCacheValid() const;
};

// Ball Bearing design class for generating parameterized ball bearing profiles
class BallBearing : public Shape {
public:
    // Ball bearing parameters
    float outerDiameter = 100.0f;        // Outer race diameter (mm)
    float innerDiameter = 50.0f;         // Inner race diameter (mm) 
    float width = 20.0f;                 // Bearing width (mm)
    float ballDiameter = 12.0f;          // Ball diameter (mm)
    int numBalls = 8;                    // Number of balls
    float raceRadius = 2.0f;             // Race groove radius (mm)
    float contactAngle = 0.0f;           // Contact angle (degrees)
    bool showBalls = true;               // Show individual balls
    bool showCage = true;                // Show ball cage/separator
    bool showDimensions = true;          // Show dimension lines
    bool isSelected = false;             // Selection state
    
    // Position and orientation
    glm::dvec2 position = glm::dvec2(0, 0);      // Bearing origin position
    float angle = 0.0f;                  // Bearing rotation angle

    // Constructor
    BallBearing(ImU32 color = Colors::LINE, float thickness = Constants::DEFAULT_LINE_THICKNESS)
        : Shape(ShapeType::BALL_BEARING, color, thickness) {}

    // Static method for template-based shape finding
    static ShapeType GetShapeType() { return ShapeType::BALL_BEARING; }

    // Shape interface implementation
    bool isValid() const override {
        return outerDiameter > innerDiameter && 
               innerDiameter > 0.0f &&
               width > 0.0f &&
               ballDiameter > 0.0f &&
               numBalls > 0 &&
               ballDiameter < (outerDiameter - innerDiameter) / 2.0f;
    }

    std::unique_ptr<Shape> clone() const override {
        return std::make_unique<BallBearing>(*this);
    }

    bool isPointNear(const glm::dvec2& point, float threshold) const override;
    
    // Required Shape interface implementation
    void getBounds(glm::dvec2& min, glm::dvec2& max) const override {
        float radius = outerDiameter / 2.0f;
        min = glm::dvec2(position.x - radius, position.y - radius);
        max = glm::dvec2(position.x + radius, position.y + radius);
    }
    
    // Generate ball bearing profile points
    std::vector<glm::dvec2> generateProfile() const;
    
    // Generate ball positions for 3D rendering
    std::vector<glm::dvec2> generateBallPositions() const;
    
    // Generate dimension lines and labels
    std::vector<std::pair<glm::dvec2, glm::dvec2>> generateDimensionLines() const;
    
    // Calculate pitch circle diameter (center of balls)
    float calculatePitchCircleDiameter() const {
        return (outerDiameter + innerDiameter) / 2.0f;
    }
    
    // Validate parameter constraints
    bool validateParameters() const {
        // Check for positive values
        if (outerDiameter <= 0.0f || innerDiameter <= 0.0f ||
            width <= 0.0f || ballDiameter <= 0.0f ||
            raceRadius <= 0.0f || numBalls <= 0) {
            return false;
        }
        
        // Check diameter relationships
        if (innerDiameter >= outerDiameter) {
            return false;
        }
        
        // Check ball size fits in race
        float raceWidth = (outerDiameter - innerDiameter) / 2.0f;
        if (ballDiameter >= raceWidth) {
            return false;
        }
        
        // Check reasonable contact angle (0-45 degrees)
        if (contactAngle < 0.0f || contactAngle > 45.0f) {
            return false;
        }
        
        return true;
    }
    
    // Calculate bounding box for fit-to-view functionality
    ImVec4 calculateBoundingBox() const {
        float radius = outerDiameter / 2.0f;
        
        // Add margin for dimensions
        double minX = -radius - 20.0f;
        double minY = -radius - 20.0f;
        double maxX = radius + 20.0f;
        double maxY = radius + 20.0f;
        
        return ImVec4(minX, minY, maxX, maxY);
    }
    
    // Reset parameters to defaults
    void resetParameters() {
        outerDiameter = 100.0f;
        innerDiameter = 50.0f;
        width = 20.0f;
        ballDiameter = 12.0f;
        numBalls = 8;
        raceRadius = 2.0f;
        contactAngle = 0.0f;
        showBalls = true;
        showCage = true;
        showDimensions = true;
    }
};

} // namespace Drawing 