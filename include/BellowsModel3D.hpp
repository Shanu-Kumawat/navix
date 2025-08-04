#ifndef BELLOWS_MODEL_3D_HPP
#define BELLOWS_MODEL_3D_HPP

#include "Base3DModel.hpp"
#include "shapes/ComplexShapes.hpp"

/**
 * BellowsModel3D - Standardized 3D model for bellows shapes
 * Inherits from Base3DModel to follow unified architecture pattern
 */
class BellowsModel3D : public Base3DModel {
public:
    BellowsModel3D();
    ~BellowsModel3D() = default;
    
    // Base3DModel interface implementation
    void generateMesh() override;
    
    // Bellows-specific mesh generation
    void generateMesh(const Drawing::Bellows* bellows);

private:
    // Current bellows being rendered (for regeneration)
    const Drawing::Bellows* currentBellows;
    
    // Bellows-specific helper methods
    void generateBellowsGeometry(const Drawing::Bellows* bellows);
};

#endif 