#pragma once

#include "Canvas.hpp"
#include "ApplicationContext.hpp"

namespace UI {
    struct StatusBar {
        static void Render(Drawing::Canvas &canvas, Core::ApplicationContext& appContext);
    };
}
