#include <glm/glm.hpp>
#ifndef COMPLEX_SHAPE_3D_MANAGER_HPP
#define COMPLEX_SHAPE_3D_MANAGER_HPP

#include <memory>
#include <unordered_map>
#include <string>
#include <functional>
#include "shapes/ComplexShapes.hpp"
#include "shapes/BasicShapes.hpp"
#include "shapes/ShockAbsorberBottomEnd.hpp"
#include "BellowsViewer3D.hpp"
#include "BallBearingViewer3D.hpp"
#include "ShockAbsorberViewer3D.hpp"

/**
 * ComplexShape3DManager - Unified manager for all complex shape 3D viewers
 * This class standardizes the 3D view architecture across all complex shapes
 * and eliminates inconsistencies in initialization, button handling, and state management
 */
class ComplexShape3DManager {
public:
    enum class Shape3DType {
        BELLOWS,
        BALL_BEARING,
        SHOCK_ABSORBER
    };

    ComplexShape3DManager() = default;
    ~ComplexShape3DManager() = default;

    // Unified 3D view management
    void renderBellows3DView(const Drawing::Bellows* bellows, bool& showFlag);
    void renderBallBearing3DView(const Drawing::BallBearing* ballBearing, bool& showFlag);
    void renderShockAbsorber3DView(const Drawing::Spring2D* spring, 
                                   const Drawing::ShockAbsorberEnd2D* end,
                                   const Drawing::ShockAbsorberBottomEnd* bottomEnd, 
                                   bool& showFlag);

    // Unified input handling
    void handleInput(const SDL_Event& event);

    // Unified state management
    void resetViewerState(Shape3DType type);
    void resetAllViewerStates();

    // Button rendering helper (standardized UI)
    static bool render3DViewButton(const std::string& label, const std::string& tooltip);

private:
    // Viewer instances - created once and reused
    std::unique_ptr<BellowsViewer3D> bellowsViewer;
    std::unique_ptr<BallBearingViewer3D> ballBearingViewer;
    std::unique_ptr<ShockAbsorberViewer3D> shockAbsorberViewer;

    // Initialization state tracking
    std::unordered_map<Shape3DType, bool> viewerInitialized;

    // Helper methods
    void ensureBellowsViewerInitialized();
    void ensureBallBearingViewerInitialized();
    void ensureShockAbsorberViewerInitialized();

    // Common window rendering logic
    void renderViewerWindow(const std::string& title, bool& showFlag, 
                          std::function<void(glm::dvec2)> renderCallback);
};

#endif
