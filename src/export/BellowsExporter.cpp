#include "export/BellowsExporter.hpp"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <ctime>
#include <algorithm>

namespace Export {

// ─── Main export entry point ────────────────────────────────────────────────

ExportResult BellowsExporter::exportAbaqusINP(
    const Core::Meshing::Mesh& mesh,
    const Drawing::Bellows& bellows,
    const Core::FEM::Material& material,
    const std::string& filePath,
    const Core::FEM::FEMResult* femResult)
{
    ExportResult result;
    result.filePath = filePath;

    // Validate input
    if (mesh.isEmpty()) {
        result.errorMessage = "Cannot export: FEM mesh is empty. Generate a mesh first.";
        std::cerr << "[BellowsExporter] " << result.errorMessage << std::endl;
        return result;
    }

    if (mesh.getNodes().empty() || mesh.getElements().empty()) {
        result.errorMessage = "Cannot export: mesh has no nodes or elements.";
        std::cerr << "[BellowsExporter] " << result.errorMessage << std::endl;
        return result;
    }

    // Open file
    std::ofstream f(filePath);
    if (!f.is_open()) {
        result.errorMessage = "Failed to open file for writing: " + filePath;
        std::cerr << "[BellowsExporter] " << result.errorMessage << std::endl;
        return result;
    }

    std::cout << "[BellowsExporter] Exporting to: " << filePath << std::endl;

    // Write all sections
    writeHeader(f, bellows);
    f << "**\n";
    f << "*Part, name=BELLOWS\n";
    f << "**\n";

    writeNodes(f, mesh);
    f << "**\n";

    writeElements(f, mesh);
    f << "**\n";

    writeMaterial(f, material);
    f << "**\n";

    f << "** Shell section: assigns wall thickness to all bellows elements\n";
    f << "*Shell Section, elset=BELLOWS_SURFACE, material=" << material.name << "\n";
    writeShellSection(f, bellows.wallThickness);
    f << "**\n";

    writeBoundaryConditions(f, mesh, bellows);
    f << "**\n";

    writeLoadInfo(f, femResult);

    f << "*End Part\n";
    f << "**\n";
    f << "** ================================================================\n";
    f << "** ANSYS Import Instructions:\n";
    f << "** 1. Open ANSYS Mechanical or ANSYS Workbench\n";
    f << "** 2. File > Import > External Model / Abaqus Input File\n";
    f << "** 3. Select this .inp file\n";
    f << "** 4. Verify units are set to mm, tonne, s, N, MPa\n";
    f << "** 5. The mesh, material, and shell section will be imported\n";
    f << "** ================================================================\n";

    f.close();

    if (f.fail()) {
        result.errorMessage = "I/O error while writing file.";
        std::cerr << "[BellowsExporter] " << result.errorMessage << std::endl;
        return result;
    }

    result.success = true;
    result.nodeCount = mesh.getNodes().size();
    result.elementCount = mesh.getElements().size();

    std::cout << "[BellowsExporter] Export complete: "
              << result.nodeCount << " nodes, "
              << result.elementCount << " elements → "
              << filePath << std::endl;

    return result;
}

// ─── Header section ─────────────────────────────────────────────────────────

void BellowsExporter::writeHeader(std::ofstream& f, const Drawing::Bellows& b) {
    // Timestamp
    std::time_t now = std::time(nullptr);
    char timeBuf[64];
    std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

    f << "*Heading\n";
    f << "** NAVIX Bellows Export — Abaqus Input Deck\n";
    f << "** Generated: " << timeBuf << "\n";
    f << "** Software:  NAVIX CAD/FEM\n";
    f << "**\n";
    f << "** Bellows Parameters:\n";
    f << "**   Convolutions:              " << b.numConvolutions << "\n";
    f << "**   Cuff A Inner Diameter:     " << std::fixed << std::setprecision(2) << b.cuffAInnerDiameter << " mm\n";
    f << "**   Cuff B Inner Diameter:     " << b.cuffBInnerDiameter << " mm\n";
    f << "**   Cuff A Length:             " << b.cuffALength << " mm\n";
    f << "**   Cuff B Length:             " << b.cuffBLength << " mm\n";
    f << "**   Base Convolution Diameter: " << b.baseConvolutionDiameter << " mm\n";
    f << "**   Peak Convolution Diameter: " << b.peakConvolutionDiameter << " mm\n";
    f << "**   Convoluted Section Length: " << b.convolutedSectionLength << " mm\n";
    f << "**   Wall Thickness:            " << b.wallThickness << " mm\n";
    f << "**   Overall Length:            " << b.calculateOverallLength() << " mm\n";
    f << "**\n";
    f << "** Unit System: mm, tonne, s, N, MPa\n";
    f << "**   Length:   mm\n";
    f << "**   Force:    N\n";
    f << "**   Stress:   MPa\n";
    f << "**   Density:  tonne/mm^3\n";
    f << "**\n";
}

// ─── Nodes section ──────────────────────────────────────────────────────────

void BellowsExporter::writeNodes(std::ofstream& f, const Core::Meshing::Mesh& mesh) {
    const auto& nodes = mesh.getNodes();

    f << "** " << nodes.size() << " nodes\n";
    f << "*Node\n";

    // FEM mesh coordinates are in "scene units" = raw_mm / 100.
    // To recover mm: multiply by 100.
    constexpr double sceneToMM = 100.0;

    f << std::fixed << std::setprecision(6);
    for (const auto& node : nodes) {
        f << std::setw(8) << node.tag << ", "
          << std::setw(14) << (node.position.x * sceneToMM) << ", "
          << std::setw(14) << (node.position.y * sceneToMM) << ", "
          << std::setw(14) << (node.position.z * sceneToMM) << "\n";
    }
}

// ─── Elements section ───────────────────────────────────────────────────────

void BellowsExporter::writeElements(std::ofstream& f, const Core::Meshing::Mesh& mesh) {
    const auto& elements = mesh.getElements();

    // Count triangle elements
    size_t triCount = 0;
    for (const auto& e : elements) {
        if (e.elementType == 2 && e.nodeTags.size() == 3) ++triCount;
    }

    f << "** " << triCount << " triangular shell elements\n";
    f << "*Element, type=S3, ELSET=BELLOWS_SURFACE\n";

    for (const auto& e : elements) {
        // Only export triangle elements (type 2 in Gmsh convention)
        if (e.elementType != 2 || e.nodeTags.size() != 3) continue;

        f << std::setw(8) << e.tag << ", "
          << std::setw(8) << e.nodeTags[0] << ", "
          << std::setw(8) << e.nodeTags[1] << ", "
          << std::setw(8) << e.nodeTags[2] << "\n";
    }
}

// ─── Material section ───────────────────────────────────────────────────────

void BellowsExporter::writeMaterial(std::ofstream& f, const Core::FEM::Material& mat) {
    // Convert from SI base units (Pa, kg/m³) to mm-consistent units (MPa, tonne/mm³)
    double E_MPa = mat.youngsModulus / 1.0e6;       // Pa → MPa
    double nu = mat.poissonsRatio;                    // dimensionless
    double rho_tonne_mm3 = mat.density * 1.0e-12;   // kg/m³ → tonne/mm³
    double yield_MPa = mat.yieldStrength / 1.0e6;    // Pa → MPa

    f << "*Material, name=" << mat.name << "\n";

    f << "**   Young's Modulus: " << std::fixed << std::setprecision(1)
      << E_MPa << " MPa (" << (mat.youngsModulus / 1e9) << " GPa)\n";
    f << "**   Poisson's Ratio: " << std::setprecision(4) << nu << "\n";
    f << "**   Density: " << std::scientific << std::setprecision(4)
      << rho_tonne_mm3 << " tonne/mm^3\n";
    f << "**   Yield Strength: " << std::fixed << std::setprecision(1)
      << yield_MPa << " MPa\n";

    f << "*Elastic\n";
    f << std::fixed << std::setprecision(1) << E_MPa << ", "
      << std::setprecision(4) << nu << "\n";

    f << "*Density\n";
    f << std::scientific << std::setprecision(6) << rho_tonne_mm3 << "\n";

    // Reset to fixed
    f << std::fixed;
}

// ─── Shell section ──────────────────────────────────────────────────────────

void BellowsExporter::writeShellSection(std::ofstream& f, float wallThickness) {
    // Note: material name is not available here — handled in exportAbaqusINP
    // This writes the section thickness only; the material reference is written inline
    f << std::fixed << std::setprecision(2) << wallThickness << ",\n";
}

// ─── Boundary conditions (node sets for cuff A and cuff B) ──────────────────

void BellowsExporter::writeBoundaryConditions(
    std::ofstream& f,
    const Core::Meshing::Mesh& mesh,
    const Drawing::Bellows& bellows)
{
    const auto& nodes = mesh.getNodes();
    if (nodes.empty()) return;

    // Identify cuff A nodes (leftmost axial position) and cuff B nodes (rightmost)
    // In scene units, axial = raw.x / 100. Cuff A is at axial ≈ 0, cuff B at max axial.
    double minAxial = 1e30, maxAxial = -1e30;
    for (const auto& n : nodes) {
        minAxial = std::min(minAxial, n.position.x);
        maxAxial = std::max(maxAxial, n.position.x);
    }

    double axialRange = maxAxial - minAxial;
    double tolerance = axialRange * 0.02; // 2% of total length for cuff identification

    std::vector<uint64_t> cuffANodes, cuffBNodes;
    for (const auto& n : nodes) {
        if (std::abs(n.position.x - minAxial) < tolerance) {
            cuffANodes.push_back(n.tag);
        }
        if (std::abs(n.position.x - maxAxial) < tolerance) {
            cuffBNodes.push_back(n.tag);
        }
    }

    // Write node sets
    if (!cuffANodes.empty()) {
        f << "** Cuff A node set (" << cuffANodes.size() << " nodes at axial min)\n";
        f << "*Nset, nset=CUFF_A\n";
        for (size_t i = 0; i < cuffANodes.size(); ++i) {
            f << cuffANodes[i];
            if (i + 1 < cuffANodes.size()) f << ", ";
            // Line break every 16 node tags
            if ((i + 1) % 16 == 0 && i + 1 < cuffANodes.size()) f << "\n";
        }
        f << "\n";
    }

    if (!cuffBNodes.empty()) {
        f << "** Cuff B node set (" << cuffBNodes.size() << " nodes at axial max)\n";
        f << "*Nset, nset=CUFF_B\n";
        for (size_t i = 0; i < cuffBNodes.size(); ++i) {
            f << cuffBNodes[i];
            if (i + 1 < cuffBNodes.size()) f << ", ";
            if ((i + 1) % 16 == 0 && i + 1 < cuffBNodes.size()) f << "\n";
        }
        f << "\n";
    }

    // Write suggested boundary conditions as comments (user can uncomment in ANSYS)
    f << "**\n";
    f << "** Suggested Boundary Conditions (uncomment as needed):\n";
    f << "** Fix all DOFs at Cuff A (encastre):\n";
    f << "** *Boundary\n";
    f << "** CUFF_A, ENCASTRE\n";
    f << "**\n";
    f << "** Fix all DOFs at Cuff B:\n";
    f << "** *Boundary\n";
    f << "** CUFF_B, ENCASTRE\n";
    f << "**\n";
}

// ─── Load / FEM result info ─────────────────────────────────────────────────

void BellowsExporter::writeLoadInfo(std::ofstream& f, const Core::FEM::FEMResult* result) {
    f << "** ── NAVIX FEM Analysis Reference ──\n";

    if (result && result->isValid) {
        f << "** A FEM analysis was run in NAVIX prior to export.\n";
        f << "** Max von Mises stress: " << std::fixed << std::setprecision(2)
          << (result->maxVonMises / 1e6) << " MPa\n";
        f << "** Max displacement: " << std::scientific << std::setprecision(4)
          << result->maxDisplacement << " m\n";
        f << "**   (" << std::fixed << std::setprecision(4)
          << (result->maxDisplacement * 1e3) << " mm)\n";
        f << "** Status: " << result->statusMessage << "\n";

        if (!result->reactionForces.empty()) {
            f << "** Total reaction magnitude: " << std::setprecision(2)
              << result->totalReactionMagnitude << " N\n";
        }
    } else {
        f << "** No FEM analysis results available at time of export.\n";
        f << "** Run analysis in ANSYS after import.\n";
    }

    f << std::fixed; // reset
    f << "**\n";
}


} // namespace Export
