import sys

with open('CMakeLists.txt', 'r') as f:
    lines = f.readlines()

new_lines = []
for line in lines:
    if "if(GMSH_FOUND)" in line and "target_link_libraries(main PRIVATE" not in ''.join(new_lines[-5:]):
        pass # Skip adding it repeatedly just in case
    new_lines.append(line)

with open('CMakeLists.txt', 'w') as f:
    f.writelines(new_lines)
