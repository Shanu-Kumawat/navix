import re

with open("src/Renderer2D.cpp", "r") as f:
    text = f.read()

# Fix Square points creation
text = text.replace("canvas->transformCoordinates(ImVec2(topLeft.x + size, topLeft.y))", "toImVec2(canvas->transformCoordinates(glm::dvec2(topLeft.x + size, topLeft.y)))")
text = text.replace("canvas->transformCoordinates(ImVec2(topLeft.x + size, topLeft.y + size))", "toImVec2(canvas->transformCoordinates(glm::dvec2(topLeft.x + size, topLeft.y + size)))")
text = text.replace("canvas->transformCoordinates(ImVec2(topLeft.x, topLeft.y + size))", "toImVec2(canvas->transformCoordinates(glm::dvec2(topLeft.x, topLeft.y + size)))")
text = text.replace("canvas->transformCoordinates(topLeft)", "toImVec2(canvas->transformCoordinates(topLeft))")

# Fix Rectangle points
text = text.replace("ImVec2 topLeft = rect->getTopLeft();", "glm::dvec2 topLeft = rect->getTopLeft();")
text = text.replace("ImVec2 size = rect->getSize();", "glm::dvec2 size = rect->getSize();")
text = text.replace("canvas->transformCoordinates(ImVec2(topLeft.x + size.x, topLeft.y))", "toImVec2(canvas->transformCoordinates(glm::dvec2(topLeft.x + size.x, topLeft.y)))")
text = text.replace("canvas->transformCoordinates(ImVec2(topLeft.x + size.x, topLeft.y + size.y))", "toImVec2(canvas->transformCoordinates(glm::dvec2(topLeft.x + size.x, topLeft.y + size.y)))")
text = text.replace("canvas->transformCoordinates(ImVec2(topLeft.x, topLeft.y + size.y))", "toImVec2(canvas->transformCoordinates(glm::dvec2(topLeft.x, topLeft.y + size.y)))")

# Fix Spline points
text = text.replace("std::vector<ImVec2> points = spline->calculatePoints(0.01f);", "std::vector<glm::dvec2> points = spline->calculatePoints(0.01f);")
text = text.replace("std::vector<glm::dvec2> points = spline->calculatePoints(0.01);", "std::vector<glm::dvec2> points = spline->calculatePoints(0.01);") # fallback

# Fix bounding box
text = text.replace("points[i]", "points[i]") # just in case
text = re.sub(r"canvas->transformCoordinates\((points\[i\])\)", r"toImVec2(canvas->transformCoordinates(\1))", text)
text = re.sub(r"canvas->transformCoordinates\((points\[i \+ 1\])\)", r"toImVec2(canvas->transformCoordinates(\1))", text)
text = re.sub(r"toImVec2\(toImVec2", r"toImVec2", text) # clean up double replacements

with open("src/Renderer2D.cpp", "w") as f:
    f.write(text)
