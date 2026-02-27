import re

with open("src/shapes/ComplexShapes.cpp", "r") as f:
    text = f.read()

# Fix floats
text = text.replace("std::clamp(", "std::clamp<double>(")
text = text.replace("2.0f", "2.0")

# Fix drawControlPoint
text = text.replace("drawList->AddCircleFilled(pos, size", "drawList->AddCircleFilled(Drawing::Math::toImVec2(pos), size")
text = text.replace("drawList->AddCircle(pos, size + 1.0f", "drawList->AddCircle(Drawing::Math::toImVec2(pos), size + 1.0f")

# Fix drawControlPolygon
text = text.replace("drawList->AddLine(points[i - 1], points[i]", "drawList->AddLine(Drawing::Math::toImVec2(points[i - 1]), Drawing::Math::toImVec2(points[i])")

# Fix drawTangentHandles
text = text.replace("drawList->AddLine(point, tangent", "drawList->AddLine(Drawing::Math::toImVec2(point), Drawing::Math::toImVec2(tangent)")

# Make sure drawing math is included
if "using namespace Drawing::Math;" not in text:
    text = text.replace("namespace Drawing {\n", "namespace Drawing {\nusing namespace Drawing::Math;\n")

# Extra fallback for any clamp without template
text = text.replace("std::clamp<double><double>", "std::clamp<double>")
text = text.replace("0.0f, 1.0f", "0.0, 1.0")

with open("src/shapes/ComplexShapes.cpp", "w") as f:
    f.write(text)

