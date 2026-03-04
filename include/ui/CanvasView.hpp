#pragma once

#include "Canvas.hpp"
#include "ApplicationContext.hpp"

namespace UI {
    struct CanvasView {
        static void Render(Drawing::Canvas &canvas, Core::ApplicationContext& appContext);
        static void HandleKeyboardShortcuts(Drawing::Canvas &canvas, Core::ApplicationContext& appContext);
    };
}
