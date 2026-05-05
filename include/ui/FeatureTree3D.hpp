#pragma once

#include "modeling3d/Viewport3D.hpp"

namespace Core { struct ApplicationContext; }

namespace UI {

/**
 * @brief SolidWorks-style feature tree panel (left side).
 * Shows construction history: bodies, sketches, with selection + visibility toggles.
 */
class FeatureTree3D {
public:
    static void Render(Modeling3D::Viewport3D& viewport, Core::ApplicationContext& appContext);
};

} // namespace UI
