#pragma once

#include "fem/FEMTypes.hpp"
#include "fem/FEASolver.hpp"
#include "meshing/Mesh.hpp"
#include "fem/Material.hpp"

namespace Core {
namespace FEM {

/**
 * High-level bellows FEM structural analysis.
 * Sets up boundary conditions and loads specific to bellows geometry:
 * - Hollow thin-walled shell under internal pressure
 * - Fixed at cuff A, optionally loaded at cuff B
 */
class BellowsFEMAnalysis {
public:
    // Run a complete FEM analysis on the bellows mesh
    // wallThickness in mm, pressure in Pa, axialForce in N
    bool run(const Core::Meshing::Mesh& mesh,
             double wallThicknessMM,
             const Material& material,
             const AnalysisConfig& config);

    // Run modal analysis (natural frequencies + mode shapes)
    bool runModal(const Core::Meshing::Mesh& mesh,
                  double wallThicknessMM,
                  const Material& material,
                  int numModes,
                  ModalResult& modalResult);

    const FEMResult& getResult() const { return solver.getResult(); }
    const FEASolver& getSolver() const { return solver; }

private:
    FEASolver solver;

    // Identify nodes at axial extremes (cuff A = min-x, cuff B = max-x)
    void findCuffNodes(const Core::Meshing::Mesh& mesh, double meshToMeters,
                       std::vector<uint64_t>& cuffA,
                       std::vector<uint64_t>& cuffB) const;
};

} // namespace FEM
} // namespace Core
