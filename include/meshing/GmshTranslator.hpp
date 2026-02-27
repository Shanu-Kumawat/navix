#pragma once

#include "topology/TopologyManager.hpp"
#include <vector>
#include <string>
#include "meshing/Mesh.hpp"
#include "meshing/Mesh.hpp"

namespace Core {
namespace Meshing {

/**
 * @brief Handles the synchronization between Core::Topology elements and the Gmsh API.
 */
class GmshTranslator {
public:
    GmshTranslator() = default;
    ~GmshTranslator() = default;

    /**
     * @brief Initializes the Gmsh API context.
     * @return true if successful, false otherwise.
     */
    bool initialize();

    /**
     * @brief Finalizes the Gmsh API context and clears internal geometry.
     */
    void finalize();

    /**
     * @brief Pushes the custom topological data (Nodes, Edges, Faces) into Gmsh's OCC geometry engine.
     * 
     * @param topologyManager The dictionary of geometric boundaries.
     * @param synchronize If true, instructs Gmsh to synchronize the CAD kernel to the model.
     * @return true if successfully translated.
     */
    bool translateTopologyToGmsh(const Core::Topology::TopologyManager& topologyManager, bool synchronize = true);

    /**
     * @brief Triggers 2D (and optionally 3D) mesh generation within Gmsh based on the translated topology.
     * 
     * @param dimensions Target mesh dimension (1 = curve, 2 = surface, 3 = volume).
     * @param elementSize Target mesh size constraint.
     */
    void generateMesh(int dimensions = 2, double elementSize = 0.5);

    /**
     * @brief Optional debug utility to launch the Gmsh GUI to observe what was generated.
     */
    void launchGmshGUI();

private:
    bool isInitialized = false;
    /**
     * @brief Extract the generated mesh from Gmsh
     * @param outMesh Mesh storage container
     * @param dim Dimension of mesh to extract
     */
    bool extractGeneratedMesh(Mesh& outMesh, int dim = 2);
};

} // namespace Meshing
} // namespace Core
