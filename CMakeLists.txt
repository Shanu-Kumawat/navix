cmake_minimum_required(VERSION 3.10)
project(opengl_project LANGUAGES C CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
# Optional Gmsh dependency for meshing
find_package(Gmsh)
if(GMSH_FOUND)
    add_definitions(-DUSE_GMSH)
    include_directories(${GMSH_INCLUDE_DIR})
    message(STATUS "Gmsh found: Meshing enabled")
else()
    message(WARNING "Gmsh NOT found. Meshing features will be disabled.")
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Windows-specific settings
if(WIN32)
    # Use local GLM for Windows builds
    set(GLM_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/deps/glm")
    message(STATUS "Using local GLM: ${GLM_INCLUDE_DIR}")
    
    # Define SDL2 paths manually
    set(SDL2_PATH "${CMAKE_SOURCE_DIR}/deps/SDL2-2.26.5")
    set(SDL2_INCLUDE_DIRS "${SDL2_PATH}/x86_64-w64-mingw32/include;${SDL2_PATH}/x86_64-w64-mingw32/include/SDL2")
    
    # Add SDL2 include directory definition for ImGui
    add_definitions(-DIMGUI_IMPL_SDL_INCLUDE_SDL2)
    
    # Set library paths
    set(SDL2_LIBRARIES "${SDL2_PATH}/x86_64-w64-mingw32/lib/libSDL2.dll.a;${SDL2_PATH}/x86_64-w64-mingw32/lib/libSDL2main.a")
    set(SDL2_DLL_PATH "${SDL2_PATH}/x86_64-w64-mingw32/bin/SDL2.dll")
    
    # Create a version info resource file
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/windows_resources/app.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/app.rc
        @ONLY
    )
else()
    # Linux-specific paths
    find_package(PkgConfig REQUIRED)
    find_package(glm REQUIRED)
    
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
    
    # Find SDL2 using pkg-config on Linux
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(SDL2_IMAGE SDL2_image)
    if(SDL2_IMAGE_FOUND)
        message(STATUS "SDL2_image found - image loading will be enabled")
        link_directories(${SDL2_LIBRARY_DIRS} ${SDL2_IMAGE_LIBRARY_DIRS})
        add_definitions(-DHAVE_SDL2_IMAGE)
    else()
        message(WARNING "SDL2_image not found - image loading will be disabled")
        set(SDL2_IMAGE_LIBRARIES "")
        set(SDL2_IMAGE_INCLUDE_DIRS "")
    endif()
endif()

# Set OpenGL preference to GLVND
set(OpenGL_GL_PREFERENCE GLVND)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Print debug information
message(STATUS "OpenGL libraries: ${OPENGL_LIBRARIES}")
message(STATUS "OpenGL include dir: ${OPENGL_INCLUDE_DIR}")
message(STATUS "SDL2 libraries: ${SDL2_LIBRARIES}")
message(STATUS "SDL2 include dirs: ${SDL2_INCLUDE_DIRS}")
message(STATUS "SDL2_image libraries: ${SDL2_IMAGE_LIBRARIES}")
message(STATUS "SDL2_image include dirs: ${SDL2_IMAGE_INCLUDE_DIRS}")

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

# Fix for ImGui SDL2 backend - create an interface for SDL2 includes
if(WIN32)
    add_library(sdl2_interface INTERFACE)
    target_include_directories(sdl2_interface INTERFACE ${SDL2_INCLUDE_DIRS})
    target_compile_definitions(sdl2_interface INTERFACE -DIMGUI_IMPL_SDL_INCLUDE_SDL2)
endif()

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

if(WIN32)
    target_link_libraries(imgui PUBLIC sdl2_interface)
else()
    target_link_libraries(imgui PUBLIC ${SDL2_LIBRARIES})
endif()

target_link_libraries(imgui PUBLIC ${OPENGL_LIBRARIES})

# Add Drawing library
add_library(drawing STATIC
    src/ApplicationContext.cpp
    src/SceneModel.cpp
    src/Renderer2D.cpp
    src/InputController.cpp
    src/Canvas.cpp
    src/commands/CommandManager.cpp
    src/topology/TopologyManager.cpp
    src/meshing/GmshTranslator.cpp
    src/topology/GmshIntegration.cpp
    src/core/ImGuiRenderer.cpp
    src/ui/TopRibbon.cpp
    src/ui/UIState.cpp
    src/ui/UIHelpers.cpp
    src/ui/PropertyPanel.cpp
    src/ui/StatusBar.cpp
    src/ui/CanvasView.cpp
    src/ui/Viewers3DUI.cpp
    src/shapes/ComplexShapes.cpp
    src/shapes/Bellows.cpp
    src/shapes/BallBearing.cpp
    src/shapes/ShockAbsorberBottomEnd.cpp
    src/shapes/Dimension.cpp
    src/shapes/BasicShapes.cpp
    src/utils/MathUtils.cpp
    src/Shader.cpp
    src/Base3DViewer.cpp
    src/Base3DModel.cpp
    src/BellowsModel3D.cpp
    src/BellowsViewer3D.cpp
    src/BallBearingModel3D.cpp
    src/BallBearingViewer3D.cpp
    src/Camera.cpp
    src/SpringModel3D.cpp
    src/SpringViewer3D.cpp
    src/ShockAbsorberModel3D.cpp
    src/ShockAbsorberViewer3D.cpp
    src/ComplexShape3DManager.cpp
)

target_include_directories(drawing PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
)

if(WIN32)
    target_include_directories(drawing PUBLIC
        ${GLM_INCLUDE_DIR}
    )
endif()

target_link_libraries(drawing PUBLIC
    imgui
)

# Create main executable
if(WIN32)
    add_executable(main WIN32 main.cpp)
    # For Windows GUI app using SDL, we need to include the SDL2main.c file
    # which provides a WinMain entry point that calls into our main()
    target_compile_definitions(main PRIVATE -DSDL_MAIN_HANDLED)
else()
    add_executable(main main.cpp)
endif()

# Set include directories for main
target_include_directories(main PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/glad/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
)

if(WIN32)
    target_include_directories(main PRIVATE
        ${GLM_INCLUDE_DIR}
    )
endif()

# Link libraries for main
target_link_libraries(main PRIVATE
    glad
    imgui
    drawing
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${OPENGL_LIBRARIES}
)

if(NOT WIN32)
    target_link_libraries(main PRIVATE
        glm::glm
    )
endif()

if(UNIX)
    target_link_libraries(main PRIVATE dl)
endif()

if(WIN32)
    # Add Windows-specific libraries
    target_link_libraries(main PRIVATE
        opengl32
        gdi32
        winmm
        user32
        shell32
        ole32
        mingw32
    )
endif()

# Add post-build command for Windows to copy SDL2.dll
if(WIN32)
    add_custom_command(TARGET main POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SDL2_DLL_PATH}
            $<TARGET_FILE_DIR:main>
    )
endif()

# Update drawing library include directories
target_include_directories(drawing PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
)

# Copy shader files to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR}) 
if(GMSH_FOUND)
    target_link_libraries(main PRIVATE ${GMSH_LIBRARY})
endif()

# Ensure DLL/SO is copied
if(GMSH_FOUND AND GMSH_LIBRARY MATCHES "external/gmsh/lib/libgmsh.so")
    add_custom_command(TARGET main POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${GMSH_LIBRARY}
        ${CMAKE_BINARY_DIR}/
    )
endif()
