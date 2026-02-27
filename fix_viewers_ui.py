with open("src/ui/Viewers3DUI.cpp", "r") as f:
    text = f.read()

text = text.replace("ImVec2 viewportSize =", "glm::dvec2 viewportSize =")
text = text.replace("ImVec2(availableSize.x", "glm::dvec2(availableSize.x")

with open("src/ui/Viewers3DUI.cpp", "w") as f:
    f.write(text)
