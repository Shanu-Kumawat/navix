#pragma once
#include "Entity.hpp"
#include <vector>

namespace Core {
namespace Topology {

class Edge : public Entity {
public:
    Edge(uint64_t id, uint64_t startNodeId, uint64_t endNodeId) 
        : Entity(EntityType::Edge, id), startNodeId(startNodeId), endNodeId(endNodeId) {}

    uint64_t getStartNodeId() const { return startNodeId; }
    uint64_t getEndNodeId() const { return endNodeId; }

private:
    uint64_t startNodeId;
    uint64_t endNodeId;
};

} // namespace Topology
} // namespace Core
