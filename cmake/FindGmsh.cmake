# Find Gmsh
# First we check the localized SDK placed in external/gmsh
# If that is not found, we check generic system paths.

set(GMSH_LOCAL_DIR "${CMAKE_SOURCE_DIR}/external/gmsh")

find_path(GMSH_INCLUDE_DIR gmsh.h
    PATHS
    "${GMSH_LOCAL_DIR}/include"
    /usr/include
    /usr/local/include
    /opt/local/include
)

find_library(GMSH_LIBRARY NAMES gmsh
    PATHS
    "${GMSH_LOCAL_DIR}/lib"
    /usr/lib
    /usr/local/lib
    /opt/local/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Gmsh DEFAULT_MSG GMSH_LIBRARY GMSH_INCLUDE_DIR)

mark_as_advanced(GMSH_INCLUDE_DIR GMSH_LIBRARY)
