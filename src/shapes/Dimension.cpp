#include "shapes/BasicShapes.hpp"
#include "Canvas.hpp"
#include <cmath>
#include <string>

namespace Drawing {

void Dimension::draw(ImDrawList* drawList, Canvas* canvas) const {
    // Transform coordinates
    ImVec2 transformedStart = canvas->transformCoordinates(start);
    ImVec2 transformedEnd = canvas->transformCoordinates(end);
    ImVec2 transformedTextPos = canvas->transformCoordinates(textPosition);
    
    // Calculate angle and offsets
    float dx = end.x - start.x;
    float dy = end.y - start.y;
    float angle = std::atan2(dy, dx);
    float perpAngle = angle + M_PI / 2.0f;
    
    float extension = 20.0f * canvas->getZoomLevel();
    ImVec2 offsetVec(extension * std::cos(perpAngle), extension * std::sin(perpAngle));
    
    switch (dimType) {
        case DimensionType::Linear: {
            // Draw the dimension line
            drawList->AddLine(transformedStart, transformedEnd, color, thickness * canvas->getZoomLevel());
            
            // Draw extension lines perpendicular to the main dimension line
            drawList->AddLine(transformedStart, 
                            ImVec2(transformedStart.x + offsetVec.x, transformedStart.y + offsetVec.y), 
                            color, thickness * 0.5f * canvas->getZoomLevel());
            drawList->AddLine(transformedEnd, 
                            ImVec2(transformedEnd.x + offsetVec.x, transformedEnd.y + offsetVec.y), 
                            color, thickness * 0.5f * canvas->getZoomLevel());
            
            break;
        }
        
        case DimensionType::Diameter: {
            // Draw a circle with center at start and radius = distance to end
            float dx = end.x - start.x;
            float dy = end.y - start.y;
            float radius = std::sqrt(dx * dx + dy * dy);
            
            // Draw the circle
            drawList->AddCircle(transformedStart, radius * canvas->getZoomLevel(), 
                              color, 48, thickness * canvas->getZoomLevel());
            
            // Draw a line from center to the edge
            drawList->AddLine(transformedStart, transformedEnd, 
                            color, thickness * 0.5f * canvas->getZoomLevel());
            
            break;
        }
        
        case DimensionType::Radius: {
            // Draw a circle with center at start and radius = distance to end
            float dx = end.x - start.x;
            float dy = end.y - start.y;
            float radius = std::sqrt(dx * dx + dy * dy);
            
            // Draw the circle
            drawList->AddCircle(transformedStart, radius * canvas->getZoomLevel(), 
                              color, 48, thickness * canvas->getZoomLevel());
            
            // Draw a line from center to the edge
            drawList->AddLine(transformedStart, transformedEnd, 
                            color, thickness * 0.5f * canvas->getZoomLevel());
            
            break;
        }
        
        case DimensionType::Angular: {
            // Not fully implemented - just draw a line for now
            drawList->AddLine(transformedStart, transformedEnd, 
                            color, thickness * canvas->getZoomLevel());
            
            break;
        }
    }
    
    // Prepare dimension text with appropriate units
    std::string displayText = dimensionText;
    
    // Check if we should use unit-aware formatting
    if (canvas->hasUnitSystem()) {
        float displayValue = canvas->getDisplayValue(lengthInPixels);
        
        std::string unitText;
        switch(canvas->getCurrentUnit()) {
            case UnitSystem::Millimeters: unitText = " mm"; break;
            case UnitSystem::Centimeters: unitText = " cm"; break;
            case UnitSystem::Inches: unitText = "\""; break;
            default: unitText = " px";
        }
        
        // Format with 1 decimal place
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.1f%s", displayValue, unitText.c_str());
        displayText = buffer;
    }
    
    // Draw the text
    ImVec2 textSize = ImGui::CalcTextSize(displayText.c_str());
    
    // Background for better visibility
    drawList->AddRectFilled(
        ImVec2(transformedTextPos.x - textSize.x * 0.5f - 2.0f, transformedTextPos.y - textSize.y * 0.5f - 2.0f),
        ImVec2(transformedTextPos.x + textSize.x * 0.5f + 2.0f, transformedTextPos.y + textSize.y * 0.5f + 2.0f),
        IM_COL32(240, 240, 240, 220)
    );
    
    // Text
    drawList->AddText(
        ImVec2(transformedTextPos.x - textSize.x * 0.5f, transformedTextPos.y - textSize.y * 0.5f),
        IM_COL32(0, 0, 0, 255),
        displayText.c_str()
    );
    
    // Draw points at the dimension endpoints
    if (dimType != DimensionType::Angular) {
        drawList->AddCircleFilled(transformedStart, 3.0f * canvas->getZoomLevel(), color, 8);
        drawList->AddCircleFilled(transformedEnd, 3.0f * canvas->getZoomLevel(), color, 8);
    }
}

bool Dimension::isPointNear(const ImVec2& point, float threshold) const {
    // Check if the point is near any of the dimension lines or the text
    
    // Check start and end points
    float distToStart = std::sqrt(std::pow(point.x - start.x, 2) + std::pow(point.y - start.y, 2));
    float distToEnd = std::sqrt(std::pow(point.x - end.x, 2) + std::pow(point.y - end.y, 2));
    
    if (distToStart <= threshold || distToEnd <= threshold) {
        return true;
    }
    
    // Check if near the text position
    float distToText = std::sqrt(std::pow(point.x - textPosition.x, 2) + std::pow(point.y - textPosition.y, 2));
    if (distToText <= threshold * 2.0f) {  // Larger threshold for text
        return true;
    }
    
    // Check if near the dimension line
    float lineLength = std::sqrt(std::pow(end.x - start.x, 2) + std::pow(end.y - start.y, 2));
    
    if (lineLength > 0.0f) {
        // Calculate distance to line
        float t = ((point.x - start.x) * (end.x - start.x) + (point.y - start.y) * (end.y - start.y)) 
                  / (lineLength * lineLength);
        
        if (t >= 0.0f && t <= 1.0f) {
            float px = start.x + t * (end.x - start.x);
            float py = start.y + t * (end.y - start.y);
            float distance = std::sqrt(std::pow(point.x - px, 2) + std::pow(point.y - py, 2));
            
            if (distance <= threshold) {
                return true;
            }
        }
    }
    
    // If dimension type is Diameter or Radius, also check if point is near the circle
    if (dimType == DimensionType::Diameter || dimType == DimensionType::Radius) {
        float dx = end.x - start.x;
        float dy = end.y - start.y;
        float radius = std::sqrt(dx * dx + dy * dy);
        
        float distToCenter = std::sqrt(std::pow(point.x - start.x, 2) + std::pow(point.y - start.y, 2));
        if (std::abs(distToCenter - radius) <= threshold) {
            return true;
        }
    }
    
    return false;
}

std::unique_ptr<Shape> Dimension::clone() const {
    return std::make_unique<Dimension>(*this);
}

bool Dimension::isValid() const {
    return (start.x != end.x) || (start.y != end.y);
}

ImVec2 Dimension::getCenter() const {
    return ImVec2((start.x + end.x) / 2.0f, (start.y + end.y) / 2.0f);
}

void Dimension::move(const ImVec2& delta) {
    start.x += delta.x;
    start.y += delta.y;
    end.x += delta.x;
    end.y += delta.y;
    textPosition.x += delta.x;
    textPosition.y += delta.y;
}

void Dimension::scale(float factor) {
    ImVec2 center = getCenter();
    start.x = center.x + (start.x - center.x) * factor;
    start.y = center.y + (start.y - center.y) * factor;
    end.x = center.x + (end.x - center.x) * factor;
    end.y = center.y + (end.y - center.y) * factor;
    textPosition.x = center.x + (textPosition.x - center.x) * factor;
    textPosition.y = center.y + (textPosition.y - center.y) * factor;
}

void Dimension::rotate(float angle, const ImVec2& center) {
    float s = sin(angle);
    float c = cos(angle);
    
    // Translate points relative to rotation center
    float startX = start.x - center.x;
    float startY = start.y - center.y;
    float endX = end.x - center.x;
    float endY = end.y - center.y;
    float textX = textPosition.x - center.x;
    float textY = textPosition.y - center.y;
    
    // Rotate points
    float newStartX = startX * c - startY * s;
    float newStartY = startX * s + startY * c;
    float newEndX = endX * c - endY * s;
    float newEndY = endX * s + endY * c;
    float newTextX = textX * c - textY * s;
    float newTextY = textX * s + textY * c;
    
    // Translate back
    start.x = newStartX + center.x;
    start.y = newStartY + center.y;
    end.x = newEndX + center.x;
    end.y = newEndY + center.y;
    textPosition.x = newTextX + center.x;
    textPosition.y = newTextY + center.y;
}

} // namespace Drawing 