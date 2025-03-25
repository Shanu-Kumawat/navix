#pragma once

#include <imgui.h>
#include <array>
#include <cmath>  // for std::abs and other math functions
#include <memory> // for std::unique_ptr
#include <algorithm> // for std::minmax_element
#include "Constants.hpp"

namespace Drawing {

enum class ShapeType {
    POINT,
    LINE,
    CIRCLE,
    TRIANGLE,
    SQUARE,
    RECTANGLE,
    SPLINE,
    BEZIER
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
    virtual bool isPointNear(const ImVec2& point, float threshold) const = 0;
};

struct Point : public Shape {
    ImVec2 position;
    float size;

    Point(const ImVec2& pos, ImU32 color = Colors::POINT, float s = Constants::DEFAULT_POINT_SIZE)
        : Shape(ShapeType::POINT, color), position(pos), size(s) {}

    std::unique_ptr<Shape> clone() const override {
        return std::make_unique<Point>(*this);
    }

    bool isValid() const override {
        return size > 0;
    }

    bool isPointNear(const ImVec2& point, float threshold) const override {
        float dx = point.x - position.x;
        float dy = point.y - position.y;
        return (dx * dx + dy * dy) <= threshold * threshold;
    }
};

struct Line : public Shape {
    ImVec2 start;
    ImVec2 end;

    Line(const ImVec2& s, const ImVec2& e, ImU32 color = Colors::LINE, float thickness = Constants::DEFAULT_LINE_THICKNESS)
        : Shape(ShapeType::LINE, color, thickness), start(s), end(e) {}

    bool isValid() const override {
        return thickness > 0 && (start.x != end.x || start.y != end.y);
    }

    bool isPointNear(const ImVec2& point, float threshold) const override {
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
};

struct Circle : public Shape {
    ImVec2 center;
    float radius;

    Circle(const ImVec2& c, float r, ImU32 color = Colors::CIRCLE, float thickness = Constants::DEFAULT_LINE_THICKNESS)
        : Shape(ShapeType::CIRCLE, color, thickness), center(c), radius(r) {}

    bool isValid() const override {
        return radius > 0 && thickness > 0;
    }

    bool isPointNear(const ImVec2& point, float threshold) const override {
        float dx = point.x - center.x;
        float dy = point.y - center.y;
        float distance = std::sqrt(dx * dx + dy * dy);
        return std::abs(distance - radius) <= threshold;
    }

    std::unique_ptr<Shape> clone() const override {
        return std::make_unique<Circle>(*this);
    }
};

struct Triangle : public Shape {
    std::array<ImVec2, 3> points;

    Triangle(const std::array<ImVec2, 3>& pts, ImU32 color = Colors::TRIANGLE, float thickness = Constants::DEFAULT_LINE_THICKNESS)
        : Shape(ShapeType::TRIANGLE, color, thickness), points(pts) {}

    bool isValid() const override {
        return thickness > 0 && calculateArea() > Constants::MIN_SHAPE_SIZE * Constants::MIN_SHAPE_SIZE;
    }

    float calculateArea() const {
        return std::fabs((points[1].x - points[0].x) * (points[2].y - points[0].y) -
                       (points[2].x - points[0].x) * (points[1].y - points[0].y)) * 0.5f;
    }

    bool isPointNear(const ImVec2& point, float threshold) const override {
        // Check if point is near any of the edges
        for (int i = 0; i < 3; ++i) {
            const ImVec2& start = points[i];
            const ImVec2& end = points[(i + 1) % 3];
            
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
};

struct Square : public Shape {
    ImVec2 start;
    ImVec2 end;

    Square(const ImVec2& s, const ImVec2& e, ImU32 color = Colors::SQUARE, float thickness = Constants::DEFAULT_LINE_THICKNESS)
        : Shape(ShapeType::SQUARE, color, thickness), start(s), end(e) {}

    bool isValid() const override {
        float size = std::abs(end.x - start.x);
        return thickness > 0 && size > Constants::MIN_SHAPE_SIZE;
    }

    ImVec2 getTopLeft() const {
        return ImVec2(std::min(start.x, end.x), std::min(start.y, end.y));
    }

    float getSize() const {
        return std::abs(end.x - start.x);
    }

    bool isPointNear(const ImVec2& point, float threshold) const override {
        ImVec2 topLeft = getTopLeft();
        float size = getSize();
        
        // Calculate the four corners
        ImVec2 topRight(topLeft.x + size, topLeft.y);
        ImVec2 bottomLeft(topLeft.x, topLeft.y + size);
        ImVec2 bottomRight(topLeft.x + size, topLeft.y + size);
        
        // Check if point is near any of the edges
        const std::array<std::pair<ImVec2, ImVec2>, 4> edges = {{
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
};

struct Rectangle : public Shape {
    ImVec2 topLeft;
    ImVec2 topRight;
    ImVec2 bottomRight;
    ImVec2 bottomLeft;

    Rectangle(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, 
             ImU32 color = Colors::RECTANGLE, float thickness = Constants::DEFAULT_LINE_THICKNESS)
        : Shape(ShapeType::RECTANGLE, color, thickness) {
        // Calculate the corners based on the points
        std::vector<float> xCoords = {p1.x, p2.x, p3.x, p4.x};
        std::vector<float> yCoords = {p1.y, p2.y, p3.y, p4.y};
        
        auto [minXIt, maxXIt] = std::minmax_element(xCoords.begin(), xCoords.end());
        auto [minYIt, maxYIt] = std::minmax_element(yCoords.begin(), yCoords.end());
        
        float minX = *minXIt;
        float maxX = *maxXIt;
        float minY = *minYIt;
        float maxY = *maxYIt;

        topLeft = ImVec2(minX, minY);
        topRight = ImVec2(maxX, minY);
        bottomRight = ImVec2(maxX, maxY);
        bottomLeft = ImVec2(minX, maxY);
    }

    bool isValid() const override {
        ImVec2 size = getSize();
        return thickness > 0 && 
               size.x > Constants::MIN_SHAPE_SIZE &&
               size.y > Constants::MIN_SHAPE_SIZE;
    }

    ImVec2 getTopLeft() const {
        return topLeft;
    }

    ImVec2 getSize() const {
        return ImVec2(std::abs(topRight.x - topLeft.x), std::abs(bottomLeft.y - topLeft.y));
    }

    bool isPointNear(const ImVec2& point, float threshold) const override {
        // Check if point is near any of the edges
        const std::array<std::pair<ImVec2, ImVec2>, 4> edges = {{
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
};

} // namespace Drawing 