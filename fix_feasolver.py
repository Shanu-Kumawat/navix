import re

with open('/home/suyas/drawing_software/src/fem/FEASolver.cpp', 'r') as f:
    text = f.read()

# Fix buildElements to accept both triangles and quads
build_elements_old = """    // Build shell elements from triangular elements (type 2 = 3-node triangle)
    for (const auto& elem : elems) {
        if (elem.elementType != 2 || elem.nodeTags.size() != 3) continue;

        ShellElement se;
        se.thickness = thickness;
        se.youngsModulus = youngsModulus;
        se.poissonsRatio = poissonsRatio;
        se.density = dens;

        bool valid = true;
        for (int k = 0; k < 3; ++k) {
            uint64_t tag = elem.nodeTags[k];
            auto it = tagToIndex.find(tag);
            if (it == tagToIndex.end()) { valid = false; break; }
            int idx = it->second;
            se.nodes[k] = nodes[idx].position * meshToMeters;
            for (int d = 0; d < DOFS_PER_NODE; ++d) {
                se.dofIndices[k * DOFS_PER_NODE + d] = idx * DOFS_PER_NODE + d;
            }
        }
        if (valid) elements.push_back(se);
    }"""

build_elements_new = """    // Build shell elements (type 2 = triangle, type 3 = quad)
    for (const auto& elem : elems) {
        if (elem.elementType == 2 && elem.nodeTags.size() == 3) {
            ShellElement se;
            se.parentTag = elem.tag;
            se.thickness = thickness;
            se.youngsModulus = youngsModulus;
            se.poissonsRatio = poissonsRatio;
            se.density = dens;

            bool valid = true;
            for (int k = 0; k < 3; ++k) {
                uint64_t tag = elem.nodeTags[k];
                auto it = tagToIndex.find(tag);
                if (it == tagToIndex.end()) { valid = false; break; }
                int idx = it->second;
                se.nodes[k] = nodes[idx].position * meshToMeters;
                for (int d = 0; d < 6; ++d) {
                    se.dofIndices[k * 6 + d] = idx * 6 + d;
                }
            }
            if (valid) elements.push_back(se);
        } else if (elem.elementType == 3 && elem.nodeTags.size() == 4) {
            // Split Quad into 2 Triangles: (0, 1, 2) and (0, 2, 3)
            int tris[2][3] = {{0, 1, 2}, {0, 2, 3}};
            for (int t = 0; t < 2; ++t) {
                ShellElement se;
                se.parentTag = elem.tag;
                se.thickness = thickness;
                se.youngsModulus = youngsModulus;
                se.poissonsRatio = poissonsRatio;
                se.density = dens;

                bool valid = true;
                for (int k = 0; k < 3; ++k) {
                    uint64_t tag = elem.nodeTags[tris[t][k]];
                    auto it = tagToIndex.find(tag);
                    if (it == tagToIndex.end()) { valid = false; break; }
                    int idx = it->second;
                    se.nodes[k] = nodes[idx].position * meshToMeters;
                    for (int d = 0; d < 6; ++d) {
                        se.dofIndices[k * 6 + d] = idx * 6 + d;
                    }
                }
                if (valid) elements.push_back(se);
            }
        }
    }"""

text = text.replace(build_elements_old, build_elements_new)

# Fix computeStresses to average by parentTag
compute_old = """    result.elementStresses.clear();
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
    }"""

# New computeStresses code using parentTag
compute_new = """    result.elementStresses.clear();
    result.maxVonMises = 0;
    result.minVonMises = 1e30;

    std::map<uint64_t, std::vector<ElementStress>> grouped;

    for (size_t e = 0; e < elements.size(); ++e) {
        const auto& elem = elements[e];
        Eigen::VectorXd de(18);
        for (int i = 0; i < 18; ++i) {
            de(i) = result.displacements(elem.dofIndices[i]);
        }
        auto sr = elem.computeStress(de);

        ElementStress es;
        es.elementTag = elem.parentTag;
        es.membraneStress = sr.membrane;
        es.bendingStressTop = sr.bendingTop;
        es.bendingStressBot = sr.bendingBot;
        es.vonMisesTop = sr.vmTop;
        es.vonMisesBot = sr.vmBot;
        es.vonMisesMax = std::max(sr.vmTop, sr.vmBot);
        
        grouped[elem.parentTag].push_back(es);
    }

    for (const auto& pair : grouped) {
        ElementStress avg;
        avg.elementTag = pair.first;
        avg.membraneStress = Eigen::Vector3d::Zero();
        avg.bendingStressTop = Eigen::Vector3d::Zero();
        avg.bendingStressBot = Eigen::Vector3d::Zero();
        avg.vonMisesTop = 0;
        avg.vonMisesBot = 0;
        avg.vonMisesMax = 0;

        for (const auto& es : pair.second) {
            avg.membraneStress += es.membraneStress;
            avg.bendingStressTop += es.bendingStressTop;
            avg.bendingStressBot += es.bendingStressBot;
            avg.vonMisesTop += es.vonMisesTop;
            avg.vonMisesBot += es.vonMisesBot;
            avg.vonMisesMax += es.vonMisesMax;
        }

        double n = static_cast<double>(pair.second.size());
        avg.membraneStress /= n;
        avg.bendingStressTop /= n;
        avg.bendingStressBot /= n;
        avg.vonMisesTop /= n;
        avg.vonMisesBot /= n;
        avg.vonMisesMax /= n;

        result.maxVonMises = std::max(result.maxVonMises, avg.vonMisesMax);
        result.minVonMises = std::min(result.minVonMises, avg.vonMisesMax);
        
        result.elementStresses.push_back(avg);
    }"""

text = text.replace(compute_old, compute_new)
text = "#include <map>\n" + text 

with open('/home/suyas/drawing_software/src/fem/FEASolver.cpp', 'w') as f:
    f.write(text)
