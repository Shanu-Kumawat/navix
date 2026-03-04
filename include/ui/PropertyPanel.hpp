#pragma once

#include "Canvas.hpp"
#include "ApplicationContext.hpp"

namespace UI {
    struct PropertyPanel {
        static void Render(Drawing::Canvas &canvas, Core::ApplicationContext& appContext);
    };
}
