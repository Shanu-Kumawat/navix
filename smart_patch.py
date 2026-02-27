import re

def patch_file(fname, operations):
    with open(fname, "r") as f: text = f.read()
    if "using namespace Drawing::Math;" not in text and "Renderer2D" not in fname and "Viewers3DUI" not in fname:
        text = text.replace("namespace Core {\n", "namespace Core {\nusing namespace Drawing::Math;\n")
        text = text.replace("namespace Drawing {\n", "namespace Drawing {\nusing namespace Drawing::Math;\n")

    for old, new in operations:
        text = text.replace(old, new)
    with open(fname, "w") as f: f.write(text)

# 1. MathUtils.cpp
patch_file("src/utils/MathUtils.cpp", [
    ("float projection", "double projection"),
    ("float t =", "double t ="),
    ("float dx", "double dx"),
    ("float dy", "double dy"),
    ("float lineLenSq", "double lineLenSq"),
    ("lerp(", "glm::mix("),
    ("float d =", "double d ="),
    ("std::acos(clamp(d, -1.0f, 1.0f))", "std::acos(glm::clamp(d, -1.0, 1.0))")
])
patch_file("include/utils/MathUtils.hpp", [
    ("float calculateDistanceToLine", "double calculateDistanceToLine")
])

# 2. Canvas.cpp
patch_file("src/Canvas.cpp", [
    ("glm::normalizeVector", "glm::normalize"),
    ("glm::dotProduct", "glm::dot"),
    ("ImVec2 mousePos = ImGui::GetMousePos();", "glm::dvec2 mousePos = Drawing::Math::toDVec2(ImGui::GetMousePos());"),
    ("panOffset = panOffset + delta;", "panOffset = panOffset + Drawing::Math::toDVec2(delta);"),
    ("setPanOffset(panOffset + ImGui::GetMouseDragDelta());", "setPanOffset(panOffset + Drawing::Math::toDVec2(ImGui::GetMouseDragDelta()));"),
    ("inverseTransformCoordinates(ImVec2(0, 0))", "inverseTransformCoordinates(glm::dvec2(0, 0))"),
    ("inverseTransformCoordinates(ImVec2(canvasWidth, canvasHeight))", "inverseTransformCoordinates(glm::dvec2(canvasWidth, canvasHeight))"),
])

# 3. Canvas.hpp
patch_file("include/Canvas.hpp", [
    ("void setPanOffset(const ImVec2& offset)", "void setPanOffset(const glm::dvec2& offset)"),
    ("ImVec2 getPanOffset() const", "glm::dvec2 getPanOffset() const"),
    ("ImVec2 panOffset;", "glm::dvec2 panOffset;")
])

# 4. Viewers 
for v in ["Base3DViewer.cpp", "SpringViewer3D.cpp", "BallBearingViewer3D.cpp", "BellowsViewer3D.cpp", "ShockAbsorberViewer3D.cpp"]:
    patch_file(f"src/{v}", [
        ("ImVec2 size = ImGui::GetContentRegionAvail();", "glm::dvec2 dSize = Drawing::Math::toDVec2(ImGui::GetContentRegionAvail());"),
        ("canvas.setWindowSize(size.x, size.y);", "canvas.setWindowSize(dSize.x, dSize.y);"),
        ("camera.setAspectRatio(size.x / size.y);", "camera.setAspectRatio((float)(dSize.x / dSize.y));"),
        ("ImGui::Image((ImTextureID)(intptr_t)textureId, size, uv0, uv1);", "ImGui::Image((ImTextureID)(intptr_t)textureId, Drawing::Math::toImVec2(dSize), uv0, uv1);")
    ])

# 5. UI Viewers
patch_file("src/ui/Viewers3DUI.cpp", [
    ("ImVec2 availableSize = ImGui::GetContentRegionAvail();", "glm::dvec2 availableSize = Drawing::Math::toDVec2(ImGui::GetContentRegionAvail());"),
    ("ImVec2 viewportSize = ImVec2(availableSize.x, availableSize.y - 20);", "glm::dvec2 viewportSize = glm::dvec2(availableSize.x, availableSize.y - 20);")
])

# 6. ComplexShape3DManager.cpp
patch_file("src/ComplexShape3DManager.cpp", [
    ("Math::distance(", "glm::distance("),
    ("void ComplexShape3DManager::handleCanvasClick(const ImVec2& clickPos)", "void ComplexShape3DManager::handleCanvasClick(const glm::dvec2& clickPos)"),
    ("void ComplexShape3DManager::handleCanvasDrag(const ImVec2& dragDelta)", "void ComplexShape3DManager::handleCanvasDrag(const glm::dvec2& dragDelta)"),
    ("ImVec2 point = shape->points[j];", "glm::dvec2 point = shape->points[j];")
])
patch_file("include/ComplexShape3DManager.hpp", [
    ("void handleCanvasClick(const ImVec2& clickPos);", "void handleCanvasClick(const glm::dvec2& clickPos);"),
    ("void handleCanvasDrag(const ImVec2& dragDelta);", "void handleCanvasDrag(const glm::dvec2& dragDelta);")
])

# 7. Dimension, ComplexShapes, BottomEnd
patch_file("src/shapes/Dimension.cpp", [
    ("ImVec2 textSize = ImGui::CalcTextSize(text.c_str());", "glm::dvec2 textSize = Drawing::Math::toDVec2(ImGui::CalcTextSize(text.c_str()));"),
    ("ImVec2 textPos = center - ImVec2(textSize.x / 2.0f, textSize.y / 2.0f);", "glm::dvec2 textPos = center - glm::dvec2(textSize.x / 2.0, textSize.y / 2.0);"),
    ("drawList->AddLine(arrowP1, end, color, thickness);", "drawList->AddLine(Drawing::Math::toImVec2(arrowP1), Drawing::Math::toImVec2(end), color, thickness);"),
    ("drawList->AddLine(arrowP2, end, color, thickness);", "drawList->AddLine(Drawing::Math::toImVec2(arrowP2), Drawing::Math::toImVec2(end), color, thickness);"),
    ("drawList->AddLine(p1, p2, color, thickness);", "drawList->AddLine(Drawing::Math::toImVec2(p1), Drawing::Math::toImVec2(p2), color, thickness);"),
    ("drawList->AddText(textPos", "drawList->AddText(Drawing::Math::toImVec2(textPos)")
])

patch_file("src/shapes/ComplexShapes.cpp", [
    ("std::clamp(", "std::clamp<double>("),
    ("0.0f, 1.0f", "0.0, 1.0"),
    ("controlPoints[0] * 2.0f", "controlPoints[0] * 2.0"),
    ("controlPoints[n-1] * 2.0f", "controlPoints[n-1] * 2.0"),
    ("drawList->AddCircleFilled(pos, size, color);", "drawList->AddCircleFilled(Drawing::Math::toImVec2(pos), size, color);"),
    ("drawList->AddCircle(pos, size + 1.0f", "drawList->AddCircle(Drawing::Math::toImVec2(pos), size + 1.0f"),
    ("drawList->AddLine(points[i - 1], points[i], color, 1.0f);", "drawList->AddLine(Drawing::Math::toImVec2(points[i - 1]), Drawing::Math::toImVec2(points[i]), color, 1.0f);"),
    ("drawList->AddLine(point, tangent, color, 1.0f);", "drawList->AddLine(Drawing::Math::toImVec2(point), Drawing::Math::toImVec2(tangent), color, 1.0f);"),
    ("drawList->AddCircleFilled(tangent, 3.0f, color);", "drawList->AddCircleFilled(Drawing::Math::toImVec2(tangent), 3.0f, color);")
])

patch_file("src/shapes/ShockAbsorberBottomEnd.cpp", [
    ("drawList->AddConvexPolyFilled(points, 4, color);", "{\n        std::vector<ImVec2> tpIm(4);\n        for(int _i=0; _i<4; ++_i) tpIm[_i] = Drawing::Math::toImVec2((points)[_i]);\n        drawList->AddConvexPolyFilled(tpIm.data(), 4, color);\n    }"),
    ("drawList->AddCircleFilled(points[i], 3.0f, IM_COL32(255, 0, 0, 255));", "drawList->AddCircleFilled(Drawing::Math::toImVec2(points[i]), 3.0f, IM_COL32(255, 0, 0, 255));"),
    ("drawList->AddPolyline(points, 4, IM_COL32(255, 165, 0, 255), ImDrawFlags_Closed, 2.0f);", "{\n        std::vector<ImVec2> tpIm(4);\n        for(int _i=0; _i<4; ++_i) tpIm[_i] = Drawing::Math::toImVec2((points)[_i]);\n        drawList->AddPolyline(tpIm.data(), 4, IM_COL32(255, 165, 0, 255), ImDrawFlags_Closed, 2.0f);\n    }")
])

print("Finished smart patch step 1")
