import re

with open('/home/suyas/drawing_software/src/ui/PropertyPanel.cpp', 'r') as f:
    text = f.read()

mesh_ui_old = '''            // Show/Hide toggle
            bool showMesh = model->getShowFEMMesh();
            if (ImGui::Checkbox("Show Surface Mesh##bellows", &showMesh)) {
              model->setShowFEMMesh(showMesh);
            }'''

mesh_ui_new = '''            // Show/Hide toggle
            bool showMesh = model->getShowFEMMesh();
            if (ImGui::Checkbox("Show Surface Mesh##bellows", &showMesh)) {
              model->setShowFEMMesh(showMesh);
            }
            
            // Mesh Type Selection
            int meshType = static_cast<int>(model->getMeshType());
            const char* meshTypes[] = { "Triangular", "Quadrilateral (Mapped)" };
            if (ImGui::Combo("Mesh Type##bellows", &meshType, meshTypes, 2)) {
                model->setMeshType(static_cast<BellowsModel3D::MeshType>(meshType));
            }'''

text = text.replace(mesh_ui_old, mesh_ui_new)

with open('/home/suyas/drawing_software/src/ui/PropertyPanel.cpp', 'w') as f:
    f.write(text)

