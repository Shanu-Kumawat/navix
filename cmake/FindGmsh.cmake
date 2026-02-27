# Find Gmsh
# Find the Gmsh includes and library

# Define search paths for local vendored Gmsh first, then system paths
set(GMSH_SEARCH_INCLUDES
    ${CMAKE_SOURCE_DIR}/external/gmsh/include
    /usr/include
    /usr/local/include
    /opt/local/include
)

set(GMSH_SEARCH_LIBS
    ${CMAKE_SOURCE_DIR}/external/gmsh/lib
    /usr/lib
    /usr/local/lib
    /opt/local/lib
)

find_path(GMSH_INCLUDE_DIR gmsh.h
    PATHS ${GMSH_SEARCH_INCLUDES}
    NO_DEFAULT_PATH
)
if(NOT GMSH_INCLUDE_DIR)
    find_path(GMSH_INCLUDE_DIR gmsh.h)
endif()

find_library(GMSH_LIBRARY NAMES gmsh
    PATHS ${GMSH_SEARCH_LIBS}
    NO_DEFAULT_PATH
)
if(NOT GMSH_LIBRARY)
    find_library(GMSH_LIBRARY NAMES gmsh)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Gmsh DEFAULT_MSG GMSH_LIBRARY GMSH_INCLUDE_DIR)

mark_as_advanced(GMSH_INCLUDE_DIR GMSH_LIBRARY)

# Copy the dynamic library to the build directory if found locally
if(GMSH_FOUND AND GMSH_LIBRARY MATCHES "external/gmsh/lib/libgmsh.so")
endif()
