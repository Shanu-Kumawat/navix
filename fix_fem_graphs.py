import os

with open('/home/suyas/drawing_software/src/ui/FEMAnalysisWindow.cpp', 'r') as f:
    text = f.read()

header = """#include "ui/FEMAnalysisWindow.hpp"
#include <imgui.h>
#include "BellowsModel3D.hpp"
#include "fem/FEMTypes.hpp"
#include "meshing/Mesh.hpp"
#include <vector>
#include <algorithm>
#include <cstdio>
"""

text = text.replace(
'''#include "ui/FEMAnalysisWindow.hpp"
#include <imgui.h>
#include "BellowsModel3D.hpp"
#include "fem/FEMTypes.hpp"
#include <vector>
#include <algorithm>
#include <cstdio>''', header)

new_content = """
        if (ImGui::CollapsingHeader("Stress vs Axial Dimension", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (!result.elementStresses.empty() && bellowsModel->hasFEMMesh()) {
                const auto& mesh = bellowsModel->getFEMMesh();
                const auto& nodes = mesh.getNodes();
                const auto& elements = mesh.getElements();

                // Compute bounding box X 
                double minX = 1e30, maxX = -1e30;
                for (const auto& n : nodes) {
                    minX = std::min(minX, n.position.x);
                    maxX = std::max(maxX, n.position.x);
                }

                const int numProfileBins = 60;
                std::vector<float> stressProfile(numProfileBins, 0.0f);
                std::vector<int> countProfile(numProfileBins, 0);

                double rangeX = maxX - minX;
                if (rangeX > 1e-6) {
                    for (const auto& es : result.elementStresses) {
                        if (es.elementTag <= elements.size()) {
                            const auto& el = elements[es.elementTag - 1]; // Assume tag is 1-based or similar, actually let's find the centroid safely
                            // better to map element to its centroid linearly
                            double cx = 0;
                            int vc = 0;
                            for (uint64_t nTag : el.nodeTags) {
                                // Find node
                                for (const auto& n : nodes) {
                                    if (n.tag == nTag) {
                                        cx += n.position.x;
                                        vc++;
                                        break;
                                    }
                                }
                            }
                            if (vc > 0) cx /= vc;
                            
                            double normalizedX = (cx - minX) / rangeX;
                            int binIdx = static_cast<int>(normalizedX * numProfileBins);
                            if (binIdx >= numProfileBins) binIdx = numProfileBins - 1;
                            if (binIdx < 0) binIdx = 0;

                            stressProfile[binIdx] += static_cast<float>(es.vonMisesMax / 1e6);
                            countProfile[binIdx]++;
                        }
                    }

                    for (int i = 0; i < numProfileBins; i++) {
                        if (countProfile[i] > 0) {
                            stressProfile[i] /= countProfile[i];
                        }
                    }

                    ImGui::PlotLines("##StressAlongX", stressProfile.data(), numProfileBins, 0, "Avg Von Mises (MPa) vs Bellows Length (0 to L)", 0.0f, static_cast<float>(result.maxVonMises/1e6), ImVec2(ImGui::GetContentRegionAvail().x, 150.0f));
                }
            }
        }

        if (ImGui::CollapsingHeader("Modal Analysis Results", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (bellowsModel->hasModalResult()) {
                const auto& modalRes = bellowsModel->getModalResult();
                ImGui::Text("Status: %s", modalRes.statusMessage.c_str());
                if (!modalRes.modes.empty()) {
                    std::vector<float> freqs;
                    for (const auto& m : modalRes.modes) freqs.push_back(static_cast<float>(m.frequency));
                    
                    float maxF = 0;
                    for (float f : freqs) if (f > maxF) maxF = f;
                    
                    ImGui::PlotHistogram("##Modes", freqs.data(), freqs.size(), 0, "Frequency (Hz) per Mode", 0.0f, maxF, ImVec2(ImGui::GetContentRegionAvail().x, 150.0f));
                }
            } else {
                ImGui::Text("No modal analysis results. Please run modal analysis.");
            }
        }

        if (ImGui::CollapsingHeader("Mesh Convergence Study", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (bellowsModel->hasConvergenceData()) {
                const auto& convData = bellowsModel->getConvergenceData();
                std::vector<float> points;
                float maxStress = 0.0f;
                for (const auto& pt : convData) {
                    float s = static_cast<float>(pt.maxStress); // usually in MPa
                    points.push_back(s);
                    if (s > maxStress) maxStress = s;
                }
                
                ImGui::PlotLines("##Convergence", points.data(), points.size(), 0, "Max Stress vs Mesh Refinement Steps", 0.0f, maxStress * 1.1f, ImVec2(ImGui::GetContentRegionAvail().x, 150.0f));
                
                // Print a table
                ImGui::Columns(4, "ConvTable");
                ImGui::Separator();
                ImGui::Text("Elem Size"); ImGui::NextColumn();
                ImGui::Text("Nodes"); ImGui::NextColumn();
                ImGui::Text("Max Stress"); ImGui::NextColumn();
                ImGui::Text("Max Disp"); ImGui::NextColumn();
                ImGui::Separator();
                
                for (const auto& pt : convData) {
                    ImGui::Text("%.2f", pt.elementSize); ImGui::NextColumn();
                    ImGui::Text("%d", pt.numNodes); ImGui::NextColumn();
                    ImGui::Text("%.2f MPa", pt.maxStress); ImGui::NextColumn();
                    ImGui::Text("%.4f mm", pt.maxDisplacement); ImGui::NextColumn();
                }
                ImGui::Columns(1);
            } else {
                ImGui::Text("No convergence data available. Please run convergence study.");
            }
        }
"""

text = text.replace(
'''        if (ImGui::CollapsingHeader("Deformation & Reaction", ImGuiTreeNodeFlags_DefaultOpen)) {''',
new_content + '''\n        if (ImGui::CollapsingHeader("Deformation & Reaction", ImGuiTreeNodeFlags_DefaultOpen)) {'''
)

with open('/home/suyas/drawing_software/src/ui/FEMAnalysisWindow.cpp', 'w') as f:
    f.write(text)
