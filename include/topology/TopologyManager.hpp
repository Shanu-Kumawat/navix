#pragma once
#include "Node.hpp"
#include "Edge.hpp"
#include "Face.hpp"
#include <unordered_map>
#include <memory>
#include <vector>

namespace Core {
namespace Topology {

class TopologyManager {
public:
    TopologyManager() = default;

    std::shared_ptr<Node> createNode(double x, double y, double z = 0.0);
    std::shared_ptr<Node> createNode(const glm::dvec3& pos);
    
    std::shared_ptr<Edge> createEdge(uint64_t startNodeId, uint64_t endNodeId);
    std::shared_ptr<Face> createFace(const std::vector<uint64_t>& edgeIds);

    std::shared_ptr<Node> getNode(uint64_t id) const;
    std::shared_ptr<Edge> getEdge(uint64_t id) const;
    std::shared_ptr<Face> getFace(uint64_t id) const;

    bool removeNode(uint64_t id);
    bool removeEdge(uint64_t id);
    bool removeFace(uint64_t id);

    void clear();

    const std::unordered_map<uint64_t, std::shared_ptr<Node>>& getNodes() const { return nodes; }
    const std::unordered_map<uint64_t, std::shared_ptr<Edge>>& getEdges() const { return edges; }
    const std::unordered_map<uint64_t, std::shared_ptr<Face>>& getFaces() const { return faces; }

private:
    uint64_t nextId = 1;
    uint64_t generateId() { return nextId++; }

    std::unordered_map<uint64_t, std::shared_ptr<Node>> nodes;
    std::unordered_map<uint64_t, std::shared_ptr<Edge>> edges;
    std::unordered_map<uint64_t, std::shared_ptr<Face>> faces;
};

} // namespace Topology
} // namespace Core
