#include "topology/TopologyManager.hpp"
#include <algorithm>

namespace Core {
namespace Topology {

std::shared_ptr<Node> TopologyManager::createNode(double x, double y, double z) {
    auto id = generateId();
    auto node = std::make_shared<Node>(id, x, y, z);
    nodes[id] = node;
    return node;
}

std::shared_ptr<Node> TopologyManager::createNode(const glm::dvec3& pos) {
    return createNode(pos.x, pos.y, pos.z);
}

std::shared_ptr<Edge> TopologyManager::createEdge(uint64_t startNodeId, uint64_t endNodeId) {
    if (nodes.find(startNodeId) == nodes.end() || nodes.find(endNodeId) == nodes.end()) {
        return nullptr; // Nodes must exist
    }
    auto id = generateId();
    auto edge = std::make_shared<Edge>(id, startNodeId, endNodeId);
    edges[id] = edge;
    return edge;
}

std::shared_ptr<Face> TopologyManager::createFace(const std::vector<uint64_t>& edgeIds) {
    for (auto eid : edgeIds) {
        if (edges.find(eid) == edges.end()) return nullptr; // All edges must exist
    }
    auto id = generateId();
    auto face = std::make_shared<Face>(id, edgeIds);
    faces[id] = face;
    return face;
}

std::shared_ptr<Node> TopologyManager::getNode(uint64_t id) const {
    auto it = nodes.find(id);
    return it != nodes.end() ? it->second : nullptr;
}

std::shared_ptr<Edge> TopologyManager::getEdge(uint64_t id) const {
    auto it = edges.find(id);
    return it != edges.end() ? it->second : nullptr;
}

std::shared_ptr<Face> TopologyManager::getFace(uint64_t id) const {
    auto it = faces.find(id);
    return it != faces.end() ? it->second : nullptr;
}

bool TopologyManager::removeNode(uint64_t id) {
    return nodes.erase(id) > 0;
}

bool TopologyManager::removeEdge(uint64_t id) {
    return edges.erase(id) > 0;
}

bool TopologyManager::removeFace(uint64_t id) {
    return faces.erase(id) > 0;
}

void TopologyManager::clear() {
    nodes.clear();
    edges.clear();
    faces.clear();
    nextId = 1;
}

} // namespace Topology
} // namespace Core
