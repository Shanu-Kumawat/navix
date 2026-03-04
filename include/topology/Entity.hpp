#pragma once
#include <cstdint>

namespace Core {
namespace Topology {

enum class EntityType {
    Node,
    Edge,
    Face,
    Volume
};

class Entity {
public:
    explicit Entity(EntityType type, uint64_t id) : type(type), id(id) {}
    virtual ~Entity() = default;

    uint64_t getId() const { return id; }
    EntityType getType() const { return type; }

private:
    EntityType type;
    uint64_t id;
};

} // namespace Topology
} // namespace Core
