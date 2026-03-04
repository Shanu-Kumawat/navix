#pragma once

#include "Canvas.hpp"
#include "ApplicationContext.hpp"

namespace UI {
    struct Viewers3DUI {
        static void RenderSpring3DViewWindow(Drawing::Canvas &canvas, Core::ApplicationContext& appContext);
        static void RenderShockAbsorber3DViewWindow(Drawing::Canvas &canvas, Core::ApplicationContext& appContext);
        static void RenderBellows3DViewWindow(Drawing::Canvas &canvas, Core::ApplicationContext& appContext);
        static void RenderBallBearing3DViewWindow(Drawing::Canvas &canvas, Core::ApplicationContext& appContext);
    };
}
