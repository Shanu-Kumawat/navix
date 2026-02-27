#include <iostream>
#include <fstream>
#include <string>
#include <vector>

int main() {
    // Read Canvas.hpp
    std::ifstream in("include/Canvas.hpp");
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    // Remove DrawingMode enum
    size_t start = content.find("enum class DrawingMode");
    size_t end = content.find("};", start) + 2;
    content.erase(start, end - start);

    // Add includes
    size_t includePos = content.find("#include \"Constants.hpp\"");
    content.insert(includePos + 24, "\n#include \"Renderer2D.hpp\"\n#include \"InputController.hpp\"\n#include \"SceneModel.hpp\"");

    // Add getters
    size_t publicPos = content.find("public:");
    std::string getters = R"(
    // Getters for Renderer2D
    float getZoomLevel() const { return zoomLevel; }
    ImVec2 getPanOffset() const { return panOffset; }
    bool isGridVisible() const { return showGrid; }
    float getGridSpacing() const { return gridSpacing; }
    float getWindowWidth() const { return windowWidth; }
    float getWindowHeight() const { return windowHeight; }
    float getWindowX() const { return windowX; }
    float getWindowY() const { return windowY; }
    bool getIsDrawing() const { return isDrawing; }
    ImVec2 getStartPoint() const { return startPoint; }
    int getClickCount() const { return clickCount; }
    const std::vector<ImVec2>& getTrianglePoints() const { return trianglePoints; }
    const std::vector<ImVec2>& getCurrentSplinePoints() const { return currentSplinePoints; }
    const std::vector<ImVec2>& getCurrentCurvePoints() const { return currentCurvePoints; }
    bool getShowControlPoints() const { return showControlPoints; }
    Shape* getSelectedShape() const { return selectedShape; }
    
    // Make these public for now
    float springOuterDiameter = 44.5f;
    float springWireDiameter = 7.25f;
    float springFreeLength = 68.0f;
    int springNumCoils = 6;
)";
    content.insert(publicPos + 7, getters);

    // Remove private spring properties
    size_t springPos = content.find("float springOuterDiameter = 44.5f;");
    if (springPos != std::string::npos) {
        size_t springEnd = content.find("int springNumCoils = 6;", springPos) + 23;
        content.erase(springPos, springEnd - springPos);
    }

    // Make calculateSplinePoints public
    size_t calcPos = content.find("std::vector<ImVec2> calculateSplinePoints");
    if (calcPos != std::string::npos) {
        size_t calcEnd = content.find(";", calcPos) + 1;
        std::string calcStr = content.substr(calcPos, calcEnd - calcPos);
        content.erase(calcPos, calcEnd - calcPos);
        content.insert(publicPos + 7, "\n    " + calcStr + "\n");
    }

    // Add unique_ptrs
    size_t privatePos = content.find("private:");
    std::string ptrs = R"(
    std::unique_ptr<Core::Renderer2D> renderer;
    std::unique_ptr<Core::InputController> inputController;
    std::unique_ptr<Core::SceneModel> sceneModel;
)";
    content.insert(privatePos + 8, ptrs);

    std::ofstream out("include/Canvas.hpp");
    out << content;
    out.close();

    return 0;
}
