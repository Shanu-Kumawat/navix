#ifndef SHOCK_ABSORBER_VIEWER_3D_HPP
#define SHOCK_ABSORBER_VIEWER_3D_HPP

#include "Base3DViewer.hpp"
#include "ShockAbsorberModel3D.hpp"
#include "shapes/BasicShapes.hpp"
#include "shapes/ShockAbsorberBottomEnd.hpp"

/**
 * ShockAbsorberViewer3D - Standardized 3D viewer for shock absorber shapes
 * Inherits from Base3DViewer to follow unified architecture pattern
 */
class ShockAbsorberViewer3D : public Base3DViewer {
public:
    ShockAbsorberViewer3D();
    ~ShockAbsorberViewer3D() = default;
    
    // Base3DViewer interface implementation
    void initialize() override;
    void handleInput(const SDL_Event& event) override;
    
    // Shock absorber-specific rendering (takes multiple components)
    void render(const Drawing::Spring2D* spring, const Drawing::ShockAbsorberEnd2D* end, 
                const Drawing::ShockAbsorberBottomEnd* bottomEnd, ImVec2 windowSize);

private:
    // Shock absorber-specific 3D model
    std::unique_ptr<ShockAbsorberModel3D> shockAbsorberModel;
};

#endif 