#include "meshing/GmshExtractor.hpp"
#include <iostream>

#ifdef USE_GMSH
#include <gmsh.h>
#endif

namespace Core {
namespace Meshing {

bool GmshExtractor::extractMesh(Mesh& target, int dim) {
    target.clear();
#ifdef USE_GMSH
    try {
        // Retrieve Nodes
        std::vector<std::size_t> nodeTags;
        std::vector<double> coord, parametricCoord;
        gmsh::model::mesh::getNodes(nodeTags, coord, parametricCoord, dim, -1, true, false);

        for (size_t i = 0; i < nodeTags.size(); ++i) {
            target.addNode(
                nodeTags[i], 
                coord[i * 3], 
                coord[i * 3 + 1], 
                coord[i * 3 + 2]
            );
        }

        // Retrieve Elements
        std::vector<int> elementTypes;
        std::vector<std::vector<std::size_t>> elementTags, elementNodeTags;
        gmsh::model::mesh::getElements(elementTypes, elementTags, elementNodeTags, dim);

        for (size_t i = 0; i < elementTypes.size(); ++i) {
            int elementType = elementTypes[i];
            const auto& tagsList = elementTags[i];
            const auto& nodesList = elementNodeTags[i];
            
            // Get nodes per element type to slice the flat array
            std::string name;
            int dim_e, order, numNodes, numPrimaryNodes;
            std::vector<double> localNodeCoord;
            gmsh::model::mesh::getElementProperties(
                elementType, name, dim_e, order, numNodes, localNodeCoord, numPrimaryNodes
            );

            for (size_t j = 0; j < tagsList.size(); ++j) {
                std::vector<uint64_t> eNodes;
                for (int k = 0; k < numNodes; ++k) {
                    eNodes.push_back(nodesList[j * numNodes + k]);
                }
                target.addElement(tagsList[j], elementType, eNodes);
            }
        }
        
        std::cout << "[GmshExtractor] Extracted " << target.getNodes().size() 
                  << " nodes and " << target.getElements().size() << " elements." << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[GmshExtractor] Extraction failed: " << e.what() << std::endl;
        return false;
    }
#else
    std::cerr << "[GmshExtractor] Compiled without Gmsh support. Cannot extract." << std::endl;
    return false;
#endif
}

} // namespace Meshing
} // namespace Core
