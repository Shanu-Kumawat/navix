with open('/home/suyas/drawing_software/include/BellowsModel3D.hpp', 'r') as f:
    text = f.read()

text = text.replace(
'''    bool getShowDeformed() const { return showDeformed; }''',
'''    bool getShowDeformed() const { return showDeformed; }
    
    enum class MeshType { TRIANGULAR = 0, QUADRILATERAL = 1 };
    void setMeshType(MeshType type) { meshType = type; generateFEMMesh(femElementSize); }
    MeshType getMeshType() const { return meshType; }'''
)

text = text.replace(
'''    Core::FEM::Material femMaterial;''',
'''    Core::FEM::Material femMaterial;
    MeshType meshType = MeshType::TRIANGULAR;'''
)

with open('/home/suyas/drawing_software/include/BellowsModel3D.hpp', 'w') as f:
    f.write(text)

with open('/home/suyas/drawing_software/src/BellowsModel3D.cpp', 'r') as f:
    text = f.read()

mesh_old = '''    // Create triangular elements connecting adjacent rings
    // Element type 2 = 3-node triangle in Gmsh convention
    uint64_t elemTag = 1;
    for (size_t pi = 0; pi + 1 < nProfile; ++pi) {
        for (size_t ai = 0; ai < nAngle; ++ai) {
            size_t aiNext = (ai + 1) % nAngle;

            // Node tags for the quad formed by two adjacent profile points and two angular positions
            uint64_t n00 = pi * nAngle + ai + 1;         // current profile, current angle
            uint64_t n10 = (pi + 1) * nAngle + ai + 1;   // next profile, current angle
            uint64_t n01 = pi * nAngle + aiNext + 1;      // current profile, next angle
            uint64_t n11 = (pi + 1) * nAngle + aiNext + 1; // next profile, next angle

            // Triangle 1
            femMesh.addElement(elemTag++, 2, {n00, n10, n01});
            // Triangle 2
            femMesh.addElement(elemTag++, 2, {n10, n11, n01});
        }
    }'''

mesh_new = '''    uint64_t elemTag = 1;
    for (size_t pi = 0; pi + 1 < nProfile; ++pi) {
        for (size_t ai = 0; ai < nAngle; ++ai) {
            size_t aiNext = (ai + 1) % nAngle;

            uint64_t n00 = pi * nAngle + ai + 1;
            uint64_t n10 = (pi + 1) * nAngle + ai + 1;
            uint64_t n01 = pi * nAngle + aiNext + 1;
            uint64_t n11 = (pi + 1) * nAngle + aiNext + 1;

            if (meshType == MeshType::QUADRILATERAL) {
                // Quad element (type 3)
                femMesh.addElement(elemTag++, 3, {n00, n10, n11, n01});
            } else {
                // Triangular elements (type 2)
                femMesh.addElement(elemTag++, 2, {n00, n10, n01});
                femMesh.addElement(elemTag++, 2, {n10, n11, n01});
            }
        }
    }'''

text = text.replace(mesh_old, mesh_new)

# Note: In src/Base3DModel.cpp is where wireframe is made... Oh wait, Base3DModel has setupMeshWireframeBuffers.
# That needs fixing too.

with open('/home/suyas/drawing_software/src/BellowsModel3D.cpp', 'w') as f:
    f.write(text)

with open('/home/suyas/drawing_software/src/Base3DModel.cpp', 'r') as f:
    bt = f.read()

wireframe_old = '''    for (const auto& elem : mesh.getElements()) {
        const auto& nodesList = elem.nodeTags;
        if (nodesList.size() == 3) {
            auto it0 = tagToIndex.find(nodesList[0]);
            auto it1 = tagToIndex.find(nodesList[1]);
            auto it2 = tagToIndex.find(nodesList[2]);

            if (it0 != tagToIndex.end() && it1 != tagToIndex.end() && it2 != tagToIndex.end()) {
                const auto& p0 = nodes[it0->second].position;
                const auto& p1 = nodes[it1->second].position;
                const auto& p2 = nodes[it2->second].position;

                femWireframeVertices.push_back(p0.x); femWireframeVertices.push_back(p0.y); femWireframeVertices.push_back(p0.z);
                femWireframeVertices.push_back(p1.x); femWireframeVertices.push_back(p1.y); femWireframeVertices.push_back(p1.z);

                femWireframeVertices.push_back(p1.x); femWireframeVertices.push_back(p1.y); femWireframeVertices.push_back(p1.z);
                femWireframeVertices.push_back(p2.x); femWireframeVertices.push_back(p2.y); femWireframeVertices.push_back(p2.z);

                femWireframeVertices.push_back(p2.x); femWireframeVertices.push_back(p2.y); femWireframeVertices.push_back(p2.z);
                femWireframeVertices.push_back(p0.x); femWireframeVertices.push_back(p0.y); femWireframeVertices.push_back(p0.z);
            }
        }
    }'''

wireframe_new = '''    for (const auto& elem : mesh.getElements()) {
        const auto& nodesList = elem.nodeTags;
        if (nodesList.size() >= 3) {
            for (size_t i = 0; i < nodesList.size(); ++i) {
                auto it0 = tagToIndex.find(nodesList[i]);
                auto it1 = tagToIndex.find(nodesList[(i + 1) % nodesList.size()]);
                
                if (it0 != tagToIndex.end() && it1 != tagToIndex.end()) {
                    const auto& p0 = nodes[it0->second].position;
                    const auto& p1 = nodes[it1->second].position;
                    
                    femWireframeVertices.push_back(p0.x); femWireframeVertices.push_back(p0.y); femWireframeVertices.push_back(p0.z);
                    femWireframeVertices.push_back(p1.x); femWireframeVertices.push_back(p1.y); femWireframeVertices.push_back(p1.z);
                }
            }
        }
    }'''

bt = bt.replace(wireframe_old, wireframe_new)
with open('/home/suyas/drawing_software/src/Base3DModel.cpp', 'w') as f:
    f.write(bt)
