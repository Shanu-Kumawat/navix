#pragma once

#include <string>
#include <cstddef>
#include "meshing/Mesh.hpp"
#include "shapes/ComplexShapes.hpp"
#include "fem/Material.hpp"
#include "fem/FEMTypes.hpp"

namespace Export {

/**
 * Result returned after an export attempt.
 */
struct ExportResult {
    bool success = false;
    std::string filePath;
    std::string errorMessage;
    size_t nodeCount = 0;
    size_t elementCount = 0;
};

/**
 * Exports a NAVIX bellows FEM mesh to Abaqus Input Deck (.inp) format.
 *
 * The exported file can be imported directly into:
 *   - ANSYS Mechanical (File > Import > Abaqus Input File)
 *   - Abaqus/CAE
 *   - Any solver that reads INP files
 *
 * Coordinate unit: millimetres (mm) — matching the NAVIX design parameters.
 * Material properties are exported in consistent units (MPa, tonne/mm³).
 *
 * Element type: S3 (3-node triangular shell), matching the NAVIX FEM mesh.
 */
class BellowsExporter {
public:
    /**
     * Export the bellows FEM mesh to an Abaqus INP file.
     *
     * @param mesh         The FEM mesh (nodes + triangle elements) from BellowsModel3D
     * @param bellows      The parametric bellows shape (provides wall thickness & dimensions)
     * @param material     FEM material (Young's modulus, Poisson's ratio, density)
     * @param filePath     Output file path (e.g. "/path/to/project/bellows_export.inp")
     * @param femResult    Optional FEM result — if valid, boundary conditions and loads are
     *                     written as comments for reference
     * @return ExportResult with success flag, stats, and error message if failed
     */
    static ExportResult exportAbaqusINP(
        const Core::Meshing::Mesh& mesh,
        const Drawing::Bellows& bellows,
        const Core::FEM::Material& material,
        const std::string& filePath,
        const Core::FEM::FEMResult* femResult = nullptr
    );

private:
    static void writeHeader(std::ofstream& f, const Drawing::Bellows& b);
    static void writeNodes(std::ofstream& f, const Core::Meshing::Mesh& mesh);
    static void writeElements(std::ofstream& f, const Core::Meshing::Mesh& mesh);
    static void writeMaterial(std::ofstream& f, const Core::FEM::Material& mat);
    static void writeShellSection(std::ofstream& f, float wallThickness);
    static void writeBoundaryConditions(std::ofstream& f,
                                        const Core::Meshing::Mesh& mesh,
                                        const Drawing::Bellows& bellows);
    static void writeLoadInfo(std::ofstream& f, const Core::FEM::FEMResult* result);
};

} // namespace Export
