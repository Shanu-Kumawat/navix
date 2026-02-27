import re
with open("src/Renderer2D.cpp", "r") as f: text = f.read()

# Replace any multi-wrapped toImVec2 calls
text = re.sub(r"toImVec2\(\s*Drawing::Math::toImVec2\(", r"toImVec2(", text)
text = re.sub(r"toImVec2\(\s*toImVec2\(", r"toImVec2(", text)
text = text.replace("))", "))") # just in case

# Fix the specific line
text = text.replace("toImVec2(canvas->transformCoordinates(points[i]))))", "toImVec2(canvas->transformCoordinates(points[i])))")
text = text.replace("toImVec2(canvas->transformCoordinates(points[i + 1]))))", "toImVec2(canvas->transformCoordinates(points[i + 1])))")
text = text.replace("toImVec2(canvas->transformCoordinates(points[i]))", "toImVec2(canvas->transformCoordinates(points[i]))") # standard
text = re.sub(r"toImVec2\(toImVec2\([^)]+\)\)", r"toImVec2", text) # fallback brute force

with open("src/Renderer2D.cpp", "w") as f: f.write(text)

