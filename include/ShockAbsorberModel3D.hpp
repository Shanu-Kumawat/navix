#include <glm/glm.hpp>
#ifndef SHOCK_ABSORBER_MODEL_3D_HPP
#define SHOCK_ABSORBER_MODEL_3D_HPP

#include "Base3DModel.hpp"
#include "shapes/BasicShapes.hpp"
#include "shapes/ShockAbsorberBottomEnd.hpp"

/**
 * ShockAbsorberModel3D - Standardized 3D model for shock absorber shapes
 * Inherits from Base3DModel to follow unified architecture pattern
 */
class ShockAbsorberModel3D : public Base3DModel {
public:
    ShockAbsorberModel3D();
    ~ShockAbsorberModel3D() = default;
    
    // Base3DModel interface implementation
    void generateMesh() override;
    bool generateFEMMesh(float elementSize) override;
    
    // Shock absorber-specific mesh generation (takes multiple components)
    void generateMesh(const Drawing::Spring2D* spring, const Drawing::ShockAbsorberEnd2D* end,
                      const Drawing::ShockAbsorberBottomEnd* bottomEnd);

    // Current component access (for geometry export)
    const Drawing::Spring2D* getCurrentSpring() const { return currentSpring; }
    const Drawing::ShockAbsorberEnd2D* getCurrentEnd() const { return currentEnd; }
    const Drawing::ShockAbsorberBottomEnd* getCurrentBottomEnd() const { return currentBottomEnd; }

private:
    // Current shock absorber components (for regeneration)
    const Drawing::Spring2D* currentSpring;
    const Drawing::ShockAbsorberEnd2D* currentEnd;
    const Drawing::ShockAbsorberBottomEnd* currentBottomEnd;
    
    // Shock absorber-specific helper methods
    void generateShockAbsorberGeometry();
    std::vector<glm::dvec2> generateEnhancedSpringProfile() const;
};

#endif 