import os, glob, re

def process_file(filepath):
    with open(filepath, 'r') as f:
        content = f.read()

    # Skip files that are strictly UI based if needed, but we'll apply it widely first on non-UI files.
    
    # We mainly target shapes and models
    content = content.replace('ImVec2', 'glm::dvec2')
    
    # Mathematical functions we might encounter
    # Distance(a, b) -> glm::distance(a, b)
    content = re.sub(r'\bDistance\b', 'glm::distance', content)
    
    # Add(a, b) -> a + b
    # Actually Add and Subtract are overloaded in some files, simple replacement might be tough.
    
    with open(filepath, 'w') as f:
        f.write(content)

