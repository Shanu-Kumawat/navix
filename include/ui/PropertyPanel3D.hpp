#pragma once

#include "modeling3d/Viewport3D.hpp"

namespace Core {
class ApplicationContext;
}

namespace UI {

/**
 * @brief Property panel for 3D modeling mode.
 * Shows context-sensitive properties for the selected body, active sketch, or operation.
 */
class PropertyPanel3D {
public:
    static void Render(Modeling3D::Viewport3D& viewport, Core::ApplicationContext& appContext);
};

} // namespace UI
