#include <glm/glm.hpp>
#ifndef BALL_BEARING_VIEWER_3D_HPP
#define BALL_BEARING_VIEWER_3D_HPP

#include "Base3DViewer.hpp"
#include "BallBearingModel3D.hpp"
#include "shapes/ComplexShapes.hpp"

/**
 * BallBearingViewer3D - Standardized 3D viewer for ball bearing shapes
 * Inherits from Base3DViewer to follow unified architecture pattern
 */
class BallBearingViewer3D : public Base3DViewer {
public:
    BallBearingViewer3D();
    ~BallBearingViewer3D() = default;
    
    // Base3DViewer interface implementation
    void initialize() override;
    void handleInput(const SDL_Event& event) override;
    
    // Ball bearing-specific rendering
    void render(const Drawing::BallBearing* ballBearing, glm::dvec2 windowSize);

private:
    // Ball bearing-specific 3D model
    std::unique_ptr<BallBearingModel3D> ballBearingModel;
};

#endif
