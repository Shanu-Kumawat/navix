#pragma once

#include <Eigen/Dense>
#include <array>

namespace Core {
namespace FEM {

/**
 * Flat triangular shell element: CST membrane + DKT bending.
 * 3 nodes, 6 DOFs per node (u, v, w, θx, θy, θz) = 18 DOFs total.
 *
 * CST: Constant Strain Triangle for in-plane (membrane) behavior.
 * DKT: Discrete Kirchhoff Triangle for out-of-plane (bending) behavior.
 * Reference: Batoz, Bathe, Ho (1980) IJNME Vol.15.
 */
class ShellElement {
public:
    // Node positions in global 3D coordinates
    std::array<Eigen::Vector3d, 3> nodes;
    // Global DOF indices for this element's 18 DOFs
    std::array<int, 18> dofIndices;

    double thickness;      // shell thickness (meters)
    double youngsModulus;   // E (Pa)
    double poissonsRatio;   // ν
    double density = 7850;  // ρ (kg/m³)

    // Compute element stiffness matrix in global coordinates (18×18)
    Eigen::MatrixXd stiffnessMatrix() const;

    // Compute consistent mass matrix in global coordinates (18×18)
    // Lumped diagonal mass for translational DOFs, scaled rotational inertia
    Eigen::MatrixXd massMatrix() const;

    // Compute consistent pressure load vector in global coordinates (18×1)
    // pressure > 0 pushes in the outward normal direction
    Eigen::VectorXd pressureLoadVector(double pressure) const;

    // Compute element stresses from the element's 18 global displacements
    struct StressResult {
        Eigen::Vector3d membrane;     // [σxx, σyy, σxy]
        Eigen::Vector3d bendingTop;   // at +t/2
        Eigen::Vector3d bendingBot;   // at -t/2
        double vmTop, vmBot;
    };
    StressResult computeStress(const Eigen::VectorXd& elemDisp) const;

    // Element area
    double area() const;

    // Element normal (unit vector)
    Eigen::Vector3d normal() const;

private:
    // Local coordinate system for the element
    struct LocalCS {
        Eigen::Vector3d e1, e2, e3;  // local x, y, z axes
        Eigen::Matrix3d T;           // rotation matrix: v_local = T * v_global
    };
    LocalCS localCS() const;

    // Local 2D coordinates of nodes projected into element plane
    struct Local2D {
        double x1, y1, x2, y2, x3, y3;
    };
    Local2D localCoords(const LocalCS& cs) const;

    // Material constitutive matrices
    Eigen::Matrix3d Dm() const;  // membrane: E*t/(1-ν²) * [...]
    Eigen::Matrix3d Db() const;  // bending:  E*t³/(12(1-ν²)) * [...]

    // CST membrane B-matrix (3×6), constant over element
    Eigen::MatrixXd membraneBMatrix(const Local2D& lc, double A) const;

    // CST membrane stiffness (6×6) in local (u,v) DOFs
    Eigen::MatrixXd membraneK(const Local2D& lc) const;

    // DKT bending B-matrix (3×9) at area coordinates (L1,L2,L3)
    Eigen::MatrixXd bendingBMatrix(double L1, double L2, double L3,
                                   const Local2D& lc) const;

    // DKT bending stiffness (9×9) in local (w,βx,βy) DOFs
    Eigen::MatrixXd bendingK_beta(const Local2D& lc) const;

    // Transform DKT stiffness from (w,βx,βy) to (w,θx,θy) convention
    // βx = -θy, βy = θx
    Eigen::MatrixXd bendingK_theta(const Local2D& lc) const;

    // Assemble local 18×18 shell stiffness from membrane (6×6) and bending (9×9)
    Eigen::MatrixXd assembleLocal(const Eigen::MatrixXd& Km,
                                  const Eigen::MatrixXd& Kb) const;

    // Build 18×18 global-to-local transformation matrix
    Eigen::MatrixXd transformationMatrix(const LocalCS& cs) const;

    // Von Mises from plane stress
    static double vonMises(const Eigen::Vector3d& s);
};

} // namespace FEM
} // namespace Core
