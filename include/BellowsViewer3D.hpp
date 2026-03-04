#include <glm/glm.hpp>
#ifndef BELLOWS_VIEWER_3D_HPP
#define BELLOWS_VIEWER_3D_HPP

#include "Base3DViewer.hpp"
#include "BellowsModel3D.hpp"
#include "shapes/ComplexShapes.hpp"

/**
 * BellowsViewer3D - Standardized 3D viewer for bellows shapes
 * Inherits from Base3DViewer to follow unified architecture pattern
 */
class BellowsViewer3D : public Base3DViewer {
public:
    BellowsViewer3D();
    ~BellowsViewer3D() = default;
    
    // Base3DViewer interface implementation
    void initialize() override;
    void handleInput(const SDL_Event& event) override;
    
    // Bellows-specific rendering
    void render(const Drawing::Bellows* bellows, glm::dvec2 windowSize);
    
    // Access to 3D model for mesh controls
    BellowsModel3D* getModel() const { return bellowsModel.get(); }

private:
    // Bellows-specific 3D model
    std::unique_ptr<BellowsModel3D> bellowsModel;
    
    // Setup methods - no longer needed as they're in base class
    // All framebuffer and common setup handled by Base3DViewer
};

#endif
