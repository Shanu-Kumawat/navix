#include "meshing/Mesh.hpp"

namespace Core {
namespace Meshing {

void Mesh::addNode(uint64_t tag, double x, double y, double z) {
    nodes.emplace_back(tag, x, y, z);
}

void Mesh::addElement(uint64_t tag, int elementType, const std::vector<uint64_t>& nodeTags) {
    elements.emplace_back(tag, elementType, nodeTags);
}

void Mesh::clear() {
    nodes.clear();
    elements.clear();
}

bool Mesh::isEmpty() const {
    return nodes.empty() && elements.empty();
}

} // namespace Meshing
} // namespace Core
