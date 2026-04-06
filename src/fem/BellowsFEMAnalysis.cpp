#include "fem/BellowsFEMAnalysis.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace Core {
namespace FEM {

void BellowsFEMAnalysis::findCuffNodes(
    const Core::Meshing::Mesh& mesh, double meshToMeters,
    std::vector<uint64_t>& cuffA,
    std::vector<uint64_t>& cuffB) const
{
    cuffA.clear();
    cuffB.clear();

    const auto& nodes = mesh.getNodes();
    if (nodes.empty()) return;

    // Find axial (x) range
    double xMin = 1e30, xMax = -1e30;
    for (const auto& n : nodes) {
        double x = n.position.x * meshToMeters;
        xMin = std::min(xMin, x);
        xMax = std::max(xMax, x);
    }

    double range = xMax - xMin;
    if (range < 1e-12) return;

    double tol = range * 0.01; // 1 % of total length

    for (const auto& n : nodes) {
        double x = n.position.x * meshToMeters;
        if (std::abs(x - xMin) < tol) cuffA.push_back(n.tag);
        if (std::abs(x - xMax) < tol) cuffB.push_back(n.tag);
    }
}

bool BellowsFEMAnalysis::run(
    const Core::Meshing::Mesh& mesh,
    double wallThicknessMM,
    const Material& material,
    const AnalysisConfig& config)
{
    if (mesh.getNodes().empty() || mesh.getElements().empty()) {
        std::cerr << "[FEM] Empty mesh – cannot run analysis.\n";
        return false;
    }

    const double meshToMeters = 0.1; // mesh coords × 0.1 = metres
    double thicknessM = wallThicknessMM * 1.0e-3;

    // ── 1. Build shell elements ──────────────────────────────────────
    solver.buildElements(mesh, material.youngsModulus, material.poissonsRatio,
                         thicknessM, meshToMeters, material.density);

    // ── 2. Identify cuff nodes ───────────────────────────────────────
    std::vector<uint64_t> cuffA, cuffB;
    findCuffNodes(mesh, meshToMeters, cuffA, cuffB);

    if (cuffA.empty()) {
        std::cerr << "[FEM] Could not identify cuff-A nodes.\n";
        return false;
    }

    // ── 3. Boundary conditions: fix all DOFs at cuff A ───────────────
    std::vector<BoundaryCondition> bcs;
    for (auto tag : cuffA) {
        for (int d = 0; d < DOFS_PER_NODE; ++d) {
            bcs.push_back({tag, static_cast<DOFType>(d), 0.0});
        }
    }

    // Optional prescribed axial displacement at cuff B
    // (future: add axialDisplacement support)

    solver.setBoundaryConditions(bcs);

    // ── 4. Solve with pressure and optional axial force ──────────────
    bool ok = solver.solve(config.pressure, config.axialForce,
                           cuffB, mesh);
    if (!ok) return false;

    // ── 5. Post-process stresses ─────────────────────────────────────
    solver.computeStresses();
    solver.computeNodeStresses(mesh);

    // ── 6. Compute reaction forces at supports ───────────────────────
    {
        // Rebuild the applied force vector to pass to reaction computation
        Eigen::VectorXd F = Eigen::VectorXd::Zero(solver.numDOFs());
        for (const auto& elem : solver.getElements()) {
            Eigen::VectorXd fe = elem.pressureLoadVector(config.pressure);
            for (int i = 0; i < 18; ++i) {
                F(elem.dofIndices[i]) += fe(i);
            }
        }
        if (std::abs(config.axialForce) > 1e-30 && !cuffB.empty()) {
            double fpn = config.axialForce / static_cast<double>(cuffB.size());
            const auto& ti = solver.getTagToIndex();
            for (auto tag : cuffB) {
                auto it = ti.find(tag);
                if (it != ti.end()) {
                    F(it->second * DOFS_PER_NODE + 0) += fpn;
                }
            }
        }
        solver.computeReactionForces(F);
    }

    std::cout << "[FEM] Analysis complete.\n"
              << "      Nodes:            " << mesh.getNodes().size() << "\n"
              << "      Elements:         " << mesh.getElements().size() << "\n"
              << "      Max displacement: " << solver.getResult().maxDisplacement * 1e3 << " mm\n"
              << "      Max von Mises:    " << solver.getResult().maxVonMises * 1e-6 << " MPa\n"
              << "      Reaction |F|:     " << solver.getResult().totalReactionMagnitude << " N\n";

    return true;
}

// ─── Modal analysis ─────────────────────────────────────────────────

bool BellowsFEMAnalysis::runModal(
    const Core::Meshing::Mesh& mesh,
    double wallThicknessMM,
    const Material& material,
    int numModes,
    ModalResult& modalResult)
{
    if (mesh.getNodes().empty() || mesh.getElements().empty()) {
        modalResult.statusMessage = "Empty mesh";
        return false;
    }

    const double meshToMeters = 0.1;
    double thicknessM = wallThicknessMM * 1.0e-3;

    solver.buildElements(mesh, material.youngsModulus, material.poissonsRatio,
                         thicknessM, meshToMeters, material.density);

    // BCs: fix cuff A
    std::vector<uint64_t> cuffA, cuffB;
    findCuffNodes(mesh, meshToMeters, cuffA, cuffB);

    std::vector<BoundaryCondition> bcs;
    for (auto tag : cuffA) {
        for (int d = 0; d < DOFS_PER_NODE; ++d) {
            bcs.push_back({tag, static_cast<DOFType>(d), 0.0});
        }
    }
    solver.setBoundaryConditions(bcs);

    return solver.solveModal(numModes, modalResult);
}

} // namespace FEM
} // namespace Core
