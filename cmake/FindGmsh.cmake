# Find Gmsh
# Find the Gmsh includes and library

find_path(GMSH_INCLUDE_DIR gmsh.h
    PATHS
    ${CMAKE_SOURCE_DIR}/external/gmsh/include
    /usr/include
    /usr/local/include
    /opt/local/include
)

find_library(GMSH_LIBRARY NAMES gmsh
    PATHS
    ${CMAKE_SOURCE_DIR}/external/gmsh/lib
    /usr/lib
    /usr/local/lib
    /opt/local/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Gmsh DEFAULT_MSG GMSH_LIBRARY GMSH_INCLUDE_DIR)

mark_as_advanced(GMSH_INCLUDE_DIR GMSH_LIBRARY)
