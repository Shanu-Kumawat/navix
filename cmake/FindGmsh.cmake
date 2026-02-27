# Find Gmsh
# Find the Gmsh includes and library

# Define paths to look for
set(GMSH_SEARCH_PATHS
    "${CMAKE_SOURCE_DIR}/external/gmsh"
    /usr
    /usr/local
    /opt/local
)

find_path(GMSH_INCLUDE_DIR gmsh.h
    PATHS ${GMSH_SEARCH_PATHS}
    PATH_SUFFIXES include
)

find_library(GMSH_LIBRARY NAMES gmsh
    PATHS ${GMSH_SEARCH_PATHS}
    PATH_SUFFIXES lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Gmsh DEFAULT_MSG GMSH_LIBRARY GMSH_INCLUDE_DIR)

mark_as_advanced(GMSH_INCLUDE_DIR GMSH_LIBRARY)
