#include <iostream>
#include "Renderer2D.hpp"
#include "Canvas.hpp"
#include "utils/MathUtils.hpp"
#include "utils/VectorMath.hpp"
#include "topology/Node.hpp"
#include "topology/Edge.hpp"
#include <cmath>

namespace Core {
using namespace Drawing::Math;

Renderer2D::Renderer2D(Drawing::Canvas* canvas) : canvas(canvas) {}

// ── Core::Renderer interface implementations ────────────────────────────

ImVec2 Renderer2D::toScreen(const glm::dvec2& worldPt) const {
    return ImVec2(
        static_cast<float>(currentWindowPos.x + currentPan.x + worldPt.x * currentZoom),
        static_cast<float>(currentWindowPos.y + currentPan.y + worldPt.y * currentZoom)
    );
}

void Renderer2D::beginFrame(ImDrawList* drawList) {
    currentDrawList = drawList;
}

void Renderer2D::setTransform(const glm::dvec2& pan, float zoom, const glm::dvec2& windowPos) {
    currentPan = pan;
    currentZoom = zoom;
    currentWindowPos = windowPos;
}

void Renderer2D::drawLine(const glm::dvec2& start, const glm::dvec2& end, uint32_t color, float thickness) {
    if (!currentDrawList) return;
    currentDrawList->AddLine(toScreen(start), toScreen(end), color, thickness);
}

void Renderer2D::drawDashedLine(const glm::dvec2& start, const glm::dvec2& end, uint32_t color, float thickness, float dashLength) {
    if (!currentDrawList) return;
    // Reuse the existing ImVec2 variant
    drawDashedLine(currentDrawList, toScreen(start), toScreen(end), color, thickness, dashLength);
}

void Renderer2D::drawCircle(const glm::dvec2& center, float radius, uint32_t color, float thickness, bool fill) {
    if (!currentDrawList) return;
    float screenRadius = radius * currentZoom;
    if (fill) {
        currentDrawList->AddCircleFilled(toScreen(center), screenRadius, color);
    } else {
        currentDrawList->AddCircle(toScreen(center), screenRadius, color, 0, thickness);
    }
}

void Renderer2D::drawPolygon(const std::vector<glm::dvec2>& points, uint32_t color, float thickness, bool fill) {
    if (!currentDrawList || points.empty()) return;
    std::vector<ImVec2> screenPoints;
    screenPoints.reserve(points.size());
    for (const auto& pt : points) screenPoints.push_back(toScreen(pt));
    if (fill) {
        currentDrawList->AddConvexPolyFilled(screenPoints.data(), static_cast<int>(screenPoints.size()), color);
    } else {
        currentDrawList->AddPolyline(screenPoints.data(), static_cast<int>(screenPoints.size()), color, ImDrawFlags_Closed, thickness);
    }
}

void Renderer2D::drawText(const glm::dvec2& pos, const char* text, uint32_t color) {
    if (!currentDrawList) return;
    currentDrawList->AddText(toScreen(pos), color, text);
}

void Renderer2D::drawShape(const Drawing::Shape* shape, bool isSelected) {
    if (!currentDrawList || !shape) return;
    // Delegate to the existing high-level renderShape
    renderShape(currentDrawList, shape, isSelected);
}

void Renderer2D::drawNode(const Topology::Node* node, bool isSelected) {
    if (!currentDrawList || !node) return;
    uint32_t color = isSelected ? IM_COL32(255, 100, 100, 255) : IM_COL32(0, 255, 0, 255);
    glm::dvec2 pos2d(node->getPosition().x, node->getPosition().y);
    drawCircle(pos2d, 4.0f / currentZoom, color, 1.0f, true);
}

void Renderer2D::drawEdge(const Topology::Edge* edge, const Topology::Node* n1, const Topology::Node* n2, bool isSelected) {
    if (!currentDrawList || !edge || !n1 || !n2) return;
    uint32_t color = isSelected ? IM_COL32(255, 255, 0, 255) : IM_COL32(0, 255, 255, 255);
    drawLine(glm::dvec2(n1->getPosition().x, n1->getPosition().y),
             glm::dvec2(n2->getPosition().x, n2->getPosition().y), color, 2.0f);
}

void Renderer2D::drawMesh(const Meshing::Mesh& mesh, const glm::dvec3& color) {
    if (!currentDrawList) return;
    const auto& nodesList = mesh.getNodes();
    const auto& elementsList = mesh.getElements();
    ImU32 col = IM_COL32(static_cast<int>(color.r * 255), static_cast<int>(color.g * 255), static_cast<int>(color.b * 255), 255);
    for (const auto& elem : elementsList) {
        if (elem.nodeTags.size() < 2) continue;
        for (size_t i = 0; i < elem.nodeTags.size(); ++i) {
            uint64_t nId1 = elem.nodeTags[i];
            uint64_t nId2 = elem.nodeTags[(i + 1) % elem.nodeTags.size()];
            const Meshing::MeshNode* node1 = nullptr;
            const Meshing::MeshNode* node2 = nullptr;
            for (const auto& n : nodesList) {
                if (n.tag == nId1) node1 = &n;
                if (n.tag == nId2) node2 = &n;
                if (node1 && node2) break;
            }
            if (node1 && node2) {
                drawLine(glm::dvec2(node1->position.x, node1->position.y),
                         glm::dvec2(node2->position.x, node2->position.y), col, 1.0f);
            }
        }
    }
}

// ── Existing high-level methods ─────────────────────────────────────────

void Renderer2D::render(ImDrawList* drawList, const SceneModel& model) {
    if (canvas->isGridVisible()) {
        renderGrid(drawList);
    }

    renderShapes(drawList, model.getShapes());

    // Preview rendering is handled by InputController calling renderPreview
}

void Renderer2D::renderGrid(ImDrawList* drawList) {
    // Get the canvas window position and size
    float startX = canvas->getWindowX();
    float startY = canvas->getWindowY();
    float endX = startX + canvas->getWindowWidth();
    float endY = startY + canvas->getWindowHeight();

    float gridSpacing = canvas->getGridSpacing();
    float zoomLevel = canvas->getZoomLevel();
    glm::dvec2 panOffsetVec = canvas->getPanOffset();
    ImVec2 panOffset = toImVec2(panOffsetVec);

    // Only render grid if it's enabled
    if (gridSpacing <= 0) return;

    // Calculate adjusted grid spacing based on zoom level
    float effectiveSpacing = gridSpacing * zoomLevel;
    
    // Ensure grid isn't too dense or sparse based on zoom level
    if (effectiveSpacing < 10.0f) {
        effectiveSpacing *= std::ceil(10.0f / effectiveSpacing);
    } else if (effectiveSpacing > 200.0f) {
        effectiveSpacing /= std::floor(effectiveSpacing / 100.0f);
    }

    // Calculate grid offset based on pan
    float offsetX = std::fmod(panOffset.x * zoomLevel, effectiveSpacing);
    float offsetY = std::fmod(panOffset.y * zoomLevel, effectiveSpacing);
    
    // Calculate number of lines needed
    int numLinesX = static_cast<int>(canvas->getWindowWidth() / effectiveSpacing) + 2;
    int numLinesY = static_cast<int>(canvas->getWindowHeight() / effectiveSpacing) + 2;

    // Draw vertical grid lines
    for (int i = 0; i <= numLinesX; i++) {
        float x = startX + i * effectiveSpacing + offsetX;
        if (x < startX) continue;
        if (x > endX) break;
        
        bool isMajor = (i % 5 == 0);
        ImU32 lineColor = isMajor ? Drawing::Colors::GRID_MAJOR : Drawing::Colors::GRID_MINOR;
        float lineThickness = isMajor ? 1.0f : 0.5f;
        
        drawList->AddLine(
            ImVec2(x, startY),
            ImVec2(x, endY),
            lineColor,
            lineThickness
        );
    }

    // Draw horizontal grid lines
    for (int i = 0; i <= numLinesY; i++) {
        float y = startY + i * effectiveSpacing + offsetY;
        if (y < startY) continue;
        if (y > endY) break;
        
        bool isMajor = (i % 5 == 0);
        ImU32 lineColor = isMajor ? Drawing::Colors::GRID_MAJOR : Drawing::Colors::GRID_MINOR;
        float lineThickness = isMajor ? 1.0f : 0.5f;
        
        drawList->AddLine(
            ImVec2(startX, y),
            ImVec2(endX, y),
            lineColor,
            lineThickness
        );
    }
    
    // Add axes lines at (0,0) if visible
    ImVec2 origin = toImVec2(canvas->transformCoordinates(glm::dvec2(0, 0)));
    ImU32 axisColor = IM_COL32(140, 100, 60, 240);  // Darker brown for axes (abacus frame)
    
    if (origin.x >= startX && origin.x <= endX) {
        drawList->AddLine(
            ImVec2(origin.x, startY),
            ImVec2(origin.x, endY),
            axisColor,
            1.5f
        );
    }
    
    if (origin.y >= startY && origin.y <= endY) {
        drawList->AddLine(
            ImVec2(startX, origin.y),
            ImVec2(endX, origin.y),
            axisColor,
            1.5f
        );
    }
}

void Renderer2D::renderShapes(ImDrawList* drawList, const std::vector<std::unique_ptr<Drawing::Shape>>& shapes) {
    // Calculate viewport bounds in world coordinates
    glm::dvec2 viewportMin = canvas->inverseTransformCoordinates(glm::dvec2(0, 0));
    glm::dvec2 viewportMax = canvas->inverseTransformCoordinates(glm::dvec2(canvas->getWindowWidth(), canvas->getWindowHeight()));
    
    // Render only shapes that intersect with the viewport
    for (const auto& shape : shapes) {
        if (!shape) continue;
        
        // Get shape bounds
        glm::dvec2 shapeMin, shapeMax;
        shape->getBounds(shapeMin, shapeMax);
        
        // Skip shapes outside viewport
        if (!canvas->isRectInViewport(shapeMin, shapeMax)) {
            continue;
        }
        
        // Render the shape
        bool isSelected = (canvas->getSelectedShape() == shape.get());
        renderShape(drawList, shape.get(), isSelected);
    }
}


void Renderer2D::renderBellows(ImDrawList* drawList, const Drawing::Bellows* bellows, bool isSelected) {
    // Delegate to Canvas for now, or move logic here
}

void Renderer2D::renderSprings2D(ImDrawList* drawList, const Drawing::Spring2D* spring, bool isSelected) {
    // Delegate to Canvas for now, or move logic here
}

void Renderer2D::renderBallBearings(ImDrawList* drawList, const Drawing::BallBearing* bearing, bool isSelected) {
    // Delegate to Canvas for now, or move logic here
}





void Renderer2D::renderShape(ImDrawList* drawList, const Drawing::Shape* shape, bool isSelected) {
    
    // Use the shape's own color (black from Constants.hpp) instead of hardcoding white
    ImU32 color = isSelected ? IM_COL32(255, 255, 0, 255) : shape->color;
    
    switch (shape->type) {
        case Drawing::ShapeType::POINT: {
            const auto* point = static_cast<const Drawing::Point*>(shape);
            ImVec2 transformedPos = toImVec2(canvas->transformCoordinates(point->position));
            // Make points more visible with larger size
            drawList->AddCircleFilled(
                transformedPos,
                std::max(point->size * canvas->getZoomLevel(), 5.0f), // Ensure minimum visible size
                color,
                16
            );
            break;
        }
        case Drawing::ShapeType::LINE: {
            const auto* line = static_cast<const Drawing::Line*>(shape);
            ImVec2 transformedStart = toImVec2(canvas->transformCoordinates(line->start));
            ImVec2 transformedEnd = toImVec2(canvas->transformCoordinates(line->end));
            
            // Debug visualization of line endpoints
            drawList->AddCircleFilled(transformedStart, 3.0f, IM_COL32(0, 0, 0, 255)); // Black start point (was Red)
            drawList->AddCircleFilled(transformedEnd, 3.0f, IM_COL32(0, 0, 0, 255));   // Black end point (was Green)
            
            // Draw the line with increased thickness for visibility
            drawList->AddLine(
                transformedStart,
                transformedEnd,
                color,
                std::max(line->thickness * canvas->getZoomLevel(), 2.0f) // Ensure minimum line thickness
            );
            
            break;
        }
        case Drawing::ShapeType::CIRCLE: {
            const auto* circle = static_cast<const Drawing::Circle*>(shape);
            ImVec2 transformedCenter = toImVec2(canvas->transformCoordinates(circle->center));
            
            // Add a visible center point
            drawList->AddCircleFilled(
                transformedCenter,
                3.0f,
                IM_COL32(0, 0, 0, 255) // Black center point (was Red)
            );
            
            // Draw the circle with adequate segments and thickness
            drawList->AddCircle(
                transformedCenter,
                circle->radius * canvas->getZoomLevel(),
                color,
                64, // Increased segment count for smoother circles
                std::max(circle->thickness * canvas->getZoomLevel(), 2.0f) // Ensure minimum thickness
            );
            
            // Debug output
            std::cout << "Drawing circle: center (" << circle->center.x << "," << circle->center.y 
                      << ") radius " << circle->radius << " - transformed center: (" 
                      << transformedCenter.x << "," << transformedCenter.y 
                      << ") transformed radius: " << (circle->radius * canvas->getZoomLevel()) << std::endl;
            break;
        }
        case Drawing::ShapeType::TRIANGLE: {
            const auto* triangle = static_cast<const Drawing::Triangle*>(shape);
            
            ImVec2 transformedPoints[3];
            for (int i = 0; i < 3; ++i) {
                transformedPoints[i] = toImVec2(canvas->transformCoordinates(triangle->points[i]));
                // Add visible points at corners
                drawList->AddCircleFilled(transformedPoints[i], 3.0f, IM_COL32(0, 0, 0, 255));
            }
            
            // Draw triangle edges with increased thickness
            for (int i = 0; i < 3; ++i) {
                drawList->AddLine(
                    transformedPoints[i],
                    transformedPoints[(i + 1) % 3],
                    color,
                    std::max(triangle->thickness * canvas->getZoomLevel(), 2.0f)
                );
            }
            break;
        }
        case Drawing::ShapeType::SQUARE: {
            const auto* square = static_cast<const Drawing::Square*>(shape);
            glm::dvec2 topLeft = square->getTopLeft();
            float size = square->getSize();
            
            // Calculate transformed points for the square
            ImVec2 points[4] = {
                toImVec2(canvas->transformCoordinates(topLeft)),
                toImVec2(canvas->transformCoordinates(glm::dvec2(topLeft.x + size, topLeft.y))),
                toImVec2(canvas->transformCoordinates(glm::dvec2(topLeft.x + size, topLeft.y + size))),
                toImVec2(canvas->transformCoordinates(glm::dvec2(topLeft.x, topLeft.y + size)))
            };
            
            // Add visible points at corners
            for (int i = 0; i < 4; ++i) {
                drawList->AddCircleFilled(points[i], 3.0f, IM_COL32(0, 0, 0, 255));
            }
            
            // Draw square with thicker lines
            for (int i = 0; i < 4; ++i) {
                drawList->AddLine(
                    points[i],
                    points[(i + 1) % 4],
                    color,
                    std::max(square->thickness * canvas->getZoomLevel(), 2.0f)
                );
            }
            
            // Debug output
            std::cout << "Drawing square: topLeft (" << topLeft.x << "," << topLeft.y 
                      << ") size " << size << std::endl;
            break;
        }
        case Drawing::ShapeType::RECTANGLE: {
            const auto* rect = static_cast<const Drawing::Rectangle*>(shape);
            glm::dvec2 topLeft = rect->getTopLeft();
            glm::dvec2 size = rect->getSize();
            
            // Calculate transformed points for the rectangle
            ImVec2 points[4] = {
                toImVec2(canvas->transformCoordinates(topLeft)),
                toImVec2(canvas->transformCoordinates(glm::dvec2(topLeft.x + size.x, topLeft.y))),
                toImVec2(canvas->transformCoordinates(glm::dvec2(topLeft.x + size.x, topLeft.y + size.y))),
                toImVec2(canvas->transformCoordinates(glm::dvec2(topLeft.x, topLeft.y + size.y)))
            };
            
            // Add visible points at corners
            for (int i = 0; i < 4; ++i) {
                drawList->AddCircleFilled(points[i], 3.0f, IM_COL32(0, 0, 0, 255));
            }
            
            // Draw rectangle with thicker lines
            for (int i = 0; i < 4; ++i) {
                drawList->AddLine(
                    points[i],
                    points[(i + 1) % 4],
                    color,
                    std::max(rect->thickness * canvas->getZoomLevel(), 2.0f)
                );
            }
            
            // Debug output
            std::cout << "Drawing rectangle: topLeft (" << topLeft.x << "," << topLeft.y 
                      << ") size (" << size.x << "," << size.y << ")" << std::endl;
            break;
        }
        case Drawing::ShapeType::SPLINE: {
            const auto* spline = static_cast<const Drawing::Spline*>(shape);
            
            // Calculate spline points
            std::vector<glm::dvec2> points = spline->calculatePoints(0.01f);
            
            // Draw spline
            for (size_t i = 0; i < points.size() - 1; ++i) {
                drawList->AddLine(
                    toImVec2(canvas->transformCoordinates(points[i])),
                    toImVec2(canvas->transformCoordinates(points[i + 1])),
                    color,
                    std::max(spline->thickness * canvas->getZoomLevel(), 2.0f)
                );
            }
            
            // Draw control points if needed
            if (canvas->getShowControlPoints() || isSelected) {
                for (const auto& cp : spline->controlPoints) {
                    drawList->AddCircleFilled(Drawing::Math::toImVec2(canvas->transformCoordinates(cp)),
                        3.0f * canvas->getZoomLevel(),
                        Drawing::Colors::CONTROL_POINT,
                        8
                    );
                }
                
                // Draw control polygon
                for (size_t i = 0; i < spline->controlPoints.size() - 1; ++i) {
                    drawList->AddLine(Drawing::Math::toImVec2(canvas->transformCoordinates(spline->controlPoints[i])), Drawing::Math::toImVec2(canvas->transformCoordinates(spline->controlPoints[i + 1])),
                        Drawing::Colors::CONTROL_POINT,
                        1.0f * canvas->getZoomLevel()
                    );
                }
            }
            break;
        }
        case Drawing::ShapeType::BEZIER: {
            const auto* bezier = static_cast<const Drawing::BezierCurve*>(shape);
            
            if (bezier->controlPoints.size() < 4) {
                break;
            }
            
            // Draw the curve
            std::vector<glm::dvec2> points = bezier->calculatePoints(0.01f);
            
            for (size_t i = 0; i < points.size() - 1; ++i) {
                drawList->AddLine(
                    toImVec2(canvas->transformCoordinates(points[i])),
                    toImVec2(canvas->transformCoordinates(points[i + 1])),
                    color,
                    std::max(bezier->thickness * canvas->getZoomLevel(), 2.0f)
                );
            }
            
            // Draw control points if needed
            if (canvas->getShowControlPoints() || isSelected) {
                for (const auto& cp : bezier->controlPoints) {
                    drawList->AddCircleFilled(Drawing::Math::toImVec2(canvas->transformCoordinates(cp)),
                        3.0f * canvas->getZoomLevel(),
                        Drawing::Colors::CONTROL_POINT,
                        8
                    );
                }
                
                // Draw control polygon
                for (size_t i = 0; i < bezier->controlPoints.size() - 1; ++i) {
                    drawList->AddLine(
                        Drawing::Math::toImVec2(canvas->transformCoordinates(bezier->controlPoints[i])),
                        Drawing::Math::toImVec2(canvas->transformCoordinates(bezier->controlPoints[i + 1])),
                        Drawing::Colors::CONTROL_POINT,
                        1.0f * canvas->getZoomLevel()
                    );
                }
            }
            break;
        }
        case Drawing::ShapeType::SPRING2D: {
            const auto* spring = static_cast<const Drawing::Spring2D*>(shape);
            // Draw a 2D spring as a sine wave helix
            int pointsPerCoil = 40;
            int totalPoints = spring->numCoils * pointsPerCoil;
            float halfLength = spring->freeLength / 2.0f;
            float radius = spring->outerDiameter / 2.0f - spring->wireDiameter / 2.0f;
            std::vector<glm::dvec2> points;
            points.reserve(totalPoints);
            for (int i = 0; i < totalPoints; ++i) {
                float t = (float)i / (totalPoints - 1);
                float y = spring->centerY - halfLength + t * spring->freeLength;
                float angle = t * spring->numCoils * 2.0f * M_PI;
                float x = spring->centerX + radius * std::sin(angle);
                points.emplace_back(x, y);
            }
            for (int i = 0; i < (int)points.size() - 1; ++i) {
                ImVec2 p1 = toImVec2(canvas->transformCoordinates(points[i]));
                ImVec2 p2 = toImVec2(canvas->transformCoordinates(points[i + 1]));
                drawList->AddLine(p1, p2, color, std::max(spring->wireDiameter * canvas->getZoomLevel(), 2.0f));
            }
            break;
        }
        case Drawing::ShapeType::SHOCK_ABSORBER_END_2D: {
            const auto* end = static_cast<const Drawing::ShockAbsorberEnd2D*>(shape);
            float x = end->baseCenter.x;
            float od = end->parentSpring->outerDiameter;
            float wd = end->parentSpring->wireDiameter;
            float fl = end->parentSpring->freeLength;
            
            // New dimensions matching the 3D model reference
            float damperTubeWidth = wd * 1.2f;  // Thicker outer tube from top
            float pistonRodWidth = wd * 0.5f;  // Thin central rod from bottom
            float collarHeight = fl * 0.05f;
            float collarWidth = od * 0.5f;
            float nutHeight = fl * 0.08f;
            float nutWidth = od * 0.4f;
            float capHeight = fl * 0.06f;
            float capWidth = od * 0.35f;
            float springSeatWidth = od * 1.05f;
            float springSeatThickness = fl * 0.035f;
            
            if (end->position == Drawing::EndPosition::Top) {
                // Top end: spring seat, damper tube going down into spring, collar, nut, and cap going upward
                float springTop = end->parentSpring->centerY - fl / 2;
                float springBottom = end->parentSpring->centerY + fl / 2;
                
                // Spring seat (wide flange at top of spring)
                float seatBottom = springTop;
                float seatTop = seatBottom - springSeatThickness;
                glm::dvec2 seatL(x - springSeatWidth/2, seatTop);
                glm::dvec2 seatR(x + springSeatWidth/2, seatBottom);
                drawList->AddRect(Drawing::Math::toImVec2(canvas->transformCoordinates(seatL)), Drawing::Math::toImVec2(canvas->transformCoordinates(seatR)), end->color, 0, 0, end->thickness);
                
                // Damper tube (thicker tube extending DOWN into spring area - 70% into spring)
                float tubeTop = seatBottom;
                float tubeBottom = springTop + fl * 0.7f;  // Goes 70% down into spring
                glm::dvec2 tubeL(x - damperTubeWidth/2, tubeTop);
                glm::dvec2 tubeR(x + damperTubeWidth/2, tubeBottom);
                drawList->AddRect(Drawing::Math::toImVec2(canvas->transformCoordinates(tubeL)), Drawing::Math::toImVec2(canvas->transformCoordinates(tubeR)), IM_COL32(160,160,160,255), 0, 0, end->thickness);
                
                // Piston rod from bottom (thinner rod extending UP into spring area - shown as dashed or different color)
                // This represents the rod coming from the bottom mount
                float rodBottom = springBottom;
                float rodTop = springTop + fl * 0.3f;  // Goes 70% up into spring (overlaps with tube)
                glm::dvec2 rodL(x - pistonRodWidth/2, rodTop);
                glm::dvec2 rodR(x + pistonRodWidth/2, rodBottom);
                drawList->AddRect(Drawing::Math::toImVec2(canvas->transformCoordinates(rodL)), Drawing::Math::toImVec2(canvas->transformCoordinates(rodR)), IM_COL32(200,200,200,255), 0, 0, end->thickness);
                
                // Collar (wider section above spring seat, below nut)
                float collarBottom = seatTop;
                float collarTop = collarBottom - collarHeight;
                glm::dvec2 collarL(x - collarWidth/2, collarTop);
                glm::dvec2 collarR(x + collarWidth/2, collarBottom);
                drawList->AddRect(Drawing::Math::toImVec2(canvas->transformCoordinates(collarL)), Drawing::Math::toImVec2(canvas->transformCoordinates(collarR)), end->color, 0, 0, end->thickness);
                
                // Nut (hexagonal shape - in 2D shown as rectangle with diagonal lines)
                float nutBottom = collarTop;
                float nutTop = nutBottom - nutHeight;
                glm::dvec2 nutL(x - nutWidth/2, nutTop);
                glm::dvec2 nutR(x + nutWidth/2, nutBottom);
                drawList->AddRect(Drawing::Math::toImVec2(canvas->transformCoordinates(nutL)), Drawing::Math::toImVec2(canvas->transformCoordinates(nutR)), end->color, 0, 0, end->thickness);
                // Draw diagonal lines to indicate hex nut (cross-hatch pattern)
                float hexOffset = nutWidth * 0.3f;
                glm::dvec2 hexL1(x - hexOffset, nutTop);
                glm::dvec2 hexL2(x - nutWidth/2, nutTop + nutHeight * 0.3f);
                glm::dvec2 hexR1(x + hexOffset, nutTop);
                glm::dvec2 hexR2(x + nutWidth/2, nutTop + nutHeight * 0.3f);
                drawList->AddLine(Drawing::Math::toImVec2(canvas->transformCoordinates(hexL1)), Drawing::Math::toImVec2(canvas->transformCoordinates(hexL2)), end->color, end->thickness);
                drawList->AddLine(Drawing::Math::toImVec2(canvas->transformCoordinates(hexR1)), Drawing::Math::toImVec2(canvas->transformCoordinates(hexR2)), end->color, end->thickness);
                glm::dvec2 hexL3(x - hexOffset, nutBottom);
                glm::dvec2 hexL4(x - nutWidth/2, nutBottom - nutHeight * 0.3f);
                glm::dvec2 hexR3(x + hexOffset, nutBottom);
                glm::dvec2 hexR4(x + nutWidth/2, nutBottom - nutHeight * 0.3f);
                drawList->AddLine(Drawing::Math::toImVec2(canvas->transformCoordinates(hexL3)), Drawing::Math::toImVec2(canvas->transformCoordinates(hexL4)), end->color, end->thickness);
                drawList->AddLine(Drawing::Math::toImVec2(canvas->transformCoordinates(hexR3)), Drawing::Math::toImVec2(canvas->transformCoordinates(hexR4)), end->color, end->thickness);
                
                // Cross-bars/handles extending from nut (horizontal mounting points)
                float nutMid = (nutTop + nutBottom) / 2.0f;
                float barExtend = nutWidth * 1.5f;  // How far bars extend
                float barThick = nutHeight * 0.15f;
                // Left bar
                glm::dvec2 barLL(x - barExtend, nutMid - barThick);
                glm::dvec2 barLR(x - nutWidth/2, nutMid + barThick);
                drawList->AddRect(Drawing::Math::toImVec2(canvas->transformCoordinates(barLL)), Drawing::Math::toImVec2(canvas->transformCoordinates(barLR)), end->color, 0, 0, end->thickness);
                // Right bar
                glm::dvec2 barRL(x + nutWidth/2, nutMid - barThick);
                glm::dvec2 barRR(x + barExtend, nutMid + barThick);
                drawList->AddRect(Drawing::Math::toImVec2(canvas->transformCoordinates(barRL)), Drawing::Math::toImVec2(canvas->transformCoordinates(barRR)), end->color, 0, 0, end->thickness);
                
                // Cap (small cylinder on top)
                float capBottom = nutTop;
                float capTop = capBottom - capHeight;
                glm::dvec2 capL(x - capWidth/2, capTop);
                glm::dvec2 capR(x + capWidth/2, capBottom);
                drawList->AddRect(Drawing::Math::toImVec2(canvas->transformCoordinates(capL)), Drawing::Math::toImVec2(canvas->transformCoordinates(capR)), end->color, 0, 0, end->thickness);
            } else {
                // Bottom end: currently handled by ShockAbsorberBottomEnd
                // Draw stepped shaft (section view, extends downward from bottom of spring)
                float y0 = end->baseCenter.y - end->shaftLength / 2.0f;
                float y1 = y0 + end->step1Length;
                float y2 = y1 + end->step2Length;
                float y3 = y2 + end->step3Length;
                // Step 1 (top, largest)
                glm::dvec2 s1L(x - end->step1Diameter/2, y0);
                glm::dvec2 s1R(x + end->step1Diameter/2, y0);
                glm::dvec2 s1L2(x - end->step1Diameter/2, y1);
                glm::dvec2 s1R2(x + end->step1Diameter/2, y1);
                drawList->AddRect(Drawing::Math::toImVec2(canvas->transformCoordinates(s1L)), Drawing::Math::toImVec2(canvas->transformCoordinates(s1R2)), end->color, 0, 0, end->thickness);
                // Step 2 (middle, spring seat)
                glm::dvec2 s2L(x - end->step2Diameter/2, y1);
                glm::dvec2 s2R(x + end->step2Diameter/2, y1);
                glm::dvec2 s2L2(x - end->step2Diameter/2, y2);
                glm::dvec2 s2R2(x + end->step2Diameter/2, y2);
                drawList->AddRect(Drawing::Math::toImVec2(canvas->transformCoordinates(s2L)), Drawing::Math::toImVec2(canvas->transformCoordinates(s2R2)), end->color, 0, 0, end->thickness);
                // Step 3 (bottom, smallest)
                glm::dvec2 s3L(x - end->step3Diameter/2, y2);
                glm::dvec2 s3R(x + end->step3Diameter/2, y2);
                glm::dvec2 s3L2(x - end->step3Diameter/2, y3);
                glm::dvec2 s3R2(x + end->step3Diameter/2, y3);
                drawList->AddRect(Drawing::Math::toImVec2(canvas->transformCoordinates(s3L)), Drawing::Math::toImVec2(canvas->transformCoordinates(s3R2)), end->color, 0, 0, end->thickness);
                // Central bore (section view: vertical rectangle)
                float boreX1 = x - end->boreDiameter/2;
                float boreX2 = x + end->boreDiameter/2;
                glm::dvec2 boreL(boreX1, y0);
                glm::dvec2 boreR(boreX2, y3);
                drawList->AddRect(Drawing::Math::toImVec2(canvas->transformCoordinates(boreL)), Drawing::Math::toImVec2(canvas->transformCoordinates(boreR)), IM_COL32(200,200,200,255), 0, 0, end->thickness);
                // Chamfers (draw as lines at ends)
                glm::dvec2 chamferL1(x - end->step1Diameter/2, y0);
                glm::dvec2 chamferL2(x - end->step1Diameter/2 + end->chamfer, y0 + end->chamfer);
                glm::dvec2 chamferR1(x + end->step1Diameter/2, y0);
                glm::dvec2 chamferR2(x + end->step1Diameter/2 - end->chamfer, y0 + end->chamfer);
                drawList->AddLine(Drawing::Math::toImVec2(canvas->transformCoordinates(chamferL1)), Drawing::Math::toImVec2(canvas->transformCoordinates(chamferL2)), end->color, end->thickness);
                drawList->AddLine(Drawing::Math::toImVec2(canvas->transformCoordinates(chamferR1)), Drawing::Math::toImVec2(canvas->transformCoordinates(chamferR2)), end->color, end->thickness);
            }
            break;
        }
        case Drawing::ShapeType::SHOCK_ABSORBER_BOTTOM_END: {
            const auto* bottomEnd = static_cast<const Drawing::ShockAbsorberBottomEnd*>(shape);
            bottomEnd->draw(drawList, canvas);
            break;
        }
        case Drawing::ShapeType::BELLOWS: {
            const auto* bellows = static_cast<const Drawing::Bellows*>(shape);
            
            // Skip if not valid
            if (!bellows->isValid()) break;
            
            // Get profile points and transform them
            const std::vector<glm::dvec2>& profilePoints = bellows->getCachedProfile();
            std::vector<glm::dvec2> transformedPoints;
            transformedPoints.reserve(profilePoints.size());
            
            float s = sin(bellows->angle);
            float c = cos(bellows->angle);
            
            for (const auto& point : profilePoints) {
                float rotatedX = point.x * c - point.y * s;
                float rotatedY = point.x * s + point.y * c;
                glm::dvec2 translatedPoint(
                    bellows->position.x + rotatedX,
                    bellows->position.y + rotatedY
                );
                transformedPoints.push_back(translatedPoint);
            }
            
            // Use appropriate color and thickness
            ImU32 profileColor = bellows->isSelected ? IM_COL32(255, 255, 0, 255) : color;
            float profileThickness = std::max(bellows->thickness * canvas->getZoomLevel(), 2.0f);
            
            // Draw the bellows profile
            for (size_t i = 0; i < transformedPoints.size() - 1; ++i) {
                ImVec2 p1 = toImVec2(canvas->transformCoordinates(transformedPoints[i]));
                ImVec2 p2 = toImVec2(canvas->transformCoordinates(transformedPoints[i + 1]));
                drawList->AddLine(p1, p2, profileColor, profileThickness);
            }
            break;
        }
        case Drawing::ShapeType::BALL_BEARING: {
            const auto* ballBearing = static_cast<const Drawing::BallBearing*>(shape);
            
            // Skip if not valid
            if (!ballBearing->isValid()) break;
            
            // Transform center position
            ImVec2 transformedCenter = toImVec2(canvas->transformCoordinates(ballBearing->position));
            
            float outerRadius = (ballBearing->outerDiameter / 2.0f) * canvas->getZoomLevel();
            float innerRadius = (ballBearing->innerDiameter / 2.0f) * canvas->getZoomLevel();
            
            // Determine colors based on selection
            ImU32 outerColor = ballBearing->isSelected ? IM_COL32(255, 255, 0, 255) : color;
            ImU32 innerColor = ballBearing->isSelected ? IM_COL32(255, 255, 0, 255) : IM_COL32(100, 100, 100, 255);
            
            // Draw outer race
            drawList->AddCircle(transformedCenter, outerRadius, outerColor, 64, std::max(ballBearing->thickness * canvas->getZoomLevel(), 2.0f));
            
            // Draw inner race
            drawList->AddCircle(transformedCenter, innerRadius, innerColor, 64, std::max(ballBearing->thickness * canvas->getZoomLevel(), 1.0f));
            
            // Draw balls if enabled
            if (ballBearing->showBalls) {
                float pitchRadius = (outerRadius + innerRadius) / 2.0f;
                float ballRadius = (ballBearing->ballDiameter / 2.0f) * canvas->getZoomLevel();
                
                ImU32 ballColor = ballBearing->isSelected ? IM_COL32(255, 255, 150, 255) : IM_COL32(150, 150, 150, 255);
                
                for (int i = 0; i < ballBearing->numBalls; ++i) {
                    float ballAngle = (float)i / ballBearing->numBalls * 2.0f * M_PI;
                    ImVec2 ballPos = ImVec2(
                        transformedCenter.x + pitchRadius * cos(ballAngle),
                        transformedCenter.y + pitchRadius * sin(ballAngle)
                    );
                    drawList->AddCircleFilled(ballPos, ballRadius, ballColor);
                }
            }
            
            // Draw cage if enabled
            if (ballBearing->showCage) {
                float cageRadius = (outerRadius + innerRadius) / 2.0f;
                ImU32 cageColor = ballBearing->isSelected ? IM_COL32(255, 255, 100, 180) : IM_COL32(200, 200, 200, 180);
                drawList->AddCircle(transformedCenter, cageRadius, cageColor, 32, 1.0f);
            }
            break;
        }
        default:
            break;
    }
}
void Renderer2D::previewBellows(ImDrawList* drawList, const glm::dvec2& start, const glm::dvec2& end) {
    // Calculate length and orientation for preview
    float dx = end.x - start.x;
    float dy = end.y - start.y;
    float length = std::sqrt(dx * dx + dy * dy);
    float angle = std::atan2(dy, dx);
    
    if (length < 1.0f) return;
    
    // Create temporary bellows for preview
    Drawing::Bellows previewBellows;
    previewBellows.convolutedSectionLength = length - previewBellows.cuffALength - previewBellows.cuffBLength;
    if (previewBellows.convolutedSectionLength < 0.0f) previewBellows.convolutedSectionLength = 0.0f;
    
    // Calculate sin and cos for rotation
    float s = sin(angle);
    float c = cos(angle);
    
    // Get profile points
    std::vector<glm::dvec2> profilePoints = previewBellows.generateProfile();
    
    // Draw profile with preview color
    for (size_t i = 0; i < profilePoints.size() - 1; ++i) {
        // Rotate and translate the points
        float rotatedX1 = profilePoints[i].x * c - profilePoints[i].y * s;
        float rotatedY1 = profilePoints[i].x * s + profilePoints[i].y * c;
        
        float rotatedX2 = profilePoints[i+1].x * c - profilePoints[i+1].y * s;
        float rotatedY2 = profilePoints[i+1].x * s + profilePoints[i+1].y * c;
        
        glm::dvec2 p1 = glm::dvec2(
            start.x + rotatedX1,
            start.y + rotatedY1
        );
        glm::dvec2 p2 = glm::dvec2(
            start.x + rotatedX2,
            start.y + rotatedY2
        );
        
        // Convert to screen coordinates
        drawList->AddLine(Drawing::Math::toImVec2(canvas->transformCoordinates(p1)), Drawing::Math::toImVec2(canvas->transformCoordinates(p2)), Drawing::Colors::PREVIEW, 1.0f);
    }
}
void Renderer2D::previewPoint(ImDrawList* drawList, const glm::dvec2& pos) {
    ImVec2 transformedPos = toImVec2(canvas->transformCoordinates(pos));
    drawList->AddCircleFilled(transformedPos, Drawing::Constants::DEFAULT_POINT_SIZE * canvas->getZoomLevel(), Drawing::Colors::PREVIEW);
}
void Renderer2D::previewLine(ImDrawList* drawList, const glm::dvec2& start, const glm::dvec2& end) {
    // Transform coordinates to screen space
    ImVec2 transformedStart = toImVec2(canvas->transformCoordinates(start));
    ImVec2 transformedEnd = toImVec2(canvas->transformCoordinates(end));
    
    // Draw the line
    drawList->AddLine(
        transformedStart, 
        transformedEnd, 
        Drawing::Colors::PREVIEW, 
        Drawing::Constants::DEFAULT_LINE_THICKNESS * canvas->getZoomLevel()
    );
    
    // Draw small circles at endpoints for better visibility
    drawList->AddCircleFilled(
        transformedStart,
        4.0f * canvas->getZoomLevel(),
        Drawing::Colors::PREVIEW,
        8
    );
    
    drawList->AddCircleFilled(
        transformedEnd,
        4.0f * canvas->getZoomLevel(),
        Drawing::Colors::PREVIEW,
        8
    );
}
void Renderer2D::previewCircle(ImDrawList* drawList, const glm::dvec2& center, float radius) {
    // Transform coordinates to screen space
    ImVec2 transformedCenter = toImVec2(canvas->transformCoordinates(center));
    
    // Draw the circle
    drawList->AddCircle(
        transformedCenter,
        radius * canvas->getZoomLevel(),
        Drawing::Colors::PREVIEW,
        128, // Increased from default to make circle smoother
        Drawing::Constants::DEFAULT_LINE_THICKNESS * canvas->getZoomLevel()
    );
    
    // Draw the center point for better visibility
    drawList->AddCircleFilled(
        transformedCenter,
        4.0f * canvas->getZoomLevel(),
        Drawing::Colors::PREVIEW,
        8
    );
    
    // Draw radius line for better visualization
    ImVec2 radiusPoint = Drawing::Math::toImVec2(canvas->transformCoordinates(glm::dvec2(
        center.x + radius, 
        center.y
    )));
    
    drawList->AddLine(
        transformedCenter,
        radiusPoint,
        Drawing::Colors::PREVIEW_LIGHT,
        1.0f * canvas->getZoomLevel()
    );
    
    // Draw a small circle at the radius point
    drawList->AddCircleFilled(
        radiusPoint,
        3.0f * canvas->getZoomLevel(),
        Drawing::Colors::PREVIEW,
        8
    );
}
void Renderer2D::previewTriangle(ImDrawList* drawList, const std::array<glm::dvec2, 3>& points, int count) {
    // Transform all points first
    std::array<ImVec2, 3> transformedPoints;
    for (int i = 0; i < count; ++i) {
        transformedPoints[i] = toImVec2(canvas->transformCoordinates(points[i]));
    }
    
    // Draw completed lines
    for (int i = 0; i < count - 1; ++i) {
        drawList->AddLine(
            transformedPoints[i],
            transformedPoints[i + 1],
            Drawing::Colors::PREVIEW,
            Drawing::Constants::DEFAULT_LINE_THICKNESS * canvas->getZoomLevel()
        );
    }
    
    // Draw preview line to current mouse position if not complete
    if (count > 0 && count < 3) {
        ImVec2 mousePos = Drawing::Math::toImVec2(canvas->transformCoordinates(canvas->inverseTransformCoordinates(Drawing::Math::toDVec2(ImGui::GetMousePos()))));
        drawList->AddLine(
            transformedPoints[count - 1],
            mousePos,
            Drawing::Colors::PREVIEW_LIGHT,
            Drawing::Constants::DEFAULT_LINE_THICKNESS * canvas->getZoomLevel()
        );
    }
    
    // Draw closing line if all points are placed
    if (count == 3) {
        drawList->AddLine(
            transformedPoints[2],
            transformedPoints[0],
            Drawing::Colors::PREVIEW,
            Drawing::Constants::DEFAULT_LINE_THICKNESS * canvas->getZoomLevel()
        );
    }
    
    // Draw corner points for better visibility
    for (int i = 0; i < count; ++i) {
        drawList->AddCircleFilled(
            transformedPoints[i],
            4.0f * canvas->getZoomLevel(),
            Drawing::Colors::PREVIEW,
            8
        );
    }
    
    // Draw midpoints of edges for better visualization
    for (int i = 0; i < count; ++i) {
        ImVec2 midpoint = {
            (transformedPoints[i].x + transformedPoints[(i + 1) % count].x) * 0.5f,
            (transformedPoints[i].y + transformedPoints[(i + 1) % count].y) * 0.5f
        };
        drawList->AddCircleFilled(
            midpoint,
            3.0f * canvas->getZoomLevel(),
            Drawing::Colors::PREVIEW_LIGHT,
            8
        );
    }
}
void Renderer2D::previewSquare(ImDrawList* drawList, const glm::dvec2& start, const glm::dvec2& end) {
    // Calculate the size based on the larger of width or height
    float size = std::max(
        std::abs(end.x - start.x),
        std::abs(end.y - start.y)
    );
    
    // Calculate signs for direction
    float signX = (end.x >= start.x) ? 1.0f : -1.0f;
    float signY = (end.y >= start.y) ? 1.0f : -1.0f;
    
    // Calculate corners in world space
    glm::dvec2 p1 = start;
    glm::dvec2 p2 = glm::dvec2(start.x + size * signX, start.y);
    glm::dvec2 p3 = glm::dvec2(start.x + size * signX, start.y + size * signY);
    glm::dvec2 p4 = glm::dvec2(start.x, start.y + size * signY);
    
    // Transform to screen space
    ImVec2 transformedPoints[4] = {
        Drawing::Math::toImVec2(canvas->transformCoordinates(p1)),
        Drawing::Math::toImVec2(canvas->transformCoordinates(p2)),
        Drawing::Math::toImVec2(canvas->transformCoordinates(p3)),
        Drawing::Math::toImVec2(canvas->transformCoordinates(p4))
    };
    
    // Draw the square outline
    drawList->AddPolyline(transformedPoints, 4, Drawing::Colors::PREVIEW, ImDrawFlags_Closed, 2.0f * canvas->getZoomLevel());
    
    // Draw corner points for better visibility
    for (const auto& point : transformedPoints) {
        drawList->AddCircleFilled(point, 4.0f * canvas->getZoomLevel(), Drawing::Colors::PREVIEW, 8);
    }
    
    // Draw diagonal line for better visualization
    drawList->AddLine(
        transformedPoints[0],
        transformedPoints[2],
        Drawing::Colors::PREVIEW_LIGHT,
        1.0f * canvas->getZoomLevel()
    );
}
void Renderer2D::previewRectangle(ImDrawList* drawList, const glm::dvec2& start, const glm::dvec2& end) {
    // Calculate width and height
    float width = std::abs(end.x - start.x);
    float height = std::abs(end.y - start.y);
    
    // Calculate signs for direction
    float signX = (end.x >= start.x) ? 1.0f : -1.0f;
    float signY = (end.y >= start.y) ? 1.0f : -1.0f;
    
    // Calculate corners in world space
    glm::dvec2 p1 = start;
    glm::dvec2 p2 = glm::dvec2(start.x + width * signX, start.y);
    glm::dvec2 p3 = glm::dvec2(start.x + width * signX, start.y + height * signY);
    glm::dvec2 p4 = glm::dvec2(start.x, start.y + height * signY);
    
    // Transform to screen space
    ImVec2 transformedPoints[4] = {
        Drawing::Math::toImVec2(canvas->transformCoordinates(p1)),
        Drawing::Math::toImVec2(canvas->transformCoordinates(p2)),
        Drawing::Math::toImVec2(canvas->transformCoordinates(p3)),
        Drawing::Math::toImVec2(canvas->transformCoordinates(p4))
    };
    
    // Draw the rectangle outline
    drawList->AddPolyline(transformedPoints, 4, Drawing::Colors::PREVIEW, ImDrawFlags_Closed, 2.0f * canvas->getZoomLevel());
    
    // Draw corner points for better visibility
    for (const auto& point : transformedPoints) {
        drawList->AddCircleFilled(point, 4.0f * canvas->getZoomLevel(), Drawing::Colors::PREVIEW, 8);
    }
    
    // Draw diagonal line for better visualization
    drawList->AddLine(
        transformedPoints[0],
        transformedPoints[2],
        Drawing::Colors::PREVIEW_LIGHT,
        1.0f * canvas->getZoomLevel()
    );
}
void Renderer2D::previewSpline(ImDrawList* drawList, const std::vector<glm::dvec2>& points) {
    if (points.empty()) return;

    // Draw control points
    for (const auto& point : points) {
        ImVec2 transformed = toImVec2(canvas->transformCoordinates(point));
        drawList->AddCircleFilled(transformed, 4.0f, IM_COL32(255, 255, 255, 255));
        drawList->AddCircle(transformed, 5.0f, IM_COL32(0, 0, 0, 255));
    }
    
    // Draw control polygon with dashed lines
    for (size_t i = 0; i < points.size() - 1; ++i) {
        ImVec2 transformed1 = toImVec2(canvas->transformCoordinates(points[i]));
        ImVec2 transformed2 = toImVec2(canvas->transformCoordinates(points[i + 1]));
        drawDashedLine(drawList, transformed1, transformed2, IM_COL32(128, 128, 128, 200), 
                      1.0f, 5.0f);
    }
    
    // Draw spline preview if we have enough points
    if (points.size() >= 2) {
        // Create temporary points array including current mouse position
        std::vector<glm::dvec2> previewPoints = points;
        glm::dvec2 mousePos = canvas->inverseTransformCoordinates(Drawing::Math::toDVec2(ImGui::GetMousePos()));
        previewPoints.push_back(mousePos);
        
        // Calculate and draw the preview curve
        std::vector<glm::dvec2> curvePoints = canvas->calculateSplinePoints(previewPoints, false);
        for (size_t i = 1; i < curvePoints.size(); ++i) {
            ImVec2 transformed1 = toImVec2(canvas->transformCoordinates(curvePoints[i-1]));
            ImVec2 transformed2 = toImVec2(canvas->transformCoordinates(curvePoints[i]));
            drawList->AddLine(transformed1, transformed2, 
                            IM_COL32(255, 255, 255, 200), 
                            2.0f * canvas->getZoomLevel());
        }
    }
}
void Renderer2D::previewBezier(ImDrawList* drawList, const std::vector<glm::dvec2>& points) {
    // Draw control points
    for (const auto& point : points) {
        ImVec2 transformed = toImVec2(canvas->transformCoordinates(point));
        Drawing::CurveUI::drawControlPoint(drawList, transformed, false);
    }
    
    // Draw control polygon with dashed lines
    for (size_t i = 0; i < points.size() - 1; ++i) {
        ImVec2 transformed1 = toImVec2(canvas->transformCoordinates(points[i]));
        ImVec2 transformed2 = toImVec2(canvas->transformCoordinates(points[i + 1]));
        drawDashedLine(drawList, transformed1, transformed2, Drawing::Colors::PREVIEW_LIGHT, 
                      1.0f * canvas->getZoomLevel(), 5.0f * canvas->getZoomLevel());
    }
    
    // Draw curve preview
    if (points.size() >= 2) {
        std::vector<glm::dvec2> previewPoints = points;
        if (points.size() < 4) {
            // Add current mouse position and any needed extra points for preview
            glm::dvec2 mousePos = canvas->inverseTransformCoordinates(Drawing::Math::toDVec2(ImGui::GetMousePos()));
            previewPoints.push_back(mousePos);
            
            // For incomplete Bezier curves, duplicate last point as needed
            while (previewPoints.size() < 4) {
                previewPoints.push_back(previewPoints.back());
            }
        }
        
        // Calculate and draw the preview curve
        Drawing::BezierCurve tempBezier(previewPoints);
        std::vector<glm::dvec2> curvePoints = tempBezier.calculatePoints();
        
        for (size_t i = 0; i < curvePoints.size() - 1; ++i) {
            ImVec2 transformed1 = toImVec2(canvas->transformCoordinates(curvePoints[i]));
            ImVec2 transformed2 = toImVec2(canvas->transformCoordinates(curvePoints[i + 1]));
            drawList->AddLine(transformed1, transformed2, Drawing::Colors::PREVIEW, 
                            Drawing::Constants::DEFAULT_LINE_THICKNESS * canvas->getZoomLevel());
        }
    }
}
void Renderer2D::previewBallBearing(ImDrawList* drawList, const glm::dvec2& center, float radius) {
    if (!canvas->getIsDrawing() || radius <= 0.0f) return;
    
    ImVec2 transformedCenter = toImVec2(canvas->transformCoordinates(center));
    float transformedRadius = radius * canvas->getZoomLevel();
    
    // Draw preview of outer circle
    drawList->AddCircle(transformedCenter, transformedRadius, Drawing::Colors::PREVIEW, 32, 2.0f);
    
    // Draw preview of inner circle
    float innerRadius = transformedRadius * 0.6f;
    drawList->AddCircle(transformedCenter, innerRadius, Drawing::Colors::PREVIEW, 32, 1.0f);
    
    // Draw preview balls
    int numBalls = 8;
    float pitchRadius = (transformedRadius + innerRadius) / 2.0f;
    float ballRadius = (transformedRadius - innerRadius) / 8.0f;
    
    for (int i = 0; i < numBalls; ++i) {
        float ballAngle = (float)i / numBalls * 2.0f * M_PI;
        ImVec2 ballPos = ImVec2(
            transformedCenter.x + pitchRadius * cos(ballAngle),
            transformedCenter.y + pitchRadius * sin(ballAngle)
        );
        drawList->AddCircleFilled(ballPos, ballRadius, Drawing::Colors::PREVIEW);
    }
}
void Renderer2D::previewSpring2D(ImDrawList* drawList, const glm::dvec2& center) {
    // Draw a preview of the spring at the given center using current parameters
    int pointsPerCoil = 40;
    int totalPoints = canvas->springNumCoils * pointsPerCoil;
    float halfLength = canvas->springFreeLength / 2.0f;
    float radius = canvas->springOuterDiameter / 2.0f - canvas->springWireDiameter / 2.0f;
    std::vector<glm::dvec2> points;
    points.reserve(totalPoints);
    for (int i = 0; i < totalPoints; ++i) {
        float t = (float)i / (totalPoints - 1);
        float y = center.y - halfLength + t * canvas->springFreeLength;
        float angle = t * canvas->springNumCoils * 2.0f * M_PI;
        float x = center.x + radius * std::sin(angle);
        points.emplace_back(x, y);
    }
    for (int i = 0; i < (int)points.size() - 1; ++i) {
        ImVec2 p1 = toImVec2(canvas->transformCoordinates(points[i]));
        ImVec2 p2 = toImVec2(canvas->transformCoordinates(points[i + 1]));
        drawList->AddLine(p1, p2, Drawing::Colors::PREVIEW, std::max(canvas->springWireDiameter * canvas->getZoomLevel(), 2.0f));
    }
    // Removed top and bottom arcs for a cleaner preview
}
void Renderer2D::renderPreview(ImDrawList* drawList, const glm::dvec2& currentPos, Drawing::DrawingMode mode) {
    switch (mode) {
        case Drawing::DrawingMode::Point:
            previewPoint(drawList, currentPos);
            break;
        case Drawing::DrawingMode::Line:
            if (canvas->getIsDrawing()) {
                previewLine(drawList, canvas->getStartPoint(), currentPos);
            }
            break;
        case Drawing::DrawingMode::Circle:
            if (canvas->getIsDrawing()) {
                float radius = Drawing::Math::calculateDistance(canvas->getStartPoint(), currentPos);
                previewCircle(drawList, canvas->getStartPoint(), radius);
            }
            break;
        case Drawing::DrawingMode::Triangle:
            if (canvas->getIsDrawing()) {
                previewTriangle(drawList, canvas->getTrianglePoints(), canvas->getClickCount());
            }
            break;
        case Drawing::DrawingMode::Square:
            if (canvas->getIsDrawing()) {
                previewSquare(drawList, canvas->getStartPoint(), currentPos);
            }
            break;
        case Drawing::DrawingMode::Rectangle:
            if (canvas->getIsDrawing()) {
                previewRectangle(drawList, canvas->getStartPoint(), currentPos);
            }
            break;
        case Drawing::DrawingMode::Spline:
            previewSpline(drawList, canvas->getCurrentSplinePoints());
            break;
        case Drawing::DrawingMode::BezierCurve:
            previewBezier(drawList, canvas->getCurrentCurvePoints());
            break;
        case Drawing::DrawingMode::Bellows:
            if (canvas->getIsDrawing()) {
                previewBellows(drawList, canvas->getStartPoint(), currentPos);
            }
            break;
            
        case Drawing::DrawingMode::BallBearing:
            if (canvas->getIsDrawing()) {
                float dx = currentPos.x - canvas->getStartPoint().x;
                float dy = currentPos.y - canvas->getStartPoint().y;
                float radius = std::sqrt(dx * dx + dy * dy);
                previewBallBearing(drawList, canvas->getStartPoint(), radius);
            }
            break;
        default:
            break;
    }
}
} // namespace Core

void Core::Renderer2D::drawDashedLine(ImDrawList* drawList, const ImVec2& p1, const ImVec2& p2, 
                   ImU32 color, float thickness, float dash_length) {
    ImVec2 direction = {p2.x - p1.x, p2.y - p1.y};
    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    
    if (length < 0.0001f) return;
    
    direction.x /= length;
    direction.y /= length;
    
    bool draw = true;
    ImVec2 current = p1;
    float remaining = length;
    
    while (remaining > 0) {
        float segment = std::min(dash_length, remaining);
        ImVec2 next = {
            current.x + direction.x * segment,
            current.y + direction.y * segment
        };
        
        if (draw) {
            drawList->AddLine(current, next, color, thickness);
        }
        
        current = next;
        remaining -= segment;
        draw = !draw;
    }
}

void Core::Renderer2D::renderSnapIndicator(ImDrawList* drawList, const glm::dvec2& pos, const std::string& type) {
    ImVec2 transformed = toImVec2(canvas->transformCoordinates(pos));
    float zoomLevel = canvas->getZoomLevel();
    float size = 5.0f * zoomLevel;
    
    // Choose color based on snap point type
    ImU32 color;
    if (type == "Grid") {
        color = IM_COL32(100, 200, 100, 255); // Green for grid snaps
    } else if (type.find("Endpoint") != std::string::npos || 
               type.find("Vertex") != std::string::npos || 
               type.find("Point") != std::string::npos) {
        color = IM_COL32(240, 200, 40, 255);  // Yellow/gold for points
    } else if (type.find("Midpoint") != std::string::npos) {
        color = IM_COL32(80, 180, 240, 255);  // Blue for midpoints
    } else if (type.find("Center") != std::string::npos) {
        color = IM_COL32(240, 100, 240, 255); // Purple for centers
    } else if (type.find("North") != std::string::npos || 
               type.find("South") != std::string::npos || 
               type.find("East") != std::string::npos || 
               type.find("West") != std::string::npos) {
        color = IM_COL32(240, 120, 80, 255);  // Orange for cardinal points
    } else {
        color = IM_COL32(200, 200, 200, 255); // Default white for other types
    }
    
    // Draw different visual indicators based on type
    if (type == "Grid") {
        // Grid snap: rectangular marker
        drawList->AddRect(
            ImVec2(transformed.x - size, transformed.y - size),
            ImVec2(transformed.x + size, transformed.y + size),
            color,
            0.0f,
            ImDrawFlags_None,
            2.0f * (zoomLevel > 1.0f ? 1.0f : zoomLevel)
        );
    } else if (type.find("Endpoint") != std::string::npos || 
               type.find("Vertex") != std::string::npos || 
               type.find("Point") != std::string::npos) {
        // Point snap: diamond marker
        float diamondSize = size * 1.2f;
        drawList->AddQuad(
            ImVec2(transformed.x, transformed.y - diamondSize),
            ImVec2(transformed.x + diamondSize, transformed.y),
            ImVec2(transformed.x, transformed.y + diamondSize),
            ImVec2(transformed.x - diamondSize, transformed.y),
            color,
            2.0f * (zoomLevel > 1.0f ? 1.0f : zoomLevel)
        );
    } else if (type.find("Midpoint") != std::string::npos) {
        // Midpoint snap: cross in circle
        drawList->AddCircle(
            transformed,
            size * 1.2f,
            color,
            0,
            2.0f * (zoomLevel > 1.0f ? 1.0f : zoomLevel)
        );
        // Add cross inside
        drawList->AddLine(
            ImVec2(transformed.x - size, transformed.y),
            ImVec2(transformed.x + size, transformed.y),
            color,
            2.0f * (zoomLevel > 1.0f ? 1.0f : zoomLevel)
        );
        drawList->AddLine(
            ImVec2(transformed.x, transformed.y - size),
            ImVec2(transformed.x, transformed.y + size),
            color,
            2.0f * (zoomLevel > 1.0f ? 1.0f : zoomLevel)
        );
    } else if (type.find("Center") != std::string::npos) {
        // Center snap: concentric circles
        drawList->AddCircle(
            transformed,
            size * 1.5f,
            color,
            0,
            2.0f * (zoomLevel > 1.0f ? 1.0f : zoomLevel)
        );
        drawList->AddCircle(
            transformed,
            size * 0.7f,
            color,
            0,
            2.0f * (zoomLevel > 1.0f ? 1.0f : zoomLevel)
        );
    } else {
        // Default: cross
        drawList->AddLine(
            ImVec2(transformed.x - size, transformed.y),
            ImVec2(transformed.x + size, transformed.y),
            color,
            2.0f * (zoomLevel > 1.0f ? 1.0f : zoomLevel)
        );
        drawList->AddLine(
            ImVec2(transformed.x, transformed.y - size),
            ImVec2(transformed.x, transformed.y + size),
            color,
            2.0f * (zoomLevel > 1.0f ? 1.0f : zoomLevel)
        );
    }
}
