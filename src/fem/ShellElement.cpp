#include "fem/ShellElement.hpp"
#include <cmath>
#include <iostream>

namespace Core {
namespace FEM {

// ============================================================
//  Geometric helpers
// ============================================================

double ShellElement::area() const {
    Eigen::Vector3d v1 = nodes[1] - nodes[0];
    Eigen::Vector3d v2 = nodes[2] - nodes[0];
    return 0.5 * v1.cross(v2).norm();
}

Eigen::Vector3d ShellElement::normal() const {
    Eigen::Vector3d v1 = nodes[1] - nodes[0];
    Eigen::Vector3d v2 = nodes[2] - nodes[0];
    Eigen::Vector3d n = v1.cross(v2);
    double len = n.norm();
    if (len < 1e-30) return Eigen::Vector3d(0, 0, 1);
    return n / len;
}

ShellElement::LocalCS ShellElement::localCS() const {
    LocalCS cs;
    Eigen::Vector3d v1 = nodes[1] - nodes[0];
    cs.e1 = v1.normalized();
    Eigen::Vector3d v2 = nodes[2] - nodes[0];
    cs.e3 = v1.cross(v2);
    double len = cs.e3.norm();
    if (len < 1e-30) cs.e3 = Eigen::Vector3d(0, 0, 1);
    else cs.e3 /= len;
    cs.e2 = cs.e3.cross(cs.e1);
    // Rotation matrix: local = T * global
    cs.T.row(0) = cs.e1.transpose();
    cs.T.row(1) = cs.e2.transpose();
    cs.T.row(2) = cs.e3.transpose();
    return cs;
}

ShellElement::Local2D ShellElement::localCoords(const LocalCS& cs) const {
    Local2D lc;
    // Transform nodes to local frame, take x,y components (z ≈ 0)
    Eigen::Vector3d p0 = cs.T * nodes[0];
    Eigen::Vector3d p1 = cs.T * nodes[1];
    Eigen::Vector3d p2 = cs.T * nodes[2];
    // Translate so node 0 is at origin
    lc.x1 = 0; lc.y1 = 0;
    lc.x2 = p1(0) - p0(0);  lc.y2 = p1(1) - p0(1);
    lc.x3 = p2(0) - p0(0);  lc.y3 = p2(1) - p0(1);
    return lc;
}

// ============================================================
//  Constitutive matrices
// ============================================================

Eigen::Matrix3d ShellElement::Dm() const {
    double E = youngsModulus, nu = poissonsRatio, t = thickness;
    double c = E * t / (1.0 - nu * nu);
    Eigen::Matrix3d D;
    D << c,     c*nu,  0,
         c*nu,  c,     0,
         0,     0,     c*(1.0-nu)/2.0;
    return D;
}

Eigen::Matrix3d ShellElement::Db() const {
    double E = youngsModulus, nu = poissonsRatio, t = thickness;
    double c = E * t * t * t / (12.0 * (1.0 - nu * nu));
    Eigen::Matrix3d D;
    D << c,     c*nu,  0,
         c*nu,  c,     0,
         0,     0,     c*(1.0-nu)/2.0;
    return D;
}

// ============================================================
//  CST Membrane
// ============================================================

Eigen::MatrixXd ShellElement::membraneBMatrix(const Local2D& lc, double A) const {
    double x1 = lc.x1, y1 = lc.y1;
    double x2 = lc.x2, y2 = lc.y2;
    double x3 = lc.x3, y3 = lc.y3;

    double y23 = y2 - y3, y31 = y3 - y1, y12 = y1 - y2;
    double x32 = x3 - x2, x13 = x1 - x3, x21 = x2 - x1;
    double invA2 = 1.0 / (2.0 * A);

    Eigen::MatrixXd B(3, 6);
    B << y23*invA2, 0,         y31*invA2, 0,         y12*invA2, 0,
         0,         x32*invA2, 0,         x13*invA2, 0,         x21*invA2,
         x32*invA2, y23*invA2, x13*invA2, y31*invA2, x21*invA2, y12*invA2;
    return B;
}

Eigen::MatrixXd ShellElement::membraneK(const Local2D& lc) const {
    double x1 = lc.x1, y1 = lc.y1;
    double x2 = lc.x2, y2 = lc.y2;
    double x3 = lc.x3, y3 = lc.y3;
    double A = 0.5 * std::abs(x1*(y2-y3) + x2*(y3-y1) + x3*(y1-y2));
    if (A < 1e-30) return Eigen::MatrixXd::Zero(6, 6);

    Eigen::MatrixXd Bm = membraneBMatrix(lc, A);
    Eigen::Matrix3d D = Dm();
    // K_m = A * Bm^T * D * Bm  (note: D already includes thickness)
    return A * Bm.transpose() * D * Bm;
}

// ============================================================
//  DKT Bending
// ============================================================

Eigen::MatrixXd ShellElement::bendingBMatrix(double L1, double L2, double L3,
                                             const Local2D& lc) const {
    double x1 = lc.x1, y1 = lc.y1;
    double x2 = lc.x2, y2 = lc.y2;
    double x3 = lc.x3, y3 = lc.y3;

    // Edge vectors
    double x23 = x2 - x3, y23 = y2 - y3;
    double x31 = x3 - x1, y31 = y3 - y1;
    double x12 = x1 - x2, y12 = y1 - y2;

    // Edge lengths squared
    double l4sq = x23*x23 + y23*y23;
    double l5sq = x31*x31 + y31*y31;
    double l6sq = x12*x12 + y12*y12;

    // Guard against degenerate edges
    if (l4sq < 1e-30 || l5sq < 1e-30 || l6sq < 1e-30)
        return Eigen::MatrixXd::Zero(3, 9);

    // DKT coefficients (Batoz 1980, eqs. 8a-8c)
    double a4 = -x23/l4sq, d4 = -y23/l4sq;
    double a5 = -x31/l5sq, d5 = -y31/l5sq;
    double a6 = -x12/l6sq, d6 = -y12/l6sq;

    double b4 = 0.75*x23*y23/l4sq;
    double b5 = 0.75*x31*y31/l5sq;
    double b6 = 0.75*x12*y12/l6sq;

    double c4 = (0.25*x23*x23 - 0.5*y23*y23)/l4sq;
    double c5 = (0.25*x31*x31 - 0.5*y31*y31)/l5sq;
    double c6 = (0.25*x12*x12 - 0.5*y12*y12)/l6sq;

    double e4 = (0.25*y23*y23 - 0.5*x23*x23)/l4sq;
    double e5 = (0.25*y31*y31 - 0.5*x31*x31)/l5sq;
    double e6 = (0.25*y12*y12 - 0.5*x12*x12)/l6sq;

    // Quadratic mid-side shape functions
    double P4 = 4.0*L2*L3;
    double P5 = 4.0*L3*L1;
    double P6 = 4.0*L1*L2;

    // Partial derivatives of P4,P5,P6 w.r.t. L1,L2,L3
    // (treating L1,L2,L3 as independent area coordinates)
    double dP4_dL1 = 0.0,    dP4_dL2 = 4.0*L3, dP4_dL3 = 4.0*L2;
    double dP5_dL1 = 4.0*L3, dP5_dL2 = 0.0,    dP5_dL3 = 4.0*L1;
    double dP6_dL1 = 4.0*L2, dP6_dL2 = 4.0*L1, dP6_dL3 = 0.0;

    // Area of local triangle
    double A = 0.5 * std::abs(x1*(y2-y3) + x2*(y3-y1) + x3*(y1-y2));
    double A2 = 2.0 * A;
    if (A < 1e-30) return Eigen::MatrixXd::Zero(3, 9);

    // Derivatives of area coordinates w.r.t. x and y
    double x32 = x3 - x2, x13 = x1 - x3, x21 = x2 - x1;
    double dL1_dx = y23/A2, dL2_dx = y31/A2, dL3_dx = y12/A2;
    double dL1_dy = x32/A2, dL2_dy = x13/A2, dL3_dy = x21/A2;

    // Helper: ∂P/∂x = ∂P/∂L1 * dL1/dx + ∂P/∂L2 * dL2/dx + ∂P/∂L3 * dL3/dx
    double dP4_dx = dP4_dL1*dL1_dx + dP4_dL2*dL2_dx + dP4_dL3*dL3_dx;
    double dP4_dy = dP4_dL1*dL1_dy + dP4_dL2*dL2_dy + dP4_dL3*dL3_dy;
    double dP5_dx = dP5_dL1*dL1_dx + dP5_dL2*dL2_dx + dP5_dL3*dL3_dx;
    double dP5_dy = dP5_dL1*dL1_dy + dP5_dL2*dL2_dy + dP5_dL3*dL3_dy;
    double dP6_dx = dP6_dL1*dL1_dx + dP6_dL2*dL2_dx + dP6_dL3*dL3_dx;
    double dP6_dy = dP6_dL1*dL1_dy + dP6_dL2*dL2_dy + dP6_dL3*dL3_dy;

    double dN1_dx = dL1_dx, dN1_dy = dL1_dy;
    double dN2_dx = dL2_dx, dN2_dy = dL2_dy;
    double dN3_dx = dL3_dx, dN3_dy = dL3_dy;

    // ∂Hx_j/∂x and ∂Hx_j/∂y for j=0..8 (DOF order: w1,βx1,βy1, w2,βx2,βy2, w3,βx3,βy3)
    // Node 1 (adjacent edges: prev=5, next=6)
    double dHx0_dx = 1.5*(a6*dP6_dx - a5*dP5_dx);
    double dHx0_dy = 1.5*(a6*dP6_dy - a5*dP5_dy);
    double dHx1_dx = b5*dP5_dx + b6*dP6_dx;
    double dHx1_dy = b5*dP5_dy + b6*dP6_dy;
    double dHx2_dx = dN1_dx - c5*dP5_dx - c6*dP6_dx;
    double dHx2_dy = dN1_dy - c5*dP5_dy - c6*dP6_dy;

    // Node 2 (adjacent edges: prev=6, next=4)
    double dHx3_dx = 1.5*(a4*dP4_dx - a6*dP6_dx);
    double dHx3_dy = 1.5*(a4*dP4_dy - a6*dP6_dy);
    double dHx4_dx = b6*dP6_dx + b4*dP4_dx;
    double dHx4_dy = b6*dP6_dy + b4*dP4_dy;
    double dHx5_dx = dN2_dx - c6*dP6_dx - c4*dP4_dx;
    double dHx5_dy = dN2_dy - c6*dP6_dy - c4*dP4_dy;

    // Node 3 (adjacent edges: prev=4, next=5)
    double dHx6_dx = 1.5*(a5*dP5_dx - a4*dP4_dx);
    double dHx6_dy = 1.5*(a5*dP5_dy - a4*dP4_dy);
    double dHx7_dx = b4*dP4_dx + b5*dP5_dx;
    double dHx7_dy = b4*dP4_dy + b5*dP5_dy;
    double dHx8_dx = dN3_dx - c4*dP4_dx - c5*dP5_dx;
    double dHx8_dy = dN3_dy - c4*dP4_dy - c5*dP5_dy;

    // ∂Hy_j/∂x and ∂Hy_j/∂y
    // Node 1
    double dHy0_dx = 1.5*(d6*dP6_dx - d5*dP5_dx);
    double dHy0_dy = 1.5*(d6*dP6_dy - d5*dP5_dy);
    double dHy1_dx = -dN1_dx + e5*dP5_dx + e6*dP6_dx;
    double dHy1_dy = -dN1_dy + e5*dP5_dy + e6*dP6_dy;
    double dHy2_dx = -b5*dP5_dx - b6*dP6_dx;
    double dHy2_dy = -b5*dP5_dy - b6*dP6_dy;

    // Node 2
    double dHy3_dx = 1.5*(d4*dP4_dx - d6*dP6_dx);
    double dHy3_dy = 1.5*(d4*dP4_dy - d6*dP6_dy);
    double dHy4_dx = -dN2_dx + e6*dP6_dx + e4*dP4_dx;
    double dHy4_dy = -dN2_dy + e6*dP6_dy + e4*dP4_dy;
    double dHy5_dx = -b6*dP6_dx - b4*dP4_dx;
    double dHy5_dy = -b6*dP6_dy - b4*dP4_dy;

    // Node 3
    double dHy6_dx = 1.5*(d5*dP5_dx - d4*dP4_dx);
    double dHy6_dy = 1.5*(d5*dP5_dy - d4*dP4_dy);
    double dHy7_dx = -dN3_dx + e4*dP4_dx + e5*dP5_dx;
    double dHy7_dy = -dN3_dy + e4*dP4_dy + e5*dP5_dy;
    double dHy8_dx = -b4*dP4_dx - b5*dP5_dx;
    double dHy8_dy = -b4*dP4_dy - b5*dP5_dy;

    // Assemble B_b (3×9): κ = {∂βx/∂x, ∂βy/∂y, ∂βx/∂y + ∂βy/∂x}
    Eigen::MatrixXd B(3, 9);
    // Row 0: ∂βx/∂x = ∂Hx/∂x
    B(0,0) = dHx0_dx; B(0,1) = dHx1_dx; B(0,2) = dHx2_dx;
    B(0,3) = dHx3_dx; B(0,4) = dHx4_dx; B(0,5) = dHx5_dx;
    B(0,6) = dHx6_dx; B(0,7) = dHx7_dx; B(0,8) = dHx8_dx;
    // Row 1: ∂βy/∂y = ∂Hy/∂y
    B(1,0) = dHy0_dy; B(1,1) = dHy1_dy; B(1,2) = dHy2_dy;
    B(1,3) = dHy3_dy; B(1,4) = dHy4_dy; B(1,5) = dHy5_dy;
    B(1,6) = dHy6_dy; B(1,7) = dHy7_dy; B(1,8) = dHy8_dy;
    // Row 2: ∂βx/∂y + ∂βy/∂x
    B(2,0) = dHx0_dy + dHy0_dx; B(2,1) = dHx1_dy + dHy1_dx; B(2,2) = dHx2_dy + dHy2_dx;
    B(2,3) = dHx3_dy + dHy3_dx; B(2,4) = dHx4_dy + dHy4_dx; B(2,5) = dHx5_dy + dHy5_dx;
    B(2,6) = dHx6_dy + dHy6_dx; B(2,7) = dHx7_dy + dHy7_dx; B(2,8) = dHx8_dy + dHy8_dx;

    return B;
}

Eigen::MatrixXd ShellElement::bendingK_beta(const Local2D& lc) const {
    double x1 = lc.x1, y1 = lc.y1;
    double x2 = lc.x2, y2 = lc.y2;
    double x3 = lc.x3, y3 = lc.y3;
    double A = 0.5 * std::abs(x1*(y2-y3) + x2*(y3-y1) + x3*(y1-y2));
    if (A < 1e-30) return Eigen::MatrixXd::Zero(9, 9);

    Eigen::Matrix3d D = Db();
    Eigen::MatrixXd Kb = Eigen::MatrixXd::Zero(9, 9);

    // 3-point Gauss quadrature on triangle
    // Points: (1/6,1/6,2/3), (2/3,1/6,1/6), (1/6,2/3,1/6), weights: 1/3 each
    static const double gp[3][3] = {
        {1.0/6.0, 1.0/6.0, 2.0/3.0},
        {2.0/3.0, 1.0/6.0, 1.0/6.0},
        {1.0/6.0, 2.0/3.0, 1.0/6.0}
    };
    static const double gw = 1.0/3.0;

    for (int ig = 0; ig < 3; ++ig) {
        Eigen::MatrixXd Bb = bendingBMatrix(gp[ig][0], gp[ig][1], gp[ig][2], lc);
        Kb += gw * Bb.transpose() * D * Bb;
    }
    Kb *= A;
    return Kb;
}

Eigen::MatrixXd ShellElement::bendingK_theta(const Local2D& lc) const {
    Eigen::MatrixXd Kb = bendingK_beta(lc);

    // Transform from (w, βx, βy) to (w, θx, θy) DOFs per node
    // Relationship: βx = -θy, βy = θx
    // G_node = [1  0  0; 0  0  -1; 0  1  0]
    Eigen::MatrixXd G = Eigen::MatrixXd::Zero(9, 9);
    for (int n = 0; n < 3; ++n) {
        int i = n * 3;
        G(i,   i)   =  1.0;  // w → w
        G(i+1, i+2) = -1.0;  // βx = -θy
        G(i+2, i+1) =  1.0;  // βy = θx
    }
    return G.transpose() * Kb * G;
}

// ============================================================
//  Assemble local shell matrix
// ============================================================

Eigen::MatrixXd ShellElement::assembleLocal(const Eigen::MatrixXd& Km,
                                            const Eigen::MatrixXd& Kb) const {
    Eigen::MatrixXd K = Eigen::MatrixXd::Zero(18, 18);

    // Scatter membrane (6×6) into DOF positions [u,v] = indices [0,1] per node
    int mDofs[6] = {0, 1, 6, 7, 12, 13};
    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 6; ++j)
            K(mDofs[i], mDofs[j]) += Km(i, j);

    // Scatter bending (9×9) into DOF positions [w,θx,θy] = indices [2,3,4] per node
    int bDofs[9] = {2, 3, 4, 8, 9, 10, 14, 15, 16};
    for (int i = 0; i < 9; ++i)
        for (int j = 0; j < 9; ++j)
            K(bDofs[i], bDofs[j]) += Kb(i, j);

    // Drilling DOF (θz) penalty: small stiffness to prevent singularity
    // Use α × max diagonal entry of K
    double maxDiag = 0;
    for (int i = 0; i < 18; ++i)
        maxDiag = std::max(maxDiag, std::abs(K(i, i)));
    double alpha = 1e-3 * maxDiag;
    if (alpha < 1e-20) alpha = 1e-6;

    K(5, 5)   += alpha;  // θz node 1
    K(11, 11) += alpha;  // θz node 2
    K(17, 17) += alpha;  // θz node 3

    return K;
}

// ============================================================
//  Coordinate transformation
// ============================================================

Eigen::MatrixXd ShellElement::transformationMatrix(const LocalCS& cs) const {
    Eigen::MatrixXd T = Eigen::MatrixXd::Zero(18, 18);
    for (int n = 0; n < 3; ++n) {
        int base = n * 6;
        // Translation DOFs: v_local = R * v_global
        T.block<3,3>(base,   base)   = cs.T;
        // Rotation DOFs: same rotation
        T.block<3,3>(base+3, base+3) = cs.T;
    }
    return T;
}

// ============================================================
//  Public: Full stiffness matrix in global coordinates
// ============================================================

Eigen::MatrixXd ShellElement::stiffnessMatrix() const {
    LocalCS cs = localCS();
    Local2D lc = localCoords(cs);

    Eigen::MatrixXd Km = membraneK(lc);
    Eigen::MatrixXd Kb = bendingK_theta(lc);
    Eigen::MatrixXd Klocal = assembleLocal(Km, Kb);

    Eigen::MatrixXd T = transformationMatrix(cs);
    // K_global = T^T * K_local * T
    return T.transpose() * Klocal * T;
}

// ============================================================
//  Pressure load vector
// ============================================================

Eigen::VectorXd ShellElement::pressureLoadVector(double pressure) const {
    double A = area();
    if (A < 1e-30) return Eigen::VectorXd::Zero(18);

    // Pressure acts normal to element surface
    Eigen::Vector3d n = normal();

    // Ensure pressure pushes outward (away from revolution axis = X-axis)
    // Centroid of element
    Eigen::Vector3d centroid = (nodes[0] + nodes[1] + nodes[2]) / 3.0;
    // Radial direction (perpendicular to X-axis)
    Eigen::Vector3d radial(0, centroid(1), centroid(2));
    double radLen = radial.norm();
    if (radLen > 1e-10) {
        radial /= radLen;
        if (n.dot(radial) < 0) n = -n;  // flip to point outward
    }

    // Consistent nodal force: f_i = (p * A / 3) * n for each node
    Eigen::Vector3d fi = (pressure * A / 3.0) * n;

    // Build global load vector (18×1)
    Eigen::VectorXd f = Eigen::VectorXd::Zero(18);
    for (int i = 0; i < 3; ++i) {
        f(i*6 + 0) = fi(0);  // Fx
        f(i*6 + 1) = fi(1);  // Fy
        f(i*6 + 2) = fi(2);  // Fz
        // No moments from pressure
    }
    return f;
}

// ============================================================
//  Stress computation
// ============================================================

double ShellElement::vonMises(const Eigen::Vector3d& s) {
    return std::sqrt(s(0)*s(0) + s(1)*s(1) - s(0)*s(1) + 3.0*s(2)*s(2));
}

ShellElement::StressResult ShellElement::computeStress(const Eigen::VectorXd& elemDisp) const {
    StressResult result;
    result.membrane = Eigen::Vector3d::Zero();
    result.bendingTop = Eigen::Vector3d::Zero();
    result.bendingBot = Eigen::Vector3d::Zero();
    result.vmTop = result.vmBot = 0;

    LocalCS cs = localCS();
    Local2D lc = localCoords(cs);
    double A = 0.5 * std::abs(lc.x1*(lc.y2-lc.y3) + lc.x2*(lc.y3-lc.y1) + lc.x3*(lc.y1-lc.y2));
    if (A < 1e-30) return result;

    // Transform displacements from global to local
    Eigen::MatrixXd T = transformationMatrix(cs);
    Eigen::VectorXd dLocal = T * elemDisp;

    // --- Membrane stress ---
    // Extract membrane DOFs: u,v for each node
    Eigen::VectorXd dm(6);
    dm << dLocal(0), dLocal(1), dLocal(6), dLocal(7), dLocal(12), dLocal(13);
    Eigen::MatrixXd Bm = membraneBMatrix(lc, A);
    Eigen::Vector3d strain_m = Bm * dm;
    // Stress = D_m * strain / thickness (D_m includes t, so σ = D_m * ε / t)
    double t = thickness;
    Eigen::Matrix3d D_m = Dm();
    Eigen::Vector3d N = D_m * strain_m;  // force resultant (N/m)
    result.membrane = N / t;              // stress (Pa)

    // --- Bending stress ---
    // Extract bending DOFs: w, θx, θy for each node
    Eigen::VectorXd db_theta(9);
    db_theta << dLocal(2), dLocal(3), dLocal(4),
                dLocal(8), dLocal(9), dLocal(10),
                dLocal(14), dLocal(15), dLocal(16);

    // Convert to βx, βy: βx = -θy, βy = θx
    Eigen::MatrixXd G = Eigen::MatrixXd::Zero(9, 9);
    for (int n = 0; n < 3; ++n) {
        int i = n * 3;
        G(i,   i)   =  1.0;
        G(i+1, i+2) = -1.0;
        G(i+2, i+1) =  1.0;
    }
    Eigen::VectorXd db_beta = G * db_theta;

    // Evaluate curvature at element centroid (L1=L2=L3=1/3)
    Eigen::MatrixXd Bb = bendingBMatrix(1.0/3.0, 1.0/3.0, 1.0/3.0, lc);
    Eigen::Vector3d kappa = Bb * db_beta;

    // Moment resultant: M = D_b * κ
    Eigen::Matrix3d D_b = Db();
    Eigen::Vector3d M = D_b * kappa;

    // Bending stress at surfaces: σ_b = ±M * (t/2) / (t³/12) = ±6*M/t²
    Eigen::Vector3d sigma_b = 6.0 * M / (t * t);
    result.bendingTop =  sigma_b;
    result.bendingBot = -sigma_b;

    // Total stress at surfaces
    Eigen::Vector3d stressTop = result.membrane + result.bendingTop;
    Eigen::Vector3d stressBot = result.membrane + result.bendingBot;
    result.vmTop = vonMises(stressTop);
    result.vmBot = vonMises(stressBot);

    return result;
}

// ============================================================
//  Consistent lumped mass matrix
// ============================================================

Eigen::MatrixXd ShellElement::massMatrix() const {
    double A = area();
    if (A < 1e-30) return Eigen::MatrixXd::Zero(18, 18);

    // Lumped mass: total mass divided equally among 3 nodes
    double totalMass = density * thickness * A;
    double nodeMass = totalMass / 3.0;

    // Rotational inertia approximation: I = m * t²/12 per node
    double nodeRotI = nodeMass * thickness * thickness / 12.0;

    Eigen::MatrixXd M = Eigen::MatrixXd::Zero(18, 18);
    for (int n = 0; n < 3; ++n) {
        int base = n * 6;
        // Translational DOFs: u, v, w
        M(base + 0, base + 0) = nodeMass;
        M(base + 1, base + 1) = nodeMass;
        M(base + 2, base + 2) = nodeMass;
        // Rotational DOFs: θx, θy, θz
        M(base + 3, base + 3) = nodeRotI;
        M(base + 4, base + 4) = nodeRotI;
        M(base + 5, base + 5) = nodeRotI;
    }

    return M;
}

} // namespace FEM
} // namespace Core
