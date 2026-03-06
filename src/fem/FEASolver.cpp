#include "fem/FEASolver.hpp"
#include <Eigen/SparseLU>
#include <Eigen/SparseCholesky>
#include <Eigen/Eigenvalues>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <cmath>

namespace Core {
namespace FEM {

void FEASolver::buildElements(const Core::Meshing::Mesh& mesh,
                              double youngsModulus,
                              double poissonsRatio,
                              double thickness,
                              double meshToMeters,
                              double dens) {
    elements.clear();
    tagToIndex.clear();

    const auto& nodes = mesh.getNodes();
    const auto& elems = mesh.getElements();

    nNodes = static_cast<int>(nodes.size());
    nDOFs = nNodes * DOFS_PER_NODE;

    // Build tag→index mapping
    for (int i = 0; i < nNodes; ++i) {
        tagToIndex[nodes[i].tag] = i;
    }

    // Build shell elements from triangular elements (type 2 = 3-node triangle)
    for (const auto& elem : elems) {
        if (elem.elementType != 2 || elem.nodeTags.size() != 3) continue;

        ShellElement se;
        se.thickness = thickness;
        se.youngsModulus = youngsModulus;
        se.poissonsRatio = poissonsRatio;
        se.density = dens;

        bool valid = true;
        for (int n = 0; n < 3; ++n) {
            auto it = tagToIndex.find(elem.nodeTags[n]);
            if (it == tagToIndex.end()) { valid = false; break; }
            int idx = it->second;
            const auto& node = nodes[idx];
            // Convert mesh coordinates to meters
            se.nodes[n] = Eigen::Vector3d(
                node.position.x * meshToMeters,
                node.position.y * meshToMeters,
                node.position.z * meshToMeters
            );
            // Global DOF indices
            for (int d = 0; d < 6; ++d) {
                se.dofIndices[n * 6 + d] = idx * DOFS_PER_NODE + d;
            }
        }

        if (valid && se.area() > 1e-30) {
            elements.push_back(se);
        }
    }

    std::cout << "[FEASolver] Built " << elements.size() << " shell elements from "
              << elems.size() << " mesh elements, " << nNodes << " nodes, "
              << nDOFs << " DOFs" << std::endl;
}

void FEASolver::setBoundaryConditions(const std::vector<BoundaryCondition>& bcsIn) {
    bcs = bcsIn;
}

bool FEASolver::solve(const Eigen::VectorXd& globalForce) {
    result = FEMResult();

    if (elements.empty() || nDOFs == 0) {
        result.statusMessage = "No elements to solve";
        return false;
    }

    if (globalForce.size() != nDOFs) {
        result.statusMessage = "Force vector size mismatch";
        return false;
    }

    auto t0 = std::chrono::high_resolution_clock::now();

    // --- Assemble global stiffness matrix ---
    std::cout << "[FEASolver] Assembling global stiffness matrix..." << std::endl;

    typedef Eigen::Triplet<double> Triplet;
    std::vector<Triplet> triplets;
    triplets.reserve(elements.size() * 18 * 18);

    int assembled = 0;
    for (const auto& elem : elements) {
        Eigen::MatrixXd Ke = elem.stiffnessMatrix();

        for (int i = 0; i < 18; ++i) {
            int gi = elem.dofIndices[i];
            for (int j = 0; j < 18; ++j) {
                int gj = elem.dofIndices[j];
                double val = Ke(i, j);
                if (std::abs(val) > 1e-30) {
                    triplets.emplace_back(gi, gj, val);
                }
            }
        }
        ++assembled;
        if (assembled % 200 == 0) {
            std::cout << "  Assembled " << assembled << "/" << elements.size() << " elements" << std::endl;
        }
    }

    Eigen::SparseMatrix<double> K(nDOFs, nDOFs);
    K.setFromTriplets(triplets.begin(), triplets.end());

    auto t1 = std::chrono::high_resolution_clock::now();
    double assemblyMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    std::cout << "[FEASolver] Assembly done in " << assemblyMs << " ms. "
              << "NNZ: " << K.nonZeros() << std::endl;

    // --- Apply boundary conditions (penalty method) ---
    // Penalty value: large number relative to max diagonal
    double maxDiag = 0;
    for (int i = 0; i < nDOFs; ++i) {
        maxDiag = std::max(maxDiag, std::abs(K.coeff(i, i)));
    }
    double penalty = maxDiag * 1e8;
    if (penalty < 1e10) penalty = 1e20;

    Eigen::VectorXd F = globalForce;

    std::unordered_set<int> constrainedDofs;
    for (const auto& bc : bcs) {
        auto it = tagToIndex.find(bc.nodeTag);
        if (it == tagToIndex.end()) continue;
        int dofIdx = it->second * DOFS_PER_NODE + static_cast<int>(bc.dof);
        if (dofIdx >= 0 && dofIdx < nDOFs) {
            constrainedDofs.insert(dofIdx);
        }
    }

    // Apply penalty to constrained DOFs
    for (int dof : constrainedDofs) {
        K.coeffRef(dof, dof) += penalty;
        F(dof) = 0;  // prescribed displacement = 0 (fixed)
    }

    std::cout << "[FEASolver] Applied " << constrainedDofs.size()
              << " BC constraints (penalty method)" << std::endl;

    // --- Solve K * u = F ---
    std::cout << "[FEASolver] Solving sparse system (" << nDOFs << " x " << nDOFs << ")..." << std::endl;

    K.makeCompressed();

    // Use SimplicialLDLT for symmetric systems (stiffness matrix with penalty BCs is SPD)
    Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> solver;
    solver.analyzePattern(K);
    solver.factorize(K);

    if (solver.info() != Eigen::Success) {
        // Fallback to SparseLU for non-SPD cases
        std::cout << "[FEASolver] LDLT failed, falling back to SparseLU..." << std::endl;
        Eigen::SparseLU<Eigen::SparseMatrix<double>> luSolver;
        luSolver.analyzePattern(K);
        luSolver.factorize(K);
        if (luSolver.info() != Eigen::Success) {
            result.statusMessage = "Matrix factorization failed (singular system)";
            std::cerr << "[FEASolver] " << result.statusMessage << std::endl;
            return false;
        }
        result.displacements = luSolver.solve(F);
        if (luSolver.info() != Eigen::Success) {
            result.statusMessage = "Solve failed";
            std::cerr << "[FEASolver] " << result.statusMessage << std::endl;
            return false;
        }
    } else {
        result.displacements = solver.solve(F);
        if (solver.info() != Eigen::Success) {
            result.statusMessage = "Solve failed";
            std::cerr << "[FEASolver] " << result.statusMessage << std::endl;
            return false;
        }
    }

    auto t2 = std::chrono::high_resolution_clock::now();
    double solveMs = std::chrono::duration<double, std::milli>(t2 - t1).count();

    // Find max displacement magnitude
    result.maxDisplacement = 0;
    for (int i = 0; i < nNodes; ++i) {
        double ux = result.displacements(i*6 + 0);
        double uy = result.displacements(i*6 + 1);
        double uz = result.displacements(i*6 + 2);
        double mag = std::sqrt(ux*ux + uy*uy + uz*uz);
        result.maxDisplacement = std::max(result.maxDisplacement, mag);
    }

    std::cout << "[FEASolver] Solved in " << solveMs << " ms. "
              << "Max displacement: " << result.maxDisplacement * 1000.0 << " mm" << std::endl;

    result.isValid = true;
    result.statusMessage = "Converged";
    return true;
}

bool FEASolver::solve(double pressure, double axialForce,
                      const std::vector<uint64_t>& cuffBNodes,
                      const Core::Meshing::Mesh& mesh) {
    // Build global force vector from pressure loads and axial force
    Eigen::VectorXd F = Eigen::VectorXd::Zero(nDOFs);

    // Assemble consistent pressure loads from all elements
    for (const auto& elem : elements) {
        Eigen::VectorXd fe = elem.pressureLoadVector(pressure);
        for (int i = 0; i < 18; ++i) {
            F(elem.dofIndices[i]) += fe(i);
        }
    }

    // Distribute axial force equally among cuff B nodes (X-direction)
    if (std::abs(axialForce) > 1e-30 && !cuffBNodes.empty()) {
        double forcePerNode = axialForce / static_cast<double>(cuffBNodes.size());
        for (auto tag : cuffBNodes) {
            auto it = tagToIndex.find(tag);
            if (it != tagToIndex.end()) {
                int dofX = it->second * DOFS_PER_NODE + static_cast<int>(DOFType::UX);
                F(dofX) += forcePerNode;
            }
        }
    }

    return solve(F);
}

void FEASolver::computeStresses() {
    if (!result.isValid || result.displacements.size() != nDOFs) return;

    result.elementStresses.clear();
    result.elementStresses.reserve(elements.size());
    result.maxVonMises = 0;
    result.minVonMises = 1e30;

    for (size_t e = 0; e < elements.size(); ++e) {
        const auto& elem = elements[e];

        // Extract element displacements from global vector
        Eigen::VectorXd de(18);
        for (int i = 0; i < 18; ++i) {
            de(i) = result.displacements(elem.dofIndices[i]);
        }

        auto sr = elem.computeStress(de);

        ElementStress es;
        es.elementTag = e + 1;
        es.membraneStress = sr.membrane;
        es.bendingStressTop = sr.bendingTop;
        es.bendingStressBot = sr.bendingBot;
        es.vonMisesTop = sr.vmTop;
        es.vonMisesBot = sr.vmBot;
        es.vonMisesMax = std::max(sr.vmTop, sr.vmBot);

        result.maxVonMises = std::max(result.maxVonMises, es.vonMisesMax);
        result.minVonMises = std::min(result.minVonMises, es.vonMisesMax);

        result.elementStresses.push_back(es);
    }

    std::cout << "[FEASolver] Stress range: " << result.minVonMises / 1e6 << " - "
              << result.maxVonMises / 1e6 << " MPa" << std::endl;
}

void FEASolver::computeNodeStresses(const Core::Meshing::Mesh& /*mesh*/) {
    if (!result.isValid || result.elementStresses.empty()) return;

    result.nodeVonMises.assign(nNodes, 0);
    std::vector<int> nodeCount(nNodes, 0);

    for (size_t e = 0; e < elements.size(); ++e) {
        double vm = result.elementStresses[e].vonMisesMax;
        for (int n = 0; n < 3; ++n) {
            int nodeIdx = elements[e].dofIndices[n * 6] / DOFS_PER_NODE;
            if (nodeIdx >= 0 && nodeIdx < nNodes) {
                result.nodeVonMises[nodeIdx] += vm;
                nodeCount[nodeIdx]++;
            }
        }
    }

    // Average
    for (int i = 0; i < nNodes; ++i) {
        if (nodeCount[i] > 0) {
            result.nodeVonMises[i] /= nodeCount[i];
        }
    }
}

// ============================================================
//  Reaction forces: R = K*u - F at constrained DOFs
// ============================================================

void FEASolver::computeReactionForces(const Eigen::VectorXd& appliedForce) {
    if (!result.isValid || result.displacements.size() != nDOFs) return;
    if (appliedForce.size() != nDOFs) return;

    result.reactionForces.clear();

    // Reassemble global stiffness (without penalty) to get true reactions
    // We use the same assembly as solve() but skip BC modification
    typedef Eigen::Triplet<double> Triplet;
    std::vector<Triplet> triplets;
    triplets.reserve(elements.size() * 18 * 18);

    for (const auto& elem : elements) {
        Eigen::MatrixXd Ke = elem.stiffnessMatrix();
        for (int i = 0; i < 18; ++i) {
            int gi = elem.dofIndices[i];
            for (int j = 0; j < 18; ++j) {
                int gj = elem.dofIndices[j];
                double val = Ke(i, j);
                if (std::abs(val) > 1e-30) {
                    triplets.emplace_back(gi, gj, val);
                }
            }
        }
    }

    Eigen::SparseMatrix<double> K(nDOFs, nDOFs);
    K.setFromTriplets(triplets.begin(), triplets.end());

    // R = K*u - F
    Eigen::VectorXd R = K * result.displacements - appliedForce;

    // Group reaction forces by constrained nodes
    std::unordered_map<uint64_t, ReactionForce> nodeReactions;

    // Build inverse mapping: index → tag
    std::unordered_map<int, uint64_t> indexToTag;
    for (const auto& p : tagToIndex) {
        indexToTag[p.second] = p.first;
    }

    // Identify constrained nodes
    std::unordered_set<int> constrainedNodes;
    for (const auto& bc : bcs) {
        auto it = tagToIndex.find(bc.nodeTag);
        if (it != tagToIndex.end()) constrainedNodes.insert(it->second);
    }

    for (int nodeIdx : constrainedNodes) {
        auto tagIt = indexToTag.find(nodeIdx);
        if (tagIt == indexToTag.end()) continue;

        ReactionForce rf;
        rf.nodeTag = tagIt->second;
        int base = nodeIdx * DOFS_PER_NODE;
        rf.force  = Eigen::Vector3d(R(base), R(base+1), R(base+2));
        rf.moment = Eigen::Vector3d(R(base+3), R(base+4), R(base+5));
        rf.magnitude = rf.force.norm();
        nodeReactions[rf.nodeTag] = rf;
    }

    // Sum total reaction
    Eigen::Vector3d totalForce = Eigen::Vector3d::Zero();
    Eigen::Vector3d totalMoment = Eigen::Vector3d::Zero();
    for (auto& [tag, rf] : nodeReactions) {
        totalForce += rf.force;
        totalMoment += rf.moment;
        result.reactionForces.push_back(rf);
    }

    result.totalReactionMagnitude = totalForce.norm();

    std::cout << "[FEASolver] Reaction forces: "
              << result.reactionForces.size() << " nodes, total |F| = "
              << result.totalReactionMagnitude << " N, total F = ("
              << totalForce.x() << ", " << totalForce.y() << ", "
              << totalForce.z() << ")" << std::endl;
}

// ============================================================
//  Modal analysis: K*φ = ω²*M*φ
// ============================================================

bool FEASolver::solveModal(int numModes, ModalResult& modalResult) {
    modalResult = ModalResult();

    if (elements.empty() || nDOFs == 0) {
        modalResult.statusMessage = "No elements for modal analysis";
        return false;
    }

    auto t0 = std::chrono::high_resolution_clock::now();

    // --- Assemble global K and M ---
    typedef Eigen::Triplet<double> Triplet;
    std::vector<Triplet> kTrip, mTrip;
    kTrip.reserve(elements.size() * 18 * 18);
    mTrip.reserve(elements.size() * 18);

    for (const auto& elem : elements) {
        Eigen::MatrixXd Ke = elem.stiffnessMatrix();
        Eigen::MatrixXd Me = elem.massMatrix();

        for (int i = 0; i < 18; ++i) {
            int gi = elem.dofIndices[i];
            // Mass matrix (diagonal)
            double mVal = Me(i, i);
            if (std::abs(mVal) > 1e-30) {
                mTrip.emplace_back(gi, gi, mVal);
            }
            // Stiffness matrix
            for (int j = 0; j < 18; ++j) {
                int gj = elem.dofIndices[j];
                double kVal = Ke(i, j);
                if (std::abs(kVal) > 1e-30) {
                    kTrip.emplace_back(gi, gj, kVal);
                }
            }
        }
    }

    Eigen::SparseMatrix<double> K(nDOFs, nDOFs);
    Eigen::SparseMatrix<double> M(nDOFs, nDOFs);
    K.setFromTriplets(kTrip.begin(), kTrip.end());
    M.setFromTriplets(mTrip.begin(), mTrip.end());

    // --- Apply BCs: eliminate constrained DOFs by setting large stiffness and mass ---
    std::unordered_set<int> constrainedDofs;
    for (const auto& bc : bcs) {
        auto it = tagToIndex.find(bc.nodeTag);
        if (it == tagToIndex.end()) continue;
        int dofIdx = it->second * DOFS_PER_NODE + static_cast<int>(bc.dof);
        if (dofIdx >= 0 && dofIdx < nDOFs) constrainedDofs.insert(dofIdx);
    }

    // Identify free DOFs (unconstrained)
    std::vector<int> freeDofs;
    freeDofs.reserve(nDOFs - constrainedDofs.size());
    for (int i = 0; i < nDOFs; ++i) {
        if (constrainedDofs.find(i) == constrainedDofs.end()) {
            freeDofs.push_back(i);
        }
    }

    int nFree = static_cast<int>(freeDofs.size());
    if (nFree < numModes) {
        modalResult.statusMessage = "Not enough free DOFs for requested modes";
        return false;
    }

    std::cout << "[FEASolver] Modal: " << nFree << " free DOFs, "
              << constrainedDofs.size() << " constrained" << std::endl;

    // DOF mapping: global DOF index -> reduced (free) index
    std::vector<int> dofMap(nDOFs, -1);
    for (int i = 0; i < nFree; ++i) {
        dofMap[freeDofs[i]] = i;
    }

    // Build sparse reduced K_r and diagonal M_r for free DOFs only
    std::vector<Triplet> krTrip;
    krTrip.reserve(elements.size() * 18 * 18);
    Eigen::VectorXd Mr = Eigen::VectorXd::Zero(nFree);

    for (const auto& elem : elements) {
        Eigen::MatrixXd Ke = elem.stiffnessMatrix();
        Eigen::MatrixXd Me = elem.massMatrix();
        for (int i = 0; i < 18; ++i) {
            int gi = dofMap[elem.dofIndices[i]];
            if (gi < 0) continue;
            Mr(gi) += Me(i, i);
            for (int j = 0; j < 18; ++j) {
                int gj = dofMap[elem.dofIndices[j]];
                if (gj < 0) continue;
                double kVal = Ke(i, j);
                if (std::abs(kVal) > 1e-30) {
                    krTrip.emplace_back(gi, gj, kVal);
                }
            }
        }
    }

    Eigen::SparseMatrix<double> Kr(nFree, nFree);
    Kr.setFromTriplets(krTrip.begin(), krTrip.end());

    // M^{-1/2} for mass-weighted transformation (diagonal mass)
    Eigen::VectorXd Minvhalf(nFree);
    for (int i = 0; i < nFree; ++i) {
        Minvhalf(i) = (Mr(i) > 1e-30) ? 1.0 / std::sqrt(Mr(i)) : 0.0;
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    std::cout << "[FEASolver] Modal assembly done in "
              << std::chrono::duration<double, std::milli>(t1 - t0).count()
              << " ms, solving eigenvalue problem..." << std::endl;

    // Use shift-invert subspace iteration for the lowest modes
    // Solve (K - σM)^{-1} M v = θ v, where σ is a small shift
    // The largest θ correspond to eigenvalues closest to σ (the lowest modes)
    double sigma = 0.01; // small shift to avoid singularity from rigid body modes

    // Build shifted matrix: Ks = K - σ * diag(M)
    Eigen::SparseMatrix<double> Ks = Kr;
    for (int i = 0; i < nFree; ++i) {
        Ks.coeffRef(i, i) -= sigma * Mr(i);
    }

    // Factorize shifted stiffness using sparse LLT (Cholesky)
    Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> solver;
    solver.compute(Ks);
    if (solver.info() != Eigen::Success) {
        // Fallback: try with a larger shift
        sigma = 1.0;
        Ks = Kr;
        for (int i = 0; i < nFree; ++i) {
            Ks.coeffRef(i, i) -= sigma * Mr(i);
        }
        solver.compute(Ks);
        if (solver.info() != Eigen::Success) {
            modalResult.statusMessage = "Sparse factorization failed";
            return false;
        }
    }

    // Subspace iteration: work with a block of vectors
    int blockSize = std::min(nFree, std::max(numModes * 3, 18));
    Eigen::MatrixXd V(nFree, blockSize);

    // Initialize with random vectors
    std::srand(42);
    V = Eigen::MatrixXd::Random(nFree, blockSize);

    // QR to orthogonalize initial vectors
    Eigen::HouseholderQR<Eigen::MatrixXd> qr(V);
    V = qr.householderQ() * Eigen::MatrixXd::Identity(nFree, blockSize);

    const int maxIter = 200;
    const double tolerance = 1e-8;
    Eigen::VectorXd prevEigs = Eigen::VectorXd::Zero(blockSize);

    std::cout << "[FEASolver] Subspace iteration: " << blockSize
              << " vectors, up to " << maxIter << " iterations" << std::endl;

    for (int iter = 0; iter < maxIter; ++iter) {
        // Apply M * V (diagonal mass)
        Eigen::MatrixXd MV(nFree, blockSize);
        for (int j = 0; j < blockSize; ++j) {
            MV.col(j) = Mr.asDiagonal() * V.col(j);
        }

        // Solve (K - σM) * W = M * V
        Eigen::MatrixXd W(nFree, blockSize);
        for (int j = 0; j < blockSize; ++j) {
            W.col(j) = solver.solve(MV.col(j));
        }

        // Build reduced eigenvalue problem: project onto subspace
        // H = W^T K W,  S = W^T M W
        Eigen::MatrixXd Hk = W.transpose() * Kr * W;
        Eigen::MatrixXd Sm(blockSize, blockSize);
        for (int j = 0; j < blockSize; ++j) {
            Eigen::VectorXd Mwj = Mr.asDiagonal() * W.col(j);
            for (int k = 0; k <= j; ++k) {
                Sm(j, k) = W.col(k).dot(Mwj);
                Sm(k, j) = Sm(j, k);
            }
        }

        // Solve small generalized eigenproblem: Hk * y = λ Sm * y
        Eigen::GeneralizedSelfAdjointEigenSolver<Eigen::MatrixXd> subEig(Hk, Sm);
        if (subEig.info() != Eigen::Success) {
            // Fallback: use standard eigenvalue problem with Cholesky of Sm
            Eigen::LLT<Eigen::MatrixXd> llt(Sm);
            if (llt.info() == Eigen::Success) {
                Eigen::MatrixXd L = llt.matrixL();
                Eigen::MatrixXd Linv = L.inverse();
                Eigen::MatrixXd Atilde = Linv * Hk * Linv.transpose();
                Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> fallbackEig(Atilde);
                if (fallbackEig.info() != Eigen::Success) {
                    modalResult.statusMessage = "Subspace eigensolve failed";
                    return false;
                }
                V = W * Linv.transpose() * fallbackEig.eigenvectors();
            } else {
                modalResult.statusMessage = "Subspace eigensolve failed (mass matrix singular)";
                return false;
            }
            continue;
        }

        // Update subspace: V = W * eigenvectors (Ritz vectors)
        V = W * subEig.eigenvectors();

        // Check convergence on the first numModes eigenvalues
        Eigen::VectorXd curEigs = subEig.eigenvalues();
        double maxChange = 0.0;
        for (int m = 0; m < std::min(numModes, blockSize); ++m) {
            if (std::abs(curEigs(m)) > 1e-10) {
                maxChange = std::max(maxChange,
                    std::abs(curEigs(m) - prevEigs(m)) / std::abs(curEigs(m)));
            }
        }
        prevEigs = curEigs;

        if (iter > 0 && maxChange < tolerance) {
            std::cout << "[FEASolver] Converged after " << (iter + 1)
                      << " iterations (change=" << maxChange << ")" << std::endl;
            break;
        }
    }

    // Final Rayleigh-Ritz to extract converged eigenvalues
    Eigen::MatrixXd Hfinal = V.transpose() * Kr * V;
    Eigen::MatrixXd Sfinal(blockSize, blockSize);
    for (int j = 0; j < blockSize; ++j) {
        Eigen::VectorXd Mvj = Mr.asDiagonal() * V.col(j);
        for (int k = 0; k <= j; ++k) {
            Sfinal(j, k) = V.col(k).dot(Mvj);
            Sfinal(k, j) = Sfinal(j, k);
        }
    }

    Eigen::GeneralizedSelfAdjointEigenSolver<Eigen::MatrixXd> finalEig(Hfinal, Sfinal);
    if (finalEig.info() != Eigen::Success) {
        modalResult.statusMessage = "Final eigenvalue extraction failed";
        return false;
    }

    auto t2 = std::chrono::high_resolution_clock::now();
    std::cout << "[FEASolver] Eigensolve done in "
              << std::chrono::duration<double, std::milli>(t2 - t1).count() << " ms" << std::endl;

    const auto& eigenvalues = finalEig.eigenvalues();
    const auto& ritzVectors = V * finalEig.eigenvectors(); // full Ritz vectors in reduced space

    // Extract lowest positive modes (skip near-zero rigid body modes)
    int modesFound = 0;
    for (int i = 0; i < blockSize && modesFound < numModes; ++i) {
        double lambda = eigenvalues(i);
        if (lambda < 1.0) continue; // skip rigid body / near-zero modes

        double omega = std::sqrt(lambda);
        double freq = omega / (2.0 * M_PI);

        // Map reduced eigenvector back to full DOF space
        Eigen::VectorXd vReduced = ritzVectors.col(i);
        Eigen::VectorXd phiFull = Eigen::VectorXd::Zero(nDOFs);
        for (int j = 0; j < nFree; ++j) {
            phiFull(freeDofs[j]) = vReduced(j);
        }

        // Normalize mode shape to max displacement = 1
        double maxDisp = 0;
        for (int n = 0; n < nNodes; ++n) {
            double ux = phiFull(n*6 + 0);
            double uy = phiFull(n*6 + 1);
            double uz = phiFull(n*6 + 2);
            maxDisp = std::max(maxDisp, std::sqrt(ux*ux + uy*uy + uz*uz));
        }
        if (maxDisp > 1e-30) phiFull /= maxDisp;

        ModeShape mode;
        mode.modeNumber = modesFound + 1;
        mode.frequency = freq;
        mode.omega = omega;
        mode.modeVector = phiFull;
        modalResult.modes.push_back(mode);
        ++modesFound;

        std::cout << "  Mode " << mode.modeNumber << ": f = " << freq
                  << " Hz (\u03c9 = " << omega << " rad/s)" << std::endl;
    }

    modalResult.isValid = (modesFound > 0);
    modalResult.statusMessage = "Found " + std::to_string(modesFound) + " modes";
    return modalResult.isValid;
}

} // namespace FEM
} // namespace Core
