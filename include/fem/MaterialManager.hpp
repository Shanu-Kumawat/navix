#pragma once
#include "Material.hpp"
#include <vector>
#include <unordered_map>

namespace Core {
namespace FEM {

class MaterialManager {
public:
    MaterialManager();
    
    // Add a new material (returns the generated ID)
    uint32_t addMaterial(const Material& material);
    
    // Update an existing material
    bool updateMaterial(uint32_t id, const Material& material);
    
    // Retrieve a material by ID
    const Material* getMaterial(uint32_t id) const;
    
    // Retrieve all materials
    std::vector<Material> getAllMaterials() const;
    
    // Remove a material by ID
    bool removeMaterial(uint32_t id);
    
private:
    void populateStandardMaterials();

    std::unordered_map<uint32_t, Material> materials;
    uint32_t nextId;
};

} // namespace FEM
} // namespace Core
