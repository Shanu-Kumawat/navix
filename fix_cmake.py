import sys

with open('CMakeLists.txt', 'r') as f:
    lines = f.readlines()

new_lines = []
for line in lines:
    new_lines.append(line)
    if "project(Navix)" in line:
        new_lines.extend([
            "\n",
            "set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} \"${CMAKE_SOURCE_DIR}/cmake\")\n",
            "# Optional Gmsh dependency for meshing\n",
            "find_package(Gmsh)\n",
            "if(GMSH_FOUND)\n",
            "    add_definitions(-DUSE_GMSH)\n",
            "    include_directories(${GMSH_INCLUDE_DIR})\n",
            "    message(STATUS \"Gmsh found: Meshing enabled\")\n",
            "else()\n",
            "    message(WARNING \"Gmsh NOT found. Meshing features will be disabled.\")\n",
            "endif()\n"
        ])
    if "target_link_libraries(main PRIVATE drawing glad imgui)" in line:
        new_lines.extend([
            "if(GMSH_FOUND)\n",
            "    target_link_libraries(main PRIVATE ${GMSH_LIBRARY})\n",
            "endif()\n"
        ])

with open('CMakeLists.txt', 'w') as f:
    f.writelines(new_lines)
