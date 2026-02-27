import os
for f in ["src/Base3DViewer.cpp", "src/SpringViewer3D.cpp", "src/BallBearingViewer3D.cpp", "src/BellowsViewer3D.cpp", "src/ShockAbsorberViewer3D.cpp"]:
    with open(f, "r") as file:
        text = file.read()
    # Add namespace inclusion
    if "using namespace Drawing::Math;" not in text:
        text = text.replace("namespace Core {\n", "namespace Core {\nusing namespace Drawing::Math;\n")

    text = text.replace("ImGui::Image((ImTextureID)(intptr_t)textureId, size, uv0, uv1);", "ImGui::Image((ImTextureID)(intptr_t)textureId, toImVec2(size), toImVec2(uv0), toImVec2(uv1));")
    
    # Check if there's any viewport size variables still passed incorrectly
    text = text.replace("glm::dvec2 size = Drawing::Math::toDVec2(ImGui::GetContentRegionAvail());", "ImVec2 size = ImGui::GetContentRegionAvail();\n        glm::dvec2 dSize = toDVec2(size);")
    text = text.replace("canvas.setWindowSize(size.x, size.y);", "canvas.setWindowSize(dSize.x, dSize.y);")
    text = text.replace("camera.setAspectRatio(size.x / size.y);", "camera.setAspectRatio(dSize.x / dSize.y);")
    text = text.replace("camera.setAspectRatio((float)(size.x / size.y));", "camera.setAspectRatio((float)(dSize.x / dSize.y));")
    text = text.replace("ImGui::Image((ImTextureID)(intptr_t)textureId, toImVec2(size), toImVec2(uv0), toImVec2(uv1));", "ImGui::Image((ImTextureID)(intptr_t)textureId, toImVec2(dSize), toImVec2(uv0), toImVec2(uv1));")

    with open(f, "w") as file:
        file.write(text)

