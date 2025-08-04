#ifndef BALL_BEARING_MODEL_3D_HPP
#define BALL_BEARING_MODEL_3D_HPP

#include "Base3DModel.hpp"
#include "shapes/ComplexShapes.hpp"

/**
 * BallBearingModel3D - Standardized 3D model for ball bearing shapes
 * Inherits from Base3DModel to follow unified architecture pattern
 */
class BallBearingModel3D : public Base3DModel {
public:
    BallBearingModel3D();
    ~BallBearingModel3D() = default;
    
    // Base3DModel interface implementation
    void generateMesh() override;
    
    // Ball bearing-specific mesh generation
    void generateMesh(const Drawing::BallBearing* ballBearing);

private:
    // Current ball bearing being rendered (for regeneration)
    const Drawing::BallBearing* currentBallBearing;
    
    // Ball bearing-specific helper methods
    void generateRaceGeometry(const Drawing::BallBearing* ballBearing);
    void generateBallGeometry(const std::vector<ImVec2>& ballPositions, float ballRadius, float scale);
    void generateSeparateRaces(float innerRadius, float outerRadius, float width, int segments);
};

#endif
