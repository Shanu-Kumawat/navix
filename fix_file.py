import re
import os

with open('src/ui/PropertyPanel.cpp', 'r') as f:
    text = f.read()

# Replace runFEMAnalysis for hasMesh2
text = text.replace(
'''            if (ImGui::Button("Run FEM##bel", ImVec2(femBtnW, 30)) && hasMesh2) {
              Core::FEM::AnalysisConfig cfg;
              cfg.pressure = static_cast<double>(pressureKPa) * 1e3;
              cfg.axialForce = static_cast<double>(axialForceN);
              model2->runFEMAnalysis(cfg);
            }''',
'''            if (ImGui::Button("Run FEM##bel", ImVec2(femBtnW, 30)) && hasMesh2) {
              Core::FEM::AnalysisConfig cfg;
              cfg.pressure = static_cast<double>(pressureKPa) * 1e3;
              cfg.axialForce = static_cast<double>(axialForceN);
              model2->runFEMAnalysis(cfg);
              appContext.showFEMAnalysisView = true;
            }'''
)

# And another one maybe
text = text.replace(
'''            if (ImGui::Button("Run FEM##bel", ImVec2(femBtnW, 30)) && hasMesh) {
              Core::FEM::AnalysisConfig cfg;
              cfg.pressure = static_cast<double>(pressureKPa) * 1e3; // kPa \u2192 Pa
              cfg.axialForce = static_cast<double>(axialForceN);
              model->runFEMAnalysis(cfg);
            }''',
'''            if (ImGui::Button("Run FEM##bel", ImVec2(femBtnW, 30)) && hasMesh) {
              Core::FEM::AnalysisConfig cfg;
              cfg.pressure = static_cast<double>(pressureKPa) * 1e3; // kPa \u2192 Pa
              cfg.axialForce = static_cast<double>(axialForceN);
              model->runFEMAnalysis(cfg);
              appContext.showFEMAnalysisView = true;
            }'''
)

# Add button to open dashboard
text = text.replace(
'''              ImGui::Text("Safety Factor: %.2f", yieldMPa / std::max(maxMPa, 1e-6));

              // Stress contour toggle''',
'''              ImGui::Text("Safety Factor: %.2f", yieldMPa / std::max(maxMPa, 1e-6));
              
              ImGui::Spacing();
              if (ImGui::Button("Open Advanced Dashboard", ImVec2(ImGui::GetContentRegionAvail().x, 24))) {
                  appContext.showFEMAnalysisView = true;
              }

              // Stress contour toggle'''
)

with open('src/ui/PropertyPanel.cpp', 'w') as f:
    f.write(text)
