#pragma once

#include "Mesh.hpp"

namespace Core {
namespace Meshing {

/**
 * @brief Utility structured to extract a generated runtime mesh from Gmsh
 * back into the standard `Core::Meshing::Mesh` objects array for FEA handling.
 */
class GmshExtractor {
public:
    GmshExtractor() = default;
    ~GmshExtractor() = default;

    /**
     * @brief Pulls currently generated nodes and elements from Gmsh active context.
     * @param target Mesh container to populate.
     * @param dim Extract dimension (e.g. 2 for surface meshes)
     * @return true if successful
     */
    bool extractMesh(Mesh& target, int dim = 2);
};

} // namespace Meshing
} // namespace Core
