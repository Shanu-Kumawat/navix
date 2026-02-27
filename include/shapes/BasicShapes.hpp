#include <glm/glm.hpp>
#pragma once

#include <imgui.h>
#include <array>
#include <cmath>  // for std::abs and other math functions
#include <memory> // for std::unique_ptr
#include <algorithm> // for std::minmax_element
#include <string>    // for std::string and std::to_string
#include <vector>    // for std::vector
#include "Constants.hpp"

namespace Drawing {

// Forward declaration
class Canvas;
struct ShockAbsorberEnd2D;

enum class ShapeType {
    POINT,
    LINE,
    CIRCLE,
    TRIANGLE,
    SQUARE,
    RECTANGLE,
    SPLINE,
    BEZIER,
    DIMENSION,
    BELLOWS,
    BALL_BEARING,
    SPRING2D,
    SHOCK_ABSORBER_END_2D,
    SHOCK_ABSORBER_BOTTOM_END
};

struct Shape {
    ShapeType type;
    ImU32 color;
    float thickness;

    Shape(ShapeType t, ImU32 c = Colors::LINE, float th = Constants::DEFAULT_LINE_THICKNESS) 
        : type(t), color(c), thickness(th) {}
    
    virtual ~Shape() = default;
    virtual std::unique_ptr<Shape> clone() const = 0;
    virtual bool isValid() const = 0;
    virtual bool isPointNear(const glm::dvec2& point, float threshold) const = 0;
    virtual void getBounds(glm::dvec2& min, glm::dvec2& max) const = 0;
};

struct Point : public Shape {
    glm::dvec2 position;
    float size;

    Point(const glm::dvec2& pos, ImU32 color = Colors::POINT, float s = Constants::DEFAULT_POINT_SIZE)
        : Shape(ShapeType::POINT, color), position(pos), size(s) {}

    std::unique_ptr<Shape> clone() const override {
        return std::make_unique<Point>(*this);
    }

    bool isValid() const override {
        return size > 0;
    }

    bool isPointNear(const glm::dvec2& point, float threshold) const override {
        float dx = point.x - position.x;
        float dy = point.y - position.y;
        return (dx * dx + dy * dy) <= threshold * threshold;
    }

    void getBounds(glm::dvec2& min, glm::dvec2& max) const override {
        min = glm::dvec2(position.x - size, position.y - size);
        max = glm::dvec2(position.x + size, position.y + size);
    }
};

struct Line : public Shape {
    glm::dvec2 start;
    glm::dvec2 end;

    Line(const glm::dvec2& s, const glm::dvec2& e, ImU32 color = Colors::LINE, float thickness = Constants::DEFAULT_LINE_THICKNESS)
        : Shape(ShapeType::LINE, color, thickness), start(s), end(e) {}

    bool isValid() const override {
        return thickness > 0 && (start.x != end.x || start.y != end.y);
    }

    bool isPointNear(const glm::dvec2& point, float threshold) const override {
        // Calculate distance from point to line segment
        float lineLength = std::sqrt(
            (end.x - start.x) * (end.x - start.x) + 
            (end.y - start.y) * (end.y - start.y)
        );
        
        if (lineLength < 0.0001f) {
            // Line is actually a point
            return std::sqrt(
                (point.x - start.x) * (point.x - start.x) + 
                (point.y - start.y) * (point.y - start.y)
            ) <= threshold;
        }
        
        // Calculate distance using the line segment formula
        float t = ((point.x - start.x) * (end.x - start.x) + 
                  (point.y - start.y) * (end.y - start.y)) / (lineLength * lineLength);
        
        if (t < 0.0f) {
            // Point is beyond the start of the line
            return std::sqrt(
                (point.x - start.x) * (point.x - start.x) + 
                (point.y - start.y) * (point.y - start.y)
            ) <= threshold;
        }
        if (t > 1.0f) {
            // Point is beyond the end of the line
            return std::sqrt(
                (point.x - end.x) * (point.x - end.x) + 
                (point.y - end.y) * (point.y - end.y)
            ) <= threshold;
        }
        
        // Calculate the closest point on the line
        float closestX = start.x + t * (end.x - start.x);
        float closestY = start.y + t * (end.y - start.y);
        
        // Calculate distance to the closest point
        return std::sqrt(
            (point.x - closestX) * (point.x - closestX) + 
            (point.y - closestY) * (point.y - closestY)
        ) <= threshold;
    }

    std::unique_ptr<Shape> clone() const override {
        return std::make_unique<Line>(*this);
    }

    void getBounds(glm::dvec2& min, glm::dvec2& max) const override {
        min = glm::dvec2(std::min(start.x, end.x), std::min(start.y, end.y));
        max = glm::dvec2(std::max(start.x, end.x), std::max(start.y, end.y));
    }
};

struct Circle : public Shape {
    glm::dvec2 center;
    float radius;

    Circle(const glm::dvec2& c, float r, ImU32 color = Colors::CIRCLE, float thickness = Constants::DEFAULT_LINE_THICKNESS)
        : Shape(ShapeType::CIRCLE, color, thickness), center(c), radius(r) {}

    bool isValid() const override {
        return radius > 0 && thickness > 0;
    }

    bool isPointNear(const glm::dvec2& point, float threshold) const override {
        float dx = point.x - center.x;
        float dy = point.y - center.y;
        float distance = std::sqrt(dx * dx + dy * dy);
        return std::abs(distance - radius) <= threshold;
    }

    std::unique_ptr<Shape> clone() const override {
        return std::make_unique<Circle>(*this);
    }

    void getBounds(glm::dvec2& min, glm::dvec2& max) const override {
        min = glm::dvec2(center.x - radius, center.y - radius);
        max = glm::dvec2(center.x + radius, center.y + radius);
    }
};

struct Triangle : public Shape {
    std::array<glm::dvec2, 3> points;

    Triangle(const std::array<glm::dvec2, 3>& pts, ImU32 color = Colors::TRIANGLE, float thickness = Constants::DEFAULT_LINE_THICKNESS)
        : Shape(ShapeType::TRIANGLE, color, thickness), points(pts) {}

    bool isValid() const override {
        return thickness > 0 && calculateArea() > Constants::MIN_SHAPE_SIZE * Constants::MIN_SHAPE_SIZE;
    }

    float calculateArea() const {
        return std::fabs((points[1].x - points[0].x) * (points[2].y - points[0].y) -
                       (points[2].x - points[0].x) * (points[1].y - points[0].y)) * 0.5f;
    }

    bool isPointNear(const glm::dvec2& point, float threshold) const override {
        // Check if point is near any of the edges
        for (int i = 0; i < 3; ++i) {
            const glm::dvec2& start = points[i];
            const glm::dvec2& end = points[(i + 1) % 3];
            
            float lineLength = std::sqrt(
                (end.x - start.x) * (end.x - start.x) + 
                (end.y - start.y) * (end.y - start.y)
            );
            
            if (lineLength < 0.0001f) continue;
            
            float t = ((point.x - start.x) * (end.x - start.x) + 
                      (point.y - start.y) * (end.y - start.y)) / (lineLength * lineLength);
            
            if (t >= 0.0f && t <= 1.0f) {
                float closestX = start.x + t * (end.x - start.x);
                float closestY = start.y + t * (end.y - start.y);
                
                float distance = std::sqrt(
                    (point.x - closestX) * (point.x - closestX) + 
                    (point.y - closestY) * (point.y - closestY)
                );
                
                if (distance <= threshold) return true;
            }
        }
        return false;
    }

    std::unique_ptr<Shape> clone() const override {
        return std::make_unique<Triangle>(*this);
    }

    void getBounds(glm::dvec2& min, glm::dvec2& max) const override {
        min = glm::dvec2(std::min({points[0].x, points[1].x, points[2].x}),
                    std::min({points[0].y, points[1].y, points[2].y}));
        max = glm::dvec2(std::max({points[0].x, points[1].x, points[2].x}),
                    std::max({points[0].y, points[1].y, points[2].y}));
    }
};

struct Square : public Shape {
    glm::dvec2 start;
    glm::dvec2 end;

    Square(const glm::dvec2& s, const glm::dvec2& e, ImU32 color = Colors::SQUARE, float thickness = Constants::DEFAULT_LINE_THICKNESS)
        : Shape(ShapeType::SQUARE, color, thickness), start(s), end(e) {}

    bool isValid() const override {
        float size = std::abs(end.x - start.x);
        return thickness > 0 && size > Constants::MIN_SHAPE_SIZE;
    }

    glm::dvec2 getTopLeft() const {
        return glm::dvec2(std::min(start.x, end.x), std::min(start.y, end.y));
    }

    float getSize() const {
        return std::abs(end.x - start.x);
    }

    bool isPointNear(const glm::dvec2& point, float threshold) const override {
        glm::dvec2 topLeft = getTopLeft();
        float size = getSize();
        
        // Calculate the four corners
        glm::dvec2 topRight(topLeft.x + size, topLeft.y);
        glm::dvec2 bottomLeft(topLeft.x, topLeft.y + size);
        glm::dvec2 bottomRight(topLeft.x + size, topLeft.y + size);
        
        // Check if point is near any of the edges
        const std::array<std::pair<glm::dvec2, glm::dvec2>, 4> edges = {{
            {topLeft, topRight},
            {topRight, bottomRight},
            {bottomRight, bottomLeft},
            {bottomLeft, topLeft}
        }};
        
        for (const auto& [edgeStart, edgeEnd] : edges) {
            float lineLength = std::sqrt(
                (edgeEnd.x - edgeStart.x) * (edgeEnd.x - edgeStart.x) + 
                (edgeEnd.y - edgeStart.y) * (edgeEnd.y - edgeStart.y)
            );
            
            if (lineLength < 0.0001f) continue;
            
            float t = ((point.x - edgeStart.x) * (edgeEnd.x - edgeStart.x) + 
                      (point.y - edgeStart.y) * (edgeEnd.y - edgeStart.y)) / (lineLength * lineLength);
            
            if (t >= 0.0f && t <= 1.0f) {
                float closestX = edgeStart.x + t * (edgeEnd.x - edgeStart.x);
                float closestY = edgeStart.y + t * (edgeEnd.y - edgeStart.y);
                
                float distance = std::sqrt(
                    (point.x - closestX) * (point.x - closestX) + 
                    (point.y - closestY) * (point.y - closestY)
                );
                
                if (distance <= threshold) return true;
            }
        }
        return false;
    }

    std::unique_ptr<Shape> clone() const override {
        return std::make_unique<Square>(*this);
    }

    void getBounds(glm::dvec2& min, glm::dvec2& max) const override {
        glm::dvec2 topLeft = getTopLeft();
        float size = getSize();
        min = topLeft;
        max = glm::dvec2(topLeft.x + size, topLeft.y + size);
    }
};

struct Rectangle : public Shape {
    glm::dvec2 topLeft;
    glm::dvec2 topRight;
    glm::dvec2 bottomRight;
    glm::dvec2 bottomLeft;

    Rectangle(const glm::dvec2& p1, const glm::dvec2& p2, const glm::dvec2& p3, const glm::dvec2& p4, 
             ImU32 color = Colors::RECTANGLE, float thickness = Constants::DEFAULT_LINE_THICKNESS)
        : Shape(ShapeType::RECTANGLE, color, thickness) {
        // Calculate the corners based on the points
        std::vector<double> xCoords = {p1.x, p2.x, p3.x, p4.x};
        std::vector<double> yCoords = {p1.y, p2.y, p3.y, p4.y};
        
        auto [minXIt, maxXIt] = std::minmax_element(xCoords.begin(), xCoords.end());
        auto [minYIt, maxYIt] = std::minmax_element(yCoords.begin(), yCoords.end());
        
        float minX = *minXIt;
        float maxX = *maxXIt;
        float minY = *minYIt;
        float maxY = *maxYIt;

        topLeft = glm::dvec2(minX, minY);
        topRight = glm::dvec2(maxX, minY);
        bottomRight = glm::dvec2(maxX, maxY);
        bottomLeft = glm::dvec2(minX, maxY);
    }

    bool isValid() const override {
        glm::dvec2 size = getSize();
        return thickness > 0 && 
               size.x > Constants::MIN_SHAPE_SIZE &&
               size.y > Constants::MIN_SHAPE_SIZE;
    }

    glm::dvec2 getTopLeft() const {
        return topLeft;
    }

    glm::dvec2 getSize() const {
        return glm::dvec2(std::abs(topRight.x - topLeft.x), std::abs(bottomLeft.y - topLeft.y));
    }

    bool isPointNear(const glm::dvec2& point, float threshold) const override {
        // Check if point is near any of the edges
        const std::array<std::pair<glm::dvec2, glm::dvec2>, 4> edges = {{
            {topLeft, topRight},
            {topRight, bottomRight},
            {bottomRight, bottomLeft},
            {bottomLeft, topLeft}
        }};
        
        for (const auto& [edgeStart, edgeEnd] : edges) {
            float lineLength = std::sqrt(
                (edgeEnd.x - edgeStart.x) * (edgeEnd.x - edgeStart.x) + 
                (edgeEnd.y - edgeStart.y) * (edgeEnd.y - edgeStart.y)
            );
            
            if (lineLength < 0.0001f) continue;
            
            float t = ((point.x - edgeStart.x) * (edgeEnd.x - edgeStart.x) + 
                      (point.y - edgeStart.y) * (edgeEnd.y - edgeStart.y)) / (lineLength * lineLength);
            
            if (t >= 0.0f && t <= 1.0f) {
                float closestX = edgeStart.x + t * (edgeEnd.x - edgeStart.x);
                float closestY = edgeStart.y + t * (edgeEnd.y - edgeStart.y);
                
                float distance = std::sqrt(
                    (point.x - closestX) * (point.x - closestX) + 
                    (point.y - closestY) * (point.y - closestY)
                );
                
                if (distance <= threshold) return true;
            }
        }
        return false;
    }

    std::unique_ptr<Shape> clone() const override {
        return std::make_unique<Rectangle>(*this);
    }

    void getBounds(glm::dvec2& min, glm::dvec2& max) const override {
        min = topLeft;
        max = bottomRight;
    }
};

// Dimension type enum
enum class DimensionType {
    Linear,
    Diameter,
    Radius,
    Angular
};

// Dimension shape for technical drawings
struct Dimension : public Shape {
    glm::dvec2 start;
    glm::dvec2 end;
    glm::dvec2 textPosition;
    std::string dimensionText;
    DimensionType dimType;
    float lengthInPixels;

    Dimension(const glm::dvec2& s, const glm::dvec2& e, 
              ImU32 color = Colors::LINE, 
              float thickness = Constants::DEFAULT_LINE_THICKNESS,
              DimensionType type = DimensionType::Linear)
        : Shape(ShapeType::DIMENSION, color, thickness), 
          start(s), end(e), 
          dimType(type)
    {
        // Calculate mid-point for text position by default
        textPosition = glm::dvec2((start.x + end.x) / 2.0f, (start.y + end.y) / 2.0f - 10.0f);
        
        // Calculate dimension length
        float dx = end.x - start.x;
        float dy = end.y - start.y;
        lengthInPixels = std::sqrt(dx * dx + dy * dy);
        
        // Set default dimension text (can be overridden)
        dimensionText = std::to_string(lengthInPixels);
    }

    bool isValid() const override;
    bool isPointNear(const glm::dvec2& point, float threshold) const override;
    std::unique_ptr<Shape> clone() const override;

    // Draw the dimension with text
    void draw(ImDrawList* drawList, Canvas* canvas) const;
    
    // Utility methods
    glm::dvec2 getCenter() const;
    void move(const glm::dvec2& delta);
    void scale(float factor);
    void rotate(float angle, const glm::dvec2& center);

    void getBounds(glm::dvec2& min, glm::dvec2& max) const override {
        // Calculate bounds based on start and end points
        min = glm::dvec2(std::min(start.x, end.x), std::min(start.y, end.y));
        max = glm::dvec2(std::max(start.x, end.x), std::max(start.y, end.y));
        
        // Add margin for text and extension lines
        float margin = 20.0f; // Adjust this value based on your text size and extension line length
        min.x -= margin;
        min.y -= margin;
        max.x += margin;
        max.y += margin;
    }
};

struct Spring2D : public Shape {
    float centerX, centerY;
    float outerDiameter;
    float wireDiameter;
    float freeLength;
    int numCoils;

    Spring2D(float cx, float cy, float od, float wd, float fl, int coils, ImU32 color = Colors::LINE, float thickness = Constants::DEFAULT_LINE_THICKNESS)
        : Shape(ShapeType::SPRING2D, color, thickness),
          centerX(cx), centerY(cy), outerDiameter(od), wireDiameter(wd), freeLength(fl), numCoils(coils) {}

    // Static method for template-based shape finding
    static ShapeType GetShapeType() { return ShapeType::SPRING2D; }

    std::unique_ptr<Shape> clone() const override {
        return std::make_unique<Spring2D>(*this);
    }
    bool isValid() const override {
        return outerDiameter > wireDiameter && wireDiameter > 0 && freeLength > 0 && numCoils > 0;
    }
    bool isPointNear(const glm::dvec2& point, float threshold) const override {
        float dx = point.x - centerX;
        float dy = point.y - centerY;
        float distance = std::sqrt(dx * dx + dy * dy);
        float radius = outerDiameter / 2.0f;
        // Allow selection if within the outer circle, plus a threshold
        return distance <= radius + threshold;
    }
    bool isPointInBoundingBox(const glm::dvec2& point, float threshold) const {
        float halfLength = freeLength / 2.0f;
        float radius = outerDiameter / 2.0f;
        return (point.x >= centerX - radius - threshold && point.x <= centerX + radius + threshold &&
                point.y >= centerY - halfLength - threshold && point.y <= centerY + halfLength + threshold);
    }

    void getBounds(glm::dvec2& min, glm::dvec2& max) const override {
        // Calculate the bounds based on the spring's properties
        float radius = outerDiameter / 2.0f;
        min = glm::dvec2(centerX - radius, centerY - radius);
        max = glm::dvec2(centerX + radius, centerY + radius);
    }

    std::vector<glm::dvec2> generateProfile() const;
};

enum class EndPosition { Top, Bottom };
struct ShockAbsorberEnd2D : public Shape {
    const Spring2D* parentSpring;
    EndPosition position; // Top or Bottom
    float shaftLength;
    float step1Length;
    float step2Length;
    float step3Length;
    float step1Diameter;
    float step2Diameter;
    float step3Diameter;
    float boreDiameter;
    float chamfer;
    glm::dvec2 baseCenter;

    ShockAbsorberEnd2D(const Spring2D* spring, EndPosition pos = EndPosition::Top, ImU32 color = IM_COL32(80, 80, 80, 255), float thickness = 2.0f)
        : Shape(ShapeType::SHOCK_ABSORBER_END_2D, color, thickness), parentSpring(spring), position(pos)
    {
        updateGeometry();
    }

    // Static method for template-based shape finding
    static ShapeType GetShapeType() { return ShapeType::SHOCK_ABSORBER_END_2D; }

    void updateGeometry() {
        float od = parentSpring->outerDiameter;
        float wd = parentSpring->wireDiameter;
        float fl = parentSpring->freeLength;
        // step1 covers 50% of the spring, step2 15%, step3 10%
        step1Length = fl * 0.50f;
        step2Length = fl * 0.15f;
        step3Length = fl * 0.10f;
        shaftLength = step1Length + step2Length + step3Length;
        step1Diameter = od * 0.9f;
        step2Diameter = step1Diameter; // Make step 2 as wide as step 1
        step3Diameter = od * 0.5f;
        boreDiameter = od * 0.22f;
        chamfer = step1Diameter * 0.08f;
        
        // baseCenter.y must be at the center of the total shaft length for isPointNear to work
        // Drawing code uses: top = baseCenter.y - shaftLength/2, bottom = baseCenter.y + shaftLength/2
        
        float springTop = parentSpring->centerY - fl/2;
        float springBottom = parentSpring->centerY + fl/2;
        
        if (position == EndPosition::Top) {
            // Top end: step1 bottom sits at springTop (slight overlap into spring)
            // Shaft spans from (springTop - shaftLength + step1Length) to (springTop + step1Length)
            // But drawing has step1 at bottom, so y0 = springTop + step1Length (drawn bottom)
            // Actually, let me trace drawing code more carefully...
            // Drawing: y3=baseCenter.y-shaftLength/2 (top), y0=y3+shaftLength (bottom)
            // Step3 drawn at y3, step1 drawn ending at y0
            // We want step1 (bottom of shaft) to overlap spring, so y0 ≈ springTop + step1Length
            // Therefore baseCenter.y = y0 - shaftLength/2 = springTop + step1Length - shaftLength/2
            baseCenter = glm::dvec2(parentSpring->centerX, springTop + step1Length - shaftLength/2);
        } else {
            // Bottom end: step1 top sits at springBottom (slight overlap into spring)
            // Drawing code for bottom: y0=baseCenter.y-shaftLength/2, y3=y0+shaftLength
            // Step1 drawn starting at y0, step3 at bottom (y3)
            // We want step1 (top of shaft) to overlap spring, so y0 ≈ springBottom - step1Length
            // Therefore baseCenter.y = y0 + shaftLength/2 = springBottom - step1Length + shaftLength/2
            baseCenter = glm::dvec2(parentSpring->centerX, springBottom - step1Length + shaftLength/2);
        }
    }

    std::unique_ptr<Shape> clone() const override {
        return std::make_unique<ShockAbsorberEnd2D>(*this);
    }
    bool isValid() const override { return parentSpring != nullptr; }
    bool isPointNear(const glm::dvec2& point, float threshold) const override {
        float halfLen = shaftLength / 2.0f;
        float halfDia = step1Diameter / 2.0f;
        if (point.x >= baseCenter.x - halfDia - threshold && point.x <= baseCenter.x + halfDia + threshold &&
            point.y >= baseCenter.y - halfLen - threshold && point.y <= baseCenter.y + halfLen + threshold) {
            return true;
        }
        return false;
    }
    void getBounds(glm::dvec2& min, glm::dvec2& max) const override {
        float halfLen = shaftLength / 2.0f;
        float halfDia = step1Diameter / 2.0f;
        min = glm::dvec2(baseCenter.x - halfDia, baseCenter.y - halfLen);
        max = glm::dvec2(baseCenter.x + halfDia, baseCenter.y + halfLen);
    }

    std::vector<glm::dvec2> generateProfile() const;
};

} // namespace Drawing 