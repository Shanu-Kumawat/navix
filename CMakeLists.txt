cmake_minimum_required(VERSION 3.10)
project(opengl_project LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(glm REQUIRED)

# Set OpenGL preference to GLVND
set(OpenGL_GL_PREFERENCE GLVND)

# Check if we are on NixOS or Ubuntu
if(EXISTS "/run/opengl-driver/lib/libGL.so")
    # NixOS-specific paths
    set(OPENGL_opengl_LIBRARY "/run/opengl-driver/lib/libGL.so")
    set(OPENGL_glx_LIBRARY "/run/opengl-driver/lib/libGLX.so")
    set(OPENGL_INCLUDE_DIR "/run/opengl-driver/include")
    message(STATUS "Using NixOS OpenGL paths")
else()
    # Standard paths (Ubuntu, etc.)
    message(STATUS "Using standard OpenGL paths")
endif()

# Find OpenGL
find_package(OpenGL REQUIRED)

# Find SDL2 using pkg-config
pkg_check_modules(SDL2 REQUIRED sdl2)
link_directories(${SDL2_LIBRARY_DIRS})

# Print debug information
message(STATUS "OpenGL libraries: ${OPENGL_LIBRARIES}")
message(STATUS "OpenGL include dir: ${OPENGL_INCLUDE_DIR}")
message(STATUS "SDL2 libraries: ${SDL2_LIBRARIES}")
message(STATUS "SDL2 include dirs: ${SDL2_INCLUDE_DIRS}")

# Add glad
add_library(glad STATIC ${CMAKE_CURRENT_SOURCE_DIR}/glad/src/glad.c)
target_include_directories(glad PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/glad/include
    ${OPENGL_INCLUDE_DIR}
)
set_target_properties(glad PROPERTIES 
    LANGUAGE C
    POSITION_INDEPENDENT_CODE ON
)

# Add ImGui
set(IMGUI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_sdl2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends
    ${SDL2_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/glad/include
    ${OPENGL_INCLUDE_DIR}
)
target_link_libraries(imgui PUBLIC 
    ${SDL2_LIBRARIES}
    ${OPENGL_LIBRARIES}
)

# Add Drawing library
add_library(drawing STATIC
    src/Canvas.cpp
    src/shapes/ComplexShapes.cpp
    src/shapes/Bellows.cpp
    src/shapes/ShockAbsorberBottomEnd.cpp
    src/shapes/Dimension.cpp
    src/shapes/BasicShapes.cpp
    src/utils/MathUtils.cpp
    src/Shader.cpp
    src/BellowsModel3D.cpp
    src/BellowsViewer3D.cpp
    src/Camera.cpp
    src/SpringModel3D.cpp
    src/SpringViewer3D.cpp
    src/ShockAbsorberModel3D.cpp
    src/ShockAbsorberViewer3D.cpp
)

target_include_directories(drawing PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
)

target_link_libraries(drawing PUBLIC
    imgui
)

# Create main executable
add_executable(main main.cpp)

# Set include directories for main
target_include_directories(main PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/glad/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends
    ${SDL2_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
)

# Link libraries for main
target_link_libraries(main PRIVATE
    glad
    imgui
    drawing
    ${SDL2_LIBRARIES}
    ${OPENGL_LIBRARIES}
    glm::glm
)

if(UNIX)
    target_link_libraries(main PRIVATE dl)
endif()

# Update drawing library include directories
target_include_directories(drawing PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
)

# Copy shader files to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})
