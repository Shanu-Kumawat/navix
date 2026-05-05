#pragma once

#include "modeling3d/Viewport3D.hpp"

namespace Core {
class ApplicationContext;
}

namespace UI {

/**
 * @brief Toolbar for 3D modeling mode.
 * Replaces the 2D drawing tools when in 3D mode.
 */
class Toolbar3D {
public:
    static void Render(Modeling3D::Viewport3D& viewport, Core::ApplicationContext& appContext);
};

} // namespace UI
