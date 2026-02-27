#pragma once

#include "Canvas.hpp"
#include "ApplicationContext.hpp"

namespace UI {

class TopRibbon {
public:
    static void Render(Drawing::Canvas& canvas, Core::ApplicationContext& appContext);
};

} // namespace UI
