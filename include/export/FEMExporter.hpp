#pragma once

// Generic Abaqus INP writer for any Core::Meshing::Mesh (all NAVIX models).
// Reads like BellowsExporter's deck but model-agnostic: part name, optional
// shell thickness, material — no model-specific boundary conditions.
// The .inp deck is importable by Abaqus/CAE and ANSYS (File > Import >
// External Model / Abaqus Input File).

#include <string>
#include "export/BellowsExporter.hpp"   // ExportResult
#include "meshing/Mesh.hpp"
#include "fem/Material.hpp"

namespace Export {

class FEMExporter {
public:
    // Shell thickness <= 0 skips the *Shell Section block (mesh-only export).
    // sceneToMM converts mesh coordinates ("scene units") to millimetres:
    // bellows/shock absorber = 100, ball bearing = outer radius (mm).
    static ExportResult exportAbaqusINP(const Core::Meshing::Mesh& mesh,
                                        const std::string& partName,
                                        const Core::FEM::Material& material,
                                        const std::string& filePath,
                                        float shellThickness = -1.0f,
                                        double sceneToMM = 100.0);
};

} // namespace Export
