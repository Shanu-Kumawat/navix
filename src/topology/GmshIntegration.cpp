#include "topology/GmshIntegration.hpp"
#include <iostream>

namespace Core {
namespace Topology {

GmshIntegration::~GmshIntegration() {
    finalize();
}

bool GmshIntegration::initialize() {
#ifdef USE_GMSH
    if (!gmsh::isInitialized()) {
        try {
            gmsh::initialize();
            gmsh::option::setNumber("General.Terminal", 1);
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Gmsh initialization failed: " << e.what() << std::endl;
            return false;
        }
    }
    return true;
#else
    std::cerr << "Gmsh support is not compiled into Navix!" << std::endl;
    return false;
#endif
}

void GmshIntegration::finalize() {
#ifdef USE_GMSH
    if (gmsh::isInitialized()) {
        gmsh::finalize();
    }
#endif
}

bool GmshIntegration::meshTopology(std::shared_ptr<TopologyManager> topologyManager, double meshSize) {
#ifdef USE_GMSH
    if (!gmsh::isInitialized() && !initialize()) {
        return false;
    }

    try {
        gmsh::model::add("NavixMesh");

        // Loop over nodes, edges, faces and create Geo Entities ...
        for (const auto& [id, node] : topologyManager->getNodes()) {
            gmsh::model::geo::addPoint(
                node->getPosition().x, 
                node->getPosition().y, 
                node->getPosition().z, 
                meshSize, 
                id
            );
        }

        gmsh::model::geo::synchronize();
        gmsh::model::mesh::generate(2);
        
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Gmsh geometry error: " << e.what() << std::endl;
        return false;
    }
#else
    return false;
#endif
}

} // namespace Topology
} // namespace Core
