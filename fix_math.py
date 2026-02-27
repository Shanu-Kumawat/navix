import os

skip_files = [
    "include/ui",
    "src/ui",
    "include/Renderer2D.hpp",
    "src/Renderer2D.cpp",
    "main.cpp",
    "include/utils/VectorMath.hpp"
]

def sh_should_skip(path):
    for skip in skip_files:
        if path.startswith(skip) or skip in path:
            return True
    return False

for root, _, files in os.walk("."):
    for file in files:
        if not file.endswith((".cpp", ".hpp", ".h", ".c")):
            continue
            
        path = os.path.join(root, file)
        path = path.replace("./", "")
        if path.startswith("external") or path.startswith("build") or path.startswith("glad") or path.startswith("imgui"):
            continue
            
        if sh_should_skip(path):
            continue
            
        with open(path, "r") as f:
            content = f.read()
            
        orig = content
        
        # Replacements
        content = content.replace("Math::distance", "glm::distance")
        content = content.replace("Drawing::Math::distance", "glm::distance")
        content = content.replace("Math::normalize", "glm::normalize")
        content = content.replace("Drawing::Math::normalize", "glm::normalize")
        content = content.replace("Math::dot", "glm::dot")
        content = content.replace("Drawing::Math::dot", "glm::dot")
        content = content.replace("Math::length", "glm::length")
        content = content.replace("Math::lerp", "glm::mix") # GLM uses mix instead of lerp
        
        if content != orig:
            if "#include <glm/glm.hpp>" not in content:
                content = "#include <glm/glm.hpp>\n" + content
            with open(path, "w") as f:
                f.write(content)
            print(f"Updated {path}")
