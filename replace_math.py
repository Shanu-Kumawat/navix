import os, glob

# Directories to process
dirs = [
    "include/shapes",
    "src/shapes",
    "include",
    "src"
]

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

for d in dirs:
    for root, _, files in os.walk(d):
        for file in files:
            if not file.endswith((".cpp", ".hpp", ".h", ".c")):
                continue
                
            path = os.path.join(root, file)
            if sh_should_skip(path):
                continue
                
            with open(path, "r") as f:
                content = f.read()
                
            orig = content
            
            # Use GLM instead of ImVec2
            content = content.replace("ImVec2", "glm::dvec2")
            
            if content != orig:
                # Make sure we include glm
                if "#include <glm/glm.hpp>" not in content:
                    content = "#include <glm/glm.hpp>\n" + content
                with open(path, "w") as f:
                    f.write(content)
                print(f"Updated {path}")
