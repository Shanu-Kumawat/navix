#pragma once

#include "fem/FEMTypes.hpp"
#include "fem/ShellElement.hpp"
#include "meshing/Mesh.hpp"
#include <vector>

namespace Core {
namespace FEM {

/**
 * FEA Solver for shell structures.
 * Assembles global stiffness matrix from shell elements,
 * applies boundary conditions, solves, and computes stresses.
 */
class FEASolver {
public:
    // Build shell elements from a triangular surface mesh
    // meshToMeters: scale factor from mesh coordinates to meters
    void buildElements(const Core::Meshing::Mesh& mesh,
                       double youngsModulus,
                       double poissonsRatio,
                       double thickness,
                       double meshToMeters,
                       double density = 7850.0);

    // Apply boundary conditions (fixed DOFs)
    void setBoundaryConditions(const std::vector<BoundaryCondition>& bcs);

    // Assemble and solve with an explicit global force vector
    bool solve(const Eigen::VectorXd& globalForce);

    // Assemble and solve with pressure + optional axial force on cuff B
    bool solve(double pressure, double axialForce,
              const std::vector<uint64_t>& cuffBNodes,
              const Core::Meshing::Mesh& mesh);

    // Compute reaction forces at constrained DOFs: R = K*u - F
    void computeReactionForces(const Eigen::VectorXd& appliedForce);

    // Modal analysis: solve generalized eigenvalue problem K*φ = ω²*M*φ
    // Returns the first numModes natural frequencies and mode shapes
    bool solveModal(int numModes, ModalResult& modalResult);

    // Compute element stresses from solved displacements
    void computeStresses();

    // Compute per-node von Mises stress (averaged from adjacent elements)
    void computeNodeStresses(const Core::Meshing::Mesh& mesh);

    // Get results
    FEMResult& getResult() { return result; }
    const FEMResult& getResult() const { return result; }
    int numDOFs() const { return nDOFs; }
    int numNodes() const { return nNodes; }
    const std::vector<ShellElement>& getElements() const { return elements; }
    const std::unordered_map<uint64_t, int>& getTagToIndex() const { return tagToIndex; }

private:
    std::vector<ShellElement> elements;
    std::vector<BoundaryCondition> bcs;
    FEMResult result;
    int nNodes = 0;
    int nDOFs = 0;

    // Tag-to-index mapping for mesh nodes
    std::unordered_map<uint64_t, int> tagToIndex;
};

} // namespace FEM
} // namespace Core
