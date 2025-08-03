void Canvas::renderGrid(ImDrawList* drawList) {
    // Get the canvas window position and size
    float startX = windowX;
    float startY = windowY;
    float endX = windowX + windowWidth;
    float endY = windowY + windowHeight;

    // Only render grid if it's enabled
    if (gridSpacing <= 0) return;

    // Calculate adjusted grid spacing based on zoom level
    float effectiveSpacing = gridSpacing * zoomLevel;
    
    // Ensure grid isn't too dense or sparse based on zoom level
    if (effectiveSpacing < 10.0f) {
        effectiveSpacing *= std::ceil(10.0f / effectiveSpacing);
    } else if (effectiveSpacing > 200.0f) {
        effectiveSpacing /= std::floor(effectiveSpacing / 100.0f);
    }

    // Calculate grid offset based on pan
    float offsetX = std::fmod(panOffset.x * zoomLevel, effectiveSpacing);
    float offsetY = std::fmod(panOffset.y * zoomLevel, effectiveSpacing);
    
    // Calculate number of lines needed
    int numLinesX = static_cast<int>(windowWidth / effectiveSpacing) + 2;
    int numLinesY = static_cast<int>(windowHeight / effectiveSpacing) + 2;

    // Draw vertical grid lines
    for (int i = 0; i <= numLinesX; i++) {
        float x = startX + i * effectiveSpacing + offsetX;
        if (x < startX) continue;
        if (x > endX) break;
        
        bool isMajor = (i % 5 == 0);
        
        // Use appropriate grid colors based on theme
        ImU32 lineColor = isMajor ? Colors::DARK_GRID_MAJOR : Colors::DARK_GRID_MINOR;
            
        float lineThickness = isMajor ? 1.0f : 0.5f;
        
        drawList->AddLine(
            ImVec2(x, startY),
            ImVec2(x, endY),
            lineColor,
            lineThickness
        );
    }

    // Draw horizontal grid lines
    for (int i = 0; i <= numLinesY; i++) {
        float y = startY + i * effectiveSpacing + offsetY;
        if (y < startY) continue;
        if (y > endY) break;
        
        bool isMajor = (i % 5 == 0);
        
        // Use appropriate grid colors based on theme
        ImU32 lineColor = isMajor ? Colors::DARK_GRID_MAJOR : Colors::DARK_GRID_MINOR;
            
        float lineThickness = isMajor ? 1.0f : 0.5f;
        
        drawList->AddLine(
            ImVec2(startX, y),
            ImVec2(endX, y),
            lineColor,
            lineThickness
        );
    }
    
    // Add axes lines at (0,0) if visible
    ImVec2 origin = transformCoordinates(ImVec2(0, 0));
    ImU32 axisColor = IM_COL32(105, 65, 35, 70);
    
    if (origin.x >= startX && origin.x <= endX) {
        drawList->AddLine(
            ImVec2(origin.x, startY),
            ImVec2(origin.x, endY),
            axisColor,
            1.5f
        );
    }
    
    if (origin.y >= startY && origin.y <= endY) {
        drawList->AddLine(
            ImVec2(startX, origin.y),
            ImVec2(endX, origin.y),
            axisColor,
            1.5f
        );
    }
} 