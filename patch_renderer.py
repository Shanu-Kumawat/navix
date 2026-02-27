import re

with open("src/Renderer2D.cpp", "r") as f:
    text = f.read()

# Replace method signatures to match header
text = text.replace("renderPreview(ImDrawList* drawList, const ImVec2& currentPos", "renderPreview(ImDrawList* drawList, const glm::dvec2& currentPos")
text = text.replace("previewPoint(ImDrawList* drawList, const ImVec2& pos)", "previewPoint(ImDrawList* drawList, const glm::dvec2& pos)")
text = text.replace("previewLine(ImDrawList* drawList, const ImVec2& start, const ImVec2& end)", "previewLine(ImDrawList* drawList, const glm::dvec2& start, const glm::dvec2& end)")
text = text.replace("previewCircle(ImDrawList* drawList, const ImVec2& center", "previewCircle(ImDrawList* drawList, const glm::dvec2& center")
text = text.replace("previewSquare(ImDrawList* drawList, const ImVec2& start, const ImVec2& end)", "previewSquare(ImDrawList* drawList, const glm::dvec2& start, const glm::dvec2& end)")
text = text.replace("previewRectangle(ImDrawList* drawList, const ImVec2& start, const ImVec2& end)", "previewRectangle(ImDrawList* drawList, const glm::dvec2& start, const glm::dvec2& end)")
text = text.replace("previewBellows(ImDrawList* drawList, const ImVec2& start, const ImVec2& end)", "previewBellows(ImDrawList* drawList, const glm::dvec2& start, const glm::dvec2& end)")
text = text.replace("previewBallBearing(ImDrawList* drawList, const ImVec2& center", "previewBallBearing(ImDrawList* drawList, const glm::dvec2& center")
text = text.replace("previewSpring2D(ImDrawList* drawList, const ImVec2& center)", "previewSpring2D(ImDrawList* drawList, const glm::dvec2& center)")

text = text.replace("const std::vector<ImVec2>& points", "const std::vector<glm::dvec2>& points")
text = text.replace("const std::array<ImVec2, 3>& points", "const std::array<glm::dvec2, 3>& points")

text = text.replace("drawDashedLine(ImDrawList* drawList, const ImVec2& p1, const ImVec2& p2", "drawDashedLine(ImDrawList* drawList, const glm::dvec2& p1, const glm::dvec2& p2")
text = text.replace("renderSnapIndicator(ImDrawList* drawList, const ImVec2& pos", "renderSnapIndicator(ImDrawList* drawList, const glm::dvec2& pos")

with open("src/Renderer2D.cpp", "w") as f:
    f.write(text)
