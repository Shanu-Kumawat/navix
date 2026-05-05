#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <Eigen/Dense>
#include <Eigen/Sparse>

namespace Core {
namespace FEM {

constexpr int DOFS_PER_NODE = 6;  // u, v, w, θx, θy, θz

enum class DOFType { UX = 0, UY = 1, UZ = 2, RX = 3, RY = 4, RZ = 5 };

struct BoundaryCondition {
    uint64_t nodeTag;
    DOFType dof;
    double value;  // prescribed displacement/rotation (0 for fixed)
};

struct NodalLoad {
    uint64_t nodeTag;
    Eigen::Vector3d force;   // Fx, Fy, Fz
    Eigen::Vector3d moment;  // Mx, My, Mz
};

// Per-element stress result in element local coordinates
struct ElementStress {
    uint64_t elementTag;
    Eigen::Vector3d membraneStress;     // [σxx, σyy, σxy] membrane
    Eigen::Vector3d bendingStressTop;   // [σxx, σyy, σxy] at +t/2
    Eigen::Vector3d bendingStressBot;   // [σxx, σyy, σxy] at -t/2
    double vonMisesTop;
    double vonMisesBot;
    double vonMisesMax;  // max(top, bot)
};

// Reaction force at a constrained node
struct ReactionForce {
    uint64_t nodeTag;
    Eigen::Vector3d force;   // Fx, Fy, Fz
    Eigen::Vector3d moment;  // Mx, My, Mz
    double magnitude;        // ||force||
};

// Single mode from modal analysis
struct ModeShape {
    int modeNumber;
    double frequency;        // Hz
    double omega;            // rad/s
    Eigen::VectorXd modeVector; // normalized mode shape DOF vector
};

// Modal analysis result
struct ModalResult {
    std::vector<ModeShape> modes;
    bool isValid = false;
    std::string statusMessage;
};

// Mesh convergence study data point
struct ConvergencePoint {
    float elementSize;
    int numNodes;
    int numElements;
    double maxStress;        // MPa
    double maxDisplacement;  // mm
};

struct FEMResult {
    Eigen::VectorXd displacements;             // full DOF vector
    std::vector<ElementStress> elementStresses;
    std::vector<double> nodeVonMises;          // per-node averaged von Mises
    double maxVonMises = 0;
    double minVonMises = 0;
    double maxDisplacement = 0;
    bool isValid = false;
    std::string statusMessage;

    // Reaction forces at constrained nodes
    std::vector<ReactionForce> reactionForces;
    double totalReactionMagnitude = 0;
};

struct AnalysisConfig {
    double pressure = 1e5;          // Internal pressure (Pa)
    double axialForce = 0;          // Axial force on free end (N)
    bool fixCuffA = true;           // Fix all DOFs at cuff A
    bool fixCuffB = false;          // Fix all DOFs at cuff B
    uint32_t materialId = 0;        // Material ID
    double meshToMeters = 0.1;      // Scale: mesh_coord × this = meters
};

// Compute von Mises stress from plane stress components [σxx, σyy, σxy]
inline double computeVonMises(const Eigen::Vector3d& s) {
    return std::sqrt(s(0)*s(0) + s(1)*s(1) - s(0)*s(1) + 3.0*s(2)*s(2));
}

} // namespace FEM
} // namespace Core
