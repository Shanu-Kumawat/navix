#pragma once

#include <vector>
#include <glm/glm.hpp>

namespace Core {
namespace Meshing {

/**
 * @brief Represents a generated finite element node.
 */
struct MeshNode {
    uint64_t tag;
    glm::dvec3 position;

    MeshNode(uint64_t t, double x, double y, double z)
        : tag(t), position(x, y, z) {}
};

/**
 * @brief Represents a generated finite element (e.g. Triangle, Quad, Line).
 */
struct MeshElement {
    uint64_t tag;
    int elementType; // Gmsh element type (e.g., 1=line, 2=triangle, 3=quad)
    std::vector<uint64_t> nodeTags;

    MeshElement(uint64_t t, int type, const std::vector<uint64_t>& nodes)
        : tag(t), elementType(type), nodeTags(nodes) {}
};

/**
 * @brief Stores the structured results from a Gmsh mesh generation.
 * This class decouples the raw Gmsh generation state into an accessible dataset for FEA processing.
 */
class Mesh {
public:
    Mesh() = default;
    ~Mesh() = default;

    void addNode(uint64_t tag, double x, double y, double z);
    void addElement(uint64_t tag, int elementType, const std::vector<uint64_t>& nodeTags);

    const std::vector<MeshNode>& getNodes() const { return nodes; }
    const std::vector<MeshElement>& getElements() const { return elements; }

    void clear();

    bool isEmpty() const;

private:
    std::vector<MeshNode> nodes;
    std::vector<MeshElement> elements;
};

} // namespace Meshing
} // namespace Core
