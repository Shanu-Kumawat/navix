#include "meshing/GmshTranslator.hpp"

#include "meshing/GmshExtractor.hpp"
#ifdef USE_GMSH
#include <gmsh.h>
#endif

#include <iostream>

namespace Core {
namespace Meshing {

bool GmshTranslator::initialize() {
#ifdef USE_GMSH
    if (isInitialized) return true;
    try {
        gmsh::initialize();
        // Hide terminal output unless explicitly requesting errors
        gmsh::option::setNumber("General.Terminal", 1); 
        isInitialized = true;
        std::cout << "[GmshTranslator] Context initialized." << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[GmshTranslator] Initialization failed: " << e.what() << std::endl;
        return false;
    }
#else
    std::cerr << "[GmshTranslator] Built without USE_GMSH. Cannot initialize." << std::endl;
    return false;
#endif
}

void GmshTranslator::finalize() {
#ifdef USE_GMSH
    if (isInitialized) {
        gmsh::finalize();
        isInitialized = false;
        std::cout << "[GmshTranslator] Context finalized." << std::endl;
    }
#endif
}

bool GmshTranslator::translateTopologyToGmsh(const Core::Topology::TopologyManager& topologyManager, bool synchronize) {
#ifdef USE_GMSH
    if (!isInitialized && !initialize()) return false;

    try {
        // Clear previous geometry to avoid stacking IDs
        gmsh::clear();
        gmsh::model::add("Navix_Topology");

        // 1. Translate Nodes to Gmsh Points
        const auto& nodes = topologyManager.getNodes();
        for (const auto& [id, node] : nodes) {
            auto pos = node->getPosition();
            gmsh::model::occ::addPoint(pos.x, pos.y, pos.z, 0.0, static_cast<int>(id));
        }

        // 2. Translate Edges to Gmsh Lines
        const auto& edges = topologyManager.getEdges();
        for (const auto& [id, edge] : edges) {
            gmsh::model::occ::addLine(
                static_cast<int>(edge->getStartNodeId()), 
                static_cast<int>(edge->getEndNodeId()), 
                static_cast<int>(id)
            );
        }

        // 3. Translate Faces to Gmsh CurveLoops + PlaneSurfaces
        const auto& faces = topologyManager.getFaces();
        for (const auto& [id, face] : faces) {
            std::vector<int> edgeTags;
            for (auto edgeId : face->getEdgeIds()) {
                edgeTags.push_back(static_cast<int>(edgeId));
            }
            
            // Create a CurveLoop bounding the surface
            int curveLoopId = gmsh::model::occ::addCurveLoop(edgeTags);
            // Create the PlaneSurface bounded by the CurveLoop
            gmsh::model::occ::addPlaneSurface({curveLoopId}, static_cast<int>(id));
        }

        if (synchronize) {
            gmsh::model::occ::synchronize();
            std::cout << "[GmshTranslator] Translated geometry boundaries via OCC sync." << std::endl;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "[GmshTranslator] Translation failed: " << e.what() << std::endl;
        return false;
    }
#else
    return false;
#endif
}

void GmshTranslator::generateMesh(int dimensions, double elementSize) {
#ifdef USE_GMSH
    if (!isInitialized) return;
    try {
        // Set global element size directly before meshing
        gmsh::option::setNumber("Mesh.CharacteristicLengthMin", elementSize * 0.5);
        gmsh::option::setNumber("Mesh.CharacteristicLengthMax", elementSize);
        
        gmsh::model::mesh::generate(dimensions);
        std::cout << "[GmshTranslator] Meshing complete for dimension: " << dimensions << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[GmshTranslator] Meshing failed: " << e.what() << std::endl;
    }
#endif
}

void GmshTranslator::launchGmshGUI() {
#ifdef USE_GMSH
    if (!isInitialized) return;
    try {
        gmsh::fltk::run();
    } catch (const std::exception& e) {
        std::cerr << "[GmshTranslator] FLTK GUI launch failed: " << e.what() << std::endl;
    }
#endif
}

bool GmshTranslator::extractGeneratedMesh(Mesh& outMesh, int dim) {
    GmshExtractor extractor;
    return extractor.extractMesh(outMesh, dim);
}

} // namespace Meshing
} // namespace Core
