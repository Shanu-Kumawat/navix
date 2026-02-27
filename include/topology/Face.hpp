#pragma once
#include "Entity.hpp"
#include <vector>

namespace Core {
namespace Topology {

class Face : public Entity {
public:
    Face(uint64_t id, const std::vector<uint64_t>& edgeIds) 
        : Entity(EntityType::Face, id), edgeIds(edgeIds) {}

    const std::vector<uint64_t>& getEdgeIds() const { return edgeIds; }

private:
    std::vector<uint64_t> edgeIds;
};

} // namespace Topology
} // namespace Core
