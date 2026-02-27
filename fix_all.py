import re

def fix_file(filepath, fixes):
    try:
        with open(filepath, "r") as f:
            content = f.f.read() if hasattr(f, 'f') else f.read()
            
        for old, new in fixes:
            content = content.replace(old, new)
            
        with open(filepath, "w") as f:
            f.write(content)
        print(f"Fixed {filepath}")
    except Exception as e:
        print(f"Error reading/writing {filepath}: {e}")

# 1. MathUtils
fix_file("src/utils/MathUtils.cpp", [
    ("float d =", "double d ="),
    ("std::acos(glm::clamp<double>(d,", "std::acos(std::clamp<double>(d,")
])

# 2. Canvas.cpp
fix_file("src/Canvas.cpp", [
    ("glm::normalizeVector(", "glm::normalize("),
    ("glm::dotProduct(", "glm::dot("),
    ("ImVec2 mousePos = ImGui::GetMousePos();", "glm::dvec2 mousePos = Drawing::Math::toDVec2(ImGui::GetMousePos());"),
    ("panOffset = panOffset + delta;", "panOffset = panOffset + Drawing::Math::toDVec2(delta);"),
    ("void Canvas::setPanOffset(const ImVec2& offset)", "void Canvas::setPanOffset(const glm::dvec2& offset)"),
    ("ImVec2 panOffset;", "glm::dvec2 panOffset;"),
])

# 2. b) fix canvas.cpp regex specific stuff
with open("src/Canvas.cpp", "r") as f:
    ctxt = f.read()
# fix transformCoordinates argument mismatch in Canvas if any? Wait, error 81 was in Canvas.cpp. Let's see what error 81 was.
# Canvas.cpp:81: `panOffset = panOffset + delta;` delta is ImVec2.
# Canvas.cpp:91: `setPanOffset(panOffset + ImGui::GetMouseDragDelta());`
ctxt = ctxt.replace("setPanOffset(panOffset + ImGui::GetMouseDragDelta());", "setPanOffset(panOffset + Drawing::Math::toDVec2(ImGui::GetMouseDragDelta()));")
# Canvas.cpp:489 is likely getting `ImVec2 viewportMin = inverseTransformCoordinates(ImVec2(0, 0));` or similar
ctxt = ctxt.replace("inverseTransformCoordinates(ImVec2(0, 0))", "inverseTransformCoordinates(glm::dvec2(0, 0))")
ctxt = ctxt.replace("inverseTransformCoordinates(ImVec2(canvasWidth, canvasHeight))", "inverseTransformCoordinates(glm::dvec2(canvasWidth, canvasHeight))")
with open("src/Canvas.cpp", "w") as f: f.write(ctxt)

# Fix header
fix_file("include/Canvas.hpp", [
    ("void setPanOffset(const ImVec2& offset)", "void setPanOffset(const glm::dvec2& offset)"),
    ("ImVec2 getPanOffset() const", "glm::dvec2 getPanOffset() const"),
    ("ImVec2 panOffset", "glm::dvec2 panOffset"),
])

# 3. Viewers
for viewer in ["BallBearingViewer3D.cpp", "BellowsViewer3D.cpp", "ShockAbsorberViewer3D.cpp", "SpringViewer3D.cpp"]:
    fix_file(f"src/{viewer}", [
        ("ImVec2 size = ImGui::GetContentRegionAvail();", "glm::dvec2 size = Drawing::Math::toDVec2(ImGui::GetContentRegionAvail());"),
        ("canvas.setWindowSize(size.x, size.y);", "canvas.setWindowSize(size.x, size.y);"), # size remains dvec2, x/y are double
        ("camera.setAspectRatio(size.x / size.y);", "camera.setAspectRatio((float)(size.x / size.y));")
    ])

# 4. Viewers3DUI.cpp
with open("src/ui/Viewers3DUI.cpp", "r") as f:
    vui = f.read()
vui = vui.replace("ImVec2 size = ImGui::GetContentRegionAvail();", "glm::dvec2 size = Drawing::Math::toDVec2(ImGui::GetContentRegionAvail());")
with open("src/ui/Viewers3DUI.cpp", "w") as f: f.write(vui)

# 5. Base3DViewer.cpp
with open("src/Base3DViewer.cpp", "r") as f:
    b3v = f.read()
b3v = b3v.replace("ImGui::Image((ImTextureID)(intptr_t)textureId, size, uv0, uv1);", "ImGui::Image((ImTextureID)(intptr_t)textureId, Drawing::Math::toImVec2(size), Drawing::Math::toImVec2(uv0), Drawing::Math::toImVec2(uv1));")
with open("src/Base3DViewer.cpp", "w") as f: f.write(b3v)

# 6. Renderer2D.cpp remaining issues
with open("src/Renderer2D.cpp", "r") as f:
    r2d = f.read()
r2d = r2d.replace("std::vector<ImVec2> transformedPoints(polygon->points.size());", "std::vector<ImVec2> transformedPoints(polygon->points.size());") # already fixed by other?
# wait, Renderer2D:359 conversion from vector<dvec2> to vector<ImVec2>
r2d = r2d.replace("std::vector<ImVec2> points = spline->calculatePoints(0.01f);", "std::vector<glm::dvec2> points = spline->calculatePoints(0.01f);")
# Let's dynamically replace polyPoints and transformedPoints assignments
r2d = re.sub(r"std::vector<ImVec2> polyPoints\(points\.size\(\)\);", r"std::vector<glm::dvec2> polyPoints(points.size());", r2d)
r2d = re.sub(r"std::vector<ImVec2> transformedPoints\(polygon->points\.size\(\)\);", r"std::vector<glm::dvec2> transformedPoints(polygon->points.size());", r2d)
r2d = re.sub(r"drawList->AddConvexPolyFilled\(transformedPoints\.data\(\), transformedPoints\.size\(\)", r"{\nstd::vector<ImVec2> tpIm(transformedPoints.size());\nfor(size_t i=0; i<transformedPoints.size(); ++i) tpIm[i] = toImVec2(transformedPoints[i]);\ndrawList->AddConvexPolyFilled(tpIm.data(), tpIm.size()", r2d)
r2d = re.sub(r"drawList->AddPolyline\(transformedPoints\.data\(\), transformedPoints\.size\(\)", r"{\nstd::vector<ImVec2> tpIm(transformedPoints.size());\nfor(size_t i=0; i<transformedPoints.size(); ++i) tpIm[i] = toImVec2(transformedPoints[i]);\ndrawList->AddPolyline(tpIm.data(), tpIm.size()", r2d)
r2d = re.sub(r"drawList->AddPolyline\(polyPoints\.data\(\), polyPoints\.size\(\)", r"{\nstd::vector<ImVec2> tpIm(polyPoints.size());\nfor(size_t i=0; i<polyPoints.size(); ++i) tpIm[i] = toImVec2(polyPoints[i]);\ndrawList->AddPolyline(tpIm.data(), tpIm.size()", r2d)

# For drawDashedLine
r2d = re.sub(r"float dashLen", r"double dashLen", r2d)
r2d = re.sub(r"float gapLen", r"double gapLen", r2d)
r2d = re.sub(r"float distance", r"double distance", r2d)

r2d = re.sub(r"canvas->transformCoordinates\((p[0-9])\)", r"toImVec2(canvas->transformCoordinates(\1))", r2d)

with open("src/Renderer2D.cpp", "w") as f: f.write(r2d)

