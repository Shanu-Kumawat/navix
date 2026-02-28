#include "fem/MaterialManager.hpp"
#include <algorithm>

namespace Core {
namespace FEM {

MaterialManager::MaterialManager() : nextId(1) {
    populateStandardMaterials();
}

uint32_t MaterialManager::addMaterial(const Material& material) {
    uint32_t id = nextId++;
    Material newMaterial = material;
    newMaterial.id = id;
    materials[id] = newMaterial;
    return id;
}

bool MaterialManager::updateMaterial(uint32_t id, const Material& material) {
    if (materials.find(id) != materials.end()) {
        Material updatedMaterial = material;
        updatedMaterial.id = id; // Ensure ID remains consistent
        materials[id] = updatedMaterial;
        return true;
    }
    return false;
}

const Material* MaterialManager::getMaterial(uint32_t id) const {
    auto it = materials.find(id);
    if (it != materials.end()) {
        return &(it->second);
    }
    return nullptr;
}

std::vector<Material> MaterialManager::getAllMaterials() const {
    std::vector<Material> result;
    result.reserve(materials.size());
    for (const auto& pair : materials) {
        result.push_back(pair.second);
    }
    // Sort by ID to maintain consistent order
    std::sort(result.begin(), result.end(), [](const Material& a, const Material& b) {
        return a.id < b.id;
    });
    return result;
}

bool MaterialManager::removeMaterial(uint32_t id) {
    return materials.erase(id) > 0;
}

void MaterialManager::populateStandardMaterials() {
    // 1. Structural Steel (A36)
    addMaterial(Material(0, "Structural Steel", 200e9, 0.3, 7850.0, 250e6));
    // 2. Aluminum Alloy (6061)
    addMaterial(Material(0, "Aluminum 6061", 69e9, 0.33, 2700.0, 276e6));
    // 3. Concrete (High Strength)
    addMaterial(Material(0, "Concrete", 30e9, 0.2, 2400.0, 40e6)); // Concrete yield is compressive strength
    // 4. Titanium (Ti-6Al-4V)
    addMaterial(Material(0, "Titanium", 114e9, 0.34, 4430.0, 880e6));
}

} // namespace FEM
} // namespace Core
