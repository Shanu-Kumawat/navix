#pragma once
#include "Entity.hpp"
#include <glm/glm.hpp>

namespace Core {
namespace Topology {

class Node : public Entity {
public:
    Node(uint64_t id, const glm::dvec3& pos) 
        : Entity(EntityType::Node, id), position(pos) {}
    
    Node(uint64_t id, double x, double y, double z = 0.0)
        : Entity(EntityType::Node, id), position(x, y, z) {}

    const glm::dvec3& getPosition() const { return position; }
    void setPosition(const glm::dvec3& pos) { position = pos; }

private:
    glm::dvec3 position;
};

} // namespace Topology
} // namespace Core
