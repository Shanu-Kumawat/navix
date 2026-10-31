#include "export/FEMExporter.hpp"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <ctime>

namespace Export {

ExportResult FEMExporter::exportAbaqusINP(const Core::Meshing::Mesh& mesh,
                                          const std::string& partName,
                                          const Core::FEM::Material& material,
                                          const std::string& filePath,
                                          float shellThickness,
                                          double sceneToMM) {
    ExportResult result;
    result.filePath = filePath;

    if (mesh.getNodes().empty() || mesh.getElements().empty()) {
        result.errorMessage = "Cannot export: FEM mesh is empty. Generate the mesh first.";
        std::cerr << "[FEMExporter] " << result.errorMessage << std::endl;
        return result;
    }

    std::ofstream f(filePath);
    if (!f.is_open()) {
        result.errorMessage = "Failed to open file for writing: " + filePath;
        std::cerr << "[FEMExporter] " << result.errorMessage << std::endl;
        return result;
    }

    std::time_t now = std::time(nullptr);
    char timeBuf[64];
    std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

    f << "*Heading\n";
    f << "** NAVIX FEM Export — Abaqus Input Deck (" << partName << ")\n";
    f << "** Generated: " << timeBuf << "\n";
    f << "** Software:  NAVIX CAD/FEM\n";
    f << "** Units: mm, tonne, s, N, MPa\n";
    f << "**\n";
    f << "*Part, name=" << partName << "\n";
    f << "**\n";

    // Nodes (mesh stores scene units; sceneToMM converts to millimetres)
    const auto& nodes = mesh.getNodes();
    f << "** " << nodes.size() << " nodes\n";
    f << "*Node\n";
    f << std::fixed << std::setprecision(6);
    for (const auto& node : nodes) {
        f << std::setw(8) << node.tag << ", "
          << std::setw(14) << (node.position.x * sceneToMM) << ", "
          << std::setw(14) << (node.position.y * sceneToMM) << ", "
          << std::setw(14) << (node.position.z * sceneToMM) << "\n";
    }
    f << "**\n";

    // Elements (S3 triangular shells; gmsh type 2)
    const auto& elements = mesh.getElements();
    size_t triCount = 0;
    for (const auto& e : elements)
        if (e.elementType == 2 && e.nodeTags.size() == 3) ++triCount;

    f << "** " << triCount << " triangular shell elements\n";
    f << "*Element, type=S3, ELSET=" << partName << "_SURFACE\n";
    for (const auto& e : elements) {
        if (e.elementType != 2 || e.nodeTags.size() != 3) continue;
        f << std::setw(8) << e.tag << ", "
          << std::setw(8) << e.nodeTags[0] << ", "
          << std::setw(8) << e.nodeTags[1] << ", "
          << std::setw(8) << e.nodeTags[2] << "\n";
    }
    f << "**\n";

    // Material (SI base → mm-consistent units)
    double E_MPa = material.youngsModulus / 1.0e6;
    double nu = material.poissonsRatio;
    double rho_tonne_mm3 = material.density * 1.0e-12;
    f << "*Material, name=" << material.name << "\n";
    f << "*Elastic\n" << std::fixed << std::setprecision(1) << E_MPa << ", "
      << std::setprecision(4) << nu << "\n";
    f << "*Density\n" << std::scientific << std::setprecision(6) << rho_tonne_mm3 << "\n";
    f << std::fixed;
    f << "**\n";

    if (shellThickness > 0.0f) {
        f << "*Shell Section, elset=" << partName << "_SURFACE, material=" << material.name << "\n";
        f << std::fixed << std::setprecision(4) << shellThickness << ", 3\n";
        f << "**\n";
    }

    f << "*End Part\n";
    f << "**\n";
    f << "** Import: Abaqus/CAE (File > Set Work Directory, open as model)\n";
    f << "**         ANSYS (File > Import > External Model / Abaqus Input File)\n";

    f.close();
    if (f.fail()) {
        result.errorMessage = "I/O error while writing file.";
        std::cerr << "[FEMExporter] " << result.errorMessage << std::endl;
        return result;
    }

    result.success = true;
    result.nodeCount = nodes.size();
    result.elementCount = triCount;
    std::cout << "[FEMExporter] Export complete: " << result.nodeCount << " nodes, "
              << result.elementCount << " elements -> " << filePath << std::endl;
    return result;
}

} // namespace Export
