import re
with open("include/shapes/BasicShapes.hpp", "r") as f:
    content = f.read()

# Includes
content = content.replace("#include <imgui.h>", "#include <glm/glm.hpp>\n#include \"utils/VectorMath.hpp\"\n#include <imgui.h>\n")

# Free functions at the bottom
content = re.sub(r'inline float Distance\(.*?\n}\n\n', '', content, flags=re.DOTALL)
content = re.sub(r'inline ImVec2 Add\(.*?\n}\n\n', '', content, flags=re.DOTALL)
content = re.sub(r'inline ImVec2 Subtract\(.*?\n}\n\n', '', content, flags=re.DOTALL)
# Or I can just strip all inline math from the bottom, let's use a simpler approach

content = content.replace("ImVec2", "glm::dvec2")

with open("include/shapes/BasicShapes.hpp", "w") as f:
    f.write(content)

