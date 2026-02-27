#pragma once

#ifdef USE_GMSH
#include <gmsh.h>
#endif

#include "TopologyManager.hpp"
#include <string>
#include <memory> 

namespace Core {
namespace Topology {

class GmshIntegration {
public:
    GmshIntegration() = default;
    ~GmshIntegration();

    // Initialize the Gmsh session
    static bool initialize();
    
    // Finalize the Gmsh session
    static void finalize();

    // Run meshing pipeline over a given topology and return success state
    bool meshTopology(std::shared_ptr<TopologyManager> topologyManager, double meshSize);

private:
    bool isInitialized{false};
};

} // namespace Topology
} // namespace Core
