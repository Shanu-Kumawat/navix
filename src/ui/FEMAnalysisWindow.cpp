#include "ui/FEMAnalysisWindow.hpp"
#include <imgui.h>
#include "BellowsModel3D.hpp"
#include "fem/FEMTypes.hpp"
#include "meshing/Mesh.hpp"
#include <vector>
#include <algorithm>
#include <cstdio>


namespace UI {

static void DrawColorScaleBar(float width, float height, float minVal, float maxVal) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 p_min = ImGui::GetCursorScreenPos();
    ImVec2 p_max = ImVec2(p_min.x + width, p_min.y + height);

    // Dark blue to Red color map for heat map
    const ImU32 col_left  = IM_COL32(0, 0, 255, 255);
    const ImU32 col_mid1  = IM_COL32(0, 255, 255, 255);
    const ImU32 col_mid2  = IM_COL32(255, 255, 0, 255);
    const ImU32 col_right = IM_COL32(255, 0, 0, 255);

    float w3 = width / 3.0f;
    draw_list->AddRectFilledMultiColor(p_min, ImVec2(p_min.x + w3, p_max.y), col_left, col_mid1, col_mid1, col_left);
    draw_list->AddRectFilledMultiColor(ImVec2(p_min.x + w3, p_min.y), ImVec2(p_min.x + 2*w3, p_max.y), col_mid1, col_mid2, col_mid2, col_mid1);
    draw_list->AddRectFilledMultiColor(ImVec2(p_min.x + 2*w3, p_min.y), p_max, col_mid2, col_right, col_right, col_mid2);

    ImGui::Dummy(ImVec2(width, height));

    char minText[32], maxText[32], midText[32];
    snprintf(minText, sizeof(minText), "%.1f", minVal);
    snprintf(maxText, sizeof(maxText), "%.1f", maxVal);
    snprintf(midText, sizeof(midText), "%.1f", (minVal + maxVal)*0.5f);

    float maxTextW = ImGui::CalcTextSize(maxText).x;
    float midTextW = ImGui::CalcTextSize(midText).x;

    ImGui::TextUnformatted(minText);
    ImGui::SameLine(width/2.0f - midTextW/2.0f);
    ImGui::TextUnformatted(midText);
    ImGui::SameLine(width - maxTextW);
    ImGui::TextUnformatted(maxText);
}

void FEMAnalysisWindow::Render(Core::ApplicationContext& appContext) {
    if (!appContext.showFEMAnalysisView) return;

    ImGui::SetNextWindowSize(ImVec2(550, 650), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Professional FEM Dashboard", &appContext.showFEMAnalysisView)) {
        BellowsModel3D* bellowsModel = nullptr;
        if (appContext.bellows3DViewInitialized) {
            bellowsModel = appContext.bellowsViewer.getModel();
        }

        if (!bellowsModel || !bellowsModel->hasFEMResult()) {
            ImGui::TextDisabled("No active FEM results to display. Run an analysis first.");
            ImGui::End();
            return;
        }

        const auto& result = bellowsModel->getFEMResult();
        const auto& mat = bellowsModel->getFEMMaterial();

        // Top Status Header
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
        ImGui::BeginChild("StatusChild", ImVec2(0, 40), true);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "  STATUS: %s", result.statusMessage.c_str());
        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::Spacing();

        // Heat Maps and Graphs Section
        if (ImGui::CollapsingHeader("Stress Field Heat Map", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Spacing();
            ImGui::Text("Von Mises Stress Distribution (MPa)");
            DrawColorScaleBar(ImGui::GetContentRegionAvail().x - 20, 25.0f, result.minVonMises / 1e6, result.maxVonMises / 1e6);
            ImGui::Spacing();
        }

        if (ImGui::CollapsingHeader("Stress Histogram", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (!result.elementStresses.empty()) {
                const int numBins = 40;
                std::vector<float> bins(numBins, 0.0f);
                double range = (result.maxVonMises - result.minVonMises);
                if (range < 1e-6) range = 1.0;
                
                for (const auto& es : result.elementStresses) {
                    double normalized = (es.vonMisesMax - result.minVonMises) / range;
                    int binIdx = static_cast<int>(normalized * numBins);
                    if (binIdx >= numBins) binIdx = numBins - 1;
                    if (binIdx < 0) binIdx = 0;
                    bins[binIdx] += 1.0f;
                }
                
                float maxCount = 0.0f;
                for (float v : bins) if (v > maxCount) maxCount = v;
                
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_PlotHistogramHovered, ImVec4(0.3f, 0.7f, 1.0f, 1.0f));
                ImGui::PlotHistogram("##stresses", bins.data(), numBins, 0, "Element Stress Distribution", 0.0f, maxCount, ImVec2(ImGui::GetContentRegionAvail().x, 150.0f));
                ImGui::PopStyleColor(2);
            }
        }


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

        if (ImGui::CollapsingHeader("Deformation & Reaction", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Columns(2, "AnalysisColumns");
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.2f, 1.0f), "Key Metrics");
            ImGui::NextColumn();
            ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.2f, 1.0f), "Values");
            ImGui::NextColumn();
            ImGui::Separator();
            
            ImGui::Text("Max Von Mises:"); ImGui::NextColumn(); 
            ImGui::Text("%.2f MPa", result.maxVonMises / 1e6); ImGui::NextColumn();

            ImGui::Text("Max Displacement:"); ImGui::NextColumn(); 
            ImGui::Text("%.4f mm", result.maxDisplacement * 1e3); ImGui::NextColumn();

            double sf = mat.yieldStrength / result.maxVonMises;
            ImGui::Text("Safety Factor (Yield):"); ImGui::NextColumn(); 
            ImVec4 sfCol = sf < 1.0 ? ImVec4(1,0,0,1) : (sf < 2.0 ? ImVec4(1,0.5f,0,1) : ImVec4(0,1,0,1));
            ImGui::TextColored(sfCol, "%.2f", sf); ImGui::NextColumn();

            ImGui::Text("Material Yield Str:"); ImGui::NextColumn(); 
            ImGui::Text("%.2f MPa", mat.yieldStrength / 1e6); ImGui::NextColumn();

            ImGui::Text("Total Reaction Force:"); ImGui::NextColumn(); 
            ImGui::Text("%.2f N", result.totalReactionMagnitude); ImGui::NextColumn();

            ImGui::Columns(1);
            ImGui::Separator();
        }
    }
    ImGui::End();
}

} // namespace UI
