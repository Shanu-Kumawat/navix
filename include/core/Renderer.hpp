#pragma once

#include "meshing/Mesh.hpp"

#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

namespace Drawing {
    struct Shape;
}

namespace Core {
namespace Topology {
    class Node;
    class Edge;
    class Face;
}

/**
 * @brief Abstract Interface for graphics pipeline
 * 
 * Provides a unified set of functions that map generic rendering calls
 * (e.g. DrawLine, DrawPolygon) to explicit backends (e.g. ImGui, OpenGL).
 */
class Renderer {
public:
    virtual ~Renderer() = default;

    // Viewport and Transform Management
    virtual void setTransform(const glm::dvec2& pan, float zoom, const glm::dvec2& windowPos) = 0;
    
    // Core Primitive Drawing
    virtual void drawLine(const glm::dvec2& start, const glm::dvec2& end, uint32_t color, float thickness) = 0;
    virtual void drawDashedLine(const glm::dvec2& start, const glm::dvec2& end, uint32_t color, float thickness, float dashLength) = 0;
    virtual void drawCircle(const glm::dvec2& center, float radius, uint32_t color, float thickness, bool fill = false) = 0;
    virtual void drawPolygon(const std::vector<glm::dvec2>& points, uint32_t color, float thickness, bool fill = false) = 0;
    virtual void drawText(const glm::dvec2& pos, const char* text, uint32_t color) = 0;

    // Specialized Rendering pipelines
    virtual void drawShape(const Drawing::Shape* shape, bool isSelected) = 0;
    
    // FEA / Topological Pipeline
    virtual void drawNode(const Topology::Node* node, bool isSelected = false) = 0;
    virtual void drawEdge(const Topology::Edge* edge, const Topology::Node* n1, const Topology::Node* n2, bool isSelected = false) = 0;
    // Mesh Drawing Function
    virtual void drawMesh(const Meshing::Mesh& mesh, const glm::dvec3& color) = 0;
};

} // namespace Core
