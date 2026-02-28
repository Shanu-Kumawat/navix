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

    uint32_t getMaterialId() const { return materialId; }
    void setMaterialId(uint32_t id) { materialId = id; }

private:
    std::vector<uint64_t> edgeIds;
    uint32_t materialId = 0; // 0 = no material, or default material
};

} // namespace Topology
} // namespace Core
