import re

with open("src/Renderer2D.cpp", "r") as f:
    text = f.read()

# Make sure VectorMath is included
if "#include \"utils/VectorMath.hpp\"" not in text:
    text = text.replace("#include \"utils/MathUtils.hpp\"", "#include \"utils/MathUtils.hpp\"\n#include \"utils/VectorMath.hpp\"")

# Add using namespace Drawing::Math; for easy toImVec2, toDVec2 etc
if "using namespace Drawing::Math;" not in text:
    text = text.replace("namespace Core {\n", "namespace Core {\nusing namespace Drawing::Math;\n")


# Fix panOffset
text = text.replace("ImVec2 panOffset = canvas->getPanOffset();", "glm::dvec2 panOffsetVec = canvas->getPanOffset();\n    ImVec2 panOffset = toImVec2(panOffsetVec);")
text = text.replace("ImVec2 origin = canvas->transformCoordinates(ImVec2(0, 0));", "ImVec2 origin = toImVec2(canvas->transformCoordinates(glm::dvec2(0, 0)));")
text = text.replace("ImVec2 viewportMin = canvas->inverseTransformCoordinates(ImVec2(0, 0));", "glm::dvec2 viewportMin = canvas->inverseTransformCoordinates(glm::dvec2(0, 0));")
text = text.replace("ImVec2 viewportMax = canvas->inverseTransformCoordinates(ImVec2(canvas->getWindowWidth(), canvas->getWindowHeight()));", "glm::dvec2 viewportMax = canvas->inverseTransformCoordinates(glm::dvec2(canvas->getWindowWidth(), canvas->getWindowHeight()));")
text = text.replace("ImVec2 shapeMin, shapeMax;", "glm::dvec2 shapeMin, shapeMax;")

# Fix transformed coordinates assignments: ImVec2 var = canvas->transformCoordinates(val); => ImVec2 var = toImVec2(canvas->transformCoordinates(val));
text = re.sub(r"ImVec2\s+(\w+)\s*=\s*canvas->transformCoordinates\(([^)]+)\);", r"ImVec2 \1 = toImVec2(canvas->transformCoordinates(\2));", text)

# For transformedPoints[i]
text = re.sub(r"transformedPoints\[i\]\s*=\s*canvas->transformCoordinates\(([^)]+)\);", r"transformedPoints[i] = toImVec2(canvas->transformCoordinates(\1));", text)
text = text.replace("std::vector<glm::dvec2> transformedPoints(polygon->points.size());", "std::vector<ImVec2> transformedPoints(polygon->points.size());")

text = text.replace("std::vector<glm::dvec2> polyPoints(points.size());", "std::vector<ImVec2> polyPoints(points.size());")
text = re.sub(r"polyPoints\[i\]\s*=\s*canvas->transformCoordinates\(([^)]+)\);", r"polyPoints[i] = toImVec2(canvas->transformCoordinates(\1));", text)

text = text.replace("ImVec2 topLeft = square->getTopLeft();", "glm::dvec2 topLeft = square->getTopLeft();")
text = text.replace("ImVec2 p1 = canvas->transformCoordinates(topLeft);", "ImVec2 p1 = toImVec2(canvas->transformCoordinates(topLeft));")
text = text.replace("ImVec2 p2 = canvas->transformCoordinates(ImVec2(topLeft.x + size, topLeft.y));", "ImVec2 p2 = toImVec2(canvas->transformCoordinates(glm::dvec2(topLeft.x + size, topLeft.y)));")
text = text.replace("ImVec2 p3 = canvas->transformCoordinates(ImVec2(topLeft.x + size, topLeft.y + size));", "ImVec2 p3 = toImVec2(canvas->transformCoordinates(glm::dvec2(topLeft.x + size, topLeft.y + size)));")
text = text.replace("ImVec2 p4 = canvas->transformCoordinates(ImVec2(topLeft.x, topLeft.y + size));", "ImVec2 p4 = toImVec2(canvas->transformCoordinates(glm::dvec2(topLeft.x, topLeft.y + size)));")

text = text.replace("ImVec2 rectTopLeft = rectangle->getTopLeft();", "glm::dvec2 rectTopLeft = rectangle->getTopLeft();")
text = text.replace("ImVec2 rectP1 = canvas->transformCoordinates(rectTopLeft);", "ImVec2 rectP1 = toImVec2(canvas->transformCoordinates(rectTopLeft));")
text = text.replace("ImVec2 rectP2 = canvas->transformCoordinates(ImVec2(rectTopLeft.x + width, rectTopLeft.y));", "ImVec2 rectP2 = toImVec2(canvas->transformCoordinates(glm::dvec2(rectTopLeft.x + width, rectTopLeft.y)));")
text = text.replace("ImVec2 rectP3 = canvas->transformCoordinates(ImVec2(rectTopLeft.x + width, rectTopLeft.y + height));", "ImVec2 rectP3 = toImVec2(canvas->transformCoordinates(glm::dvec2(rectTopLeft.x + width, rectTopLeft.y + height)));")
text = text.replace("ImVec2 rectP4 = canvas->transformCoordinates(ImVec2(rectTopLeft.x, rectTopLeft.y + height));", "ImVec2 rectP4 = toImVec2(canvas->transformCoordinates(glm::dvec2(rectTopLeft.x, rectTopLeft.y + height)));")

text = text.replace("ImColor::HSV", "ImColor::HSV") # dummy to keep logic clean

with open("src/Renderer2D.cpp", "w") as f:
    f.write(text)
