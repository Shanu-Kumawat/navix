#!/usr/bin/env python3
"""Inject FEM Analysis section into PropertyPanel.cpp on WSL filesystem."""

import sys

filepath = '/home/suyas/drawing_software/src/ui/PropertyPanel.cpp'

with open(filepath, 'r') as f:
    content = f.read()

# Verify the file is the old version without FEM
if 'FEM Analysis##bellows' in content:
    print('FEM section already present, skipping.')
    sys.exit(0)

lines = content.split('\n')
print(f'Original line count: {len(lines)}')

# Find insertion point: the closing brace of bellows block after PopStyleColor for Surface Meshing
# Look for the pattern: PopStyleColor() followed by } on the next line, then Ball Bearing else if
insert_idx = None
for i in range(len(lines)):
    if 'ImGui::PopStyleColor()' in lines[i] and i + 1 < len(lines):
        # Check if next line is closing brace and line after is ball bearing
        if lines[i+1].strip() == '}' and i + 2 < len(lines) and 'Ball Bearing' in lines[i+2]:
            insert_idx = i + 1  # Insert before the closing brace
            print(f'Found insertion point at line {insert_idx + 1}: "{lines[i+1].strip()}"')
            break

if insert_idx is None:
    # Fallback: search for the specific pattern
    for i in range(len(lines)):
        if 'PopStyleColor' in lines[i]:
            stripped = lines[i].strip()
            if stripped == 'ImGui::PopStyleColor();':
                if i + 1 < len(lines) and lines[i+1].strip() == '}':
                    if i + 2 < len(lines) and ('ball' in lines[i+2].lower() or 'else if' in lines[i+2]):
                        insert_idx = i + 1
                        print(f'Fallback: Found insertion point at line {insert_idx + 1}')
                        break

if insert_idx is None:
    print('ERROR: Could not find insertion point!')
    # Show context around PopStyleColor lines
    for i, l in enumerate(lines):
        if 'PopStyleColor' in l:
            print(f'  Line {i+1}: {l.rstrip()}')
    sys.exit(1)

fem_section = '''
        // ── FEM Structural Analysis section ──────────────────────────
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.15f, 0.35f, 0.7f));
        if (ImGui::CollapsingHeader("FEM Analysis##bellows", ImGuiTreeNodeFlags_DefaultOpen)) {
          if (!appContext.bellows3DViewInitialized) {
            appContext.bellowsViewer.initialize();
            appContext.bellows3DViewInitialized = true;
          }
          BellowsModel3D* model2 = appContext.bellowsViewer.getModel();
          if (model2) {
            // ── Material ──
            ImGui::PushItemWidth(availWidth);
            static int matChoice = 0;
            const char* matNames[] = { "Steel", "Stainless Steel", "Inconel 718" };
            if (ImGui::Combo("Material##fem", &matChoice, matNames, IM_ARRAYSIZE(matNames))) {
              Core::FEM::Material m;
              switch (matChoice) {
                default:
                case 0: m = Core::FEM::Material(0, "Steel",           200e9, 0.30, 7850, 250e6); break;
                case 1: m = Core::FEM::Material(1, "Stainless Steel", 193e9, 0.29, 8000, 215e6); break;
                case 2: m = Core::FEM::Material(2, "Inconel 718",     205e9, 0.29, 8190, 1034e6); break;
              }
              model2->setFEMMaterial(m);
            }
            ImGui::PopItemWidth();

            const auto& mat = model2->getFEMMaterial();
            ImGui::Text("  E = %.0f GPa, v = %.2f", mat.youngsModulus / 1e9, mat.poissonsRatio);
            ImGui::Text("  Wall: %.2f mm", bellows->wallThickness);

            // ── Load configuration ──
            ImGui::Spacing();
            static float pressureKPa = 100.0f;
            static float axialForceN = 0.0f;
            ImGui::PushItemWidth(availWidth);
            ImGui::DragFloat("Pressure (kPa)##fem", &pressureKPa, 1.0f, 0.0f, 10000.0f, "%.1f");
            ImGui::DragFloat("Axial Force (N)##fem", &axialForceN, 1.0f, -100000.0f, 100000.0f, "%.1f");
            ImGui::PopItemWidth();

            // ── Run / Clear buttons ──
            ImGui::Spacing();
            float femFullW = ImGui::GetContentRegionAvail().x;
            float femBtnW = femFullW / 2 - 4;

            bool hasMesh2 = model2->hasFEMMesh();
            if (!hasMesh2) {
              ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
              ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
            } else {
              ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.35f, 0.6f, 1.0f));
              ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.45f, 0.7f, 1.0f));
            }
            if (ImGui::Button("Run FEM##bel", ImVec2(femBtnW, 30)) && hasMesh2) {
              Core::FEM::AnalysisConfig cfg;
              cfg.pressure = static_cast<double>(pressureKPa) * 1e3;
              cfg.axialForce = static_cast<double>(axialForceN);
              model2->runFEMAnalysis(cfg);
            }
            ImGui::PopStyleColor(2);

            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.3f, 0.3f, 1.0f));
            if (ImGui::Button("Clear##fem", ImVec2(femBtnW, 30))) {
              model2->clearFEMResult();
            }
            ImGui::PopStyleColor(2);

            if (!hasMesh2) {
              ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.2f, 1.0f), "Generate mesh first");
            }

            // ── Results display ──
            if (model2->hasFEMResult()) {
              const auto& res = model2->getFEMResult();
              ImGui::Spacing();
              ImGui::SeparatorText("Results");
              ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f), "Status: %s", res.statusMessage.c_str());
              ImGui::Text("Max Disp:  %.4f mm", res.maxDisplacement * 1e3);
              double maxMPa = res.maxVonMises / 1e6;
              double yieldMPa = mat.yieldStrength / 1e6;
              if (maxMPa > yieldMPa) {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                  "Max Stress: %.1f MPa (> yield %.0f)", maxMPa, yieldMPa);
              } else {
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f),
                  "Max Stress: %.1f MPa", maxMPa);
              }
              ImGui::Text("Safety Factor: %.2f", yieldMPa / std::max(maxMPa, 1e-6));

              // Stress contour toggle
              bool showContours = model2->getShowStressContours();
              if (ImGui::Checkbox("Show Stress Contours##bel", &showContours)) {
                model2->setShowStressContours(showContours);
              }

              // Colour legend
              if (showContours) {
                ImGui::Text("%.1f MPa", res.minVonMises / 1e6);
                ImGui::SameLine();
                ImVec2 p = ImGui::GetCursorScreenPos();
                float legW = ImGui::GetContentRegionAvail().x - 60;
                ImDrawList* dl = ImGui::GetWindowDrawList();
                for (int i = 0; i < static_cast<int>(legW); ++i) {
                  float t = static_cast<float>(i) / legW;
                  ImU32 col;
                  if (t < 0.25f) { float s = t/0.25f; col = IM_COL32(0, (int)(s*255), 255, 255); }
                  else if (t < 0.5f) { float s = (t-0.25f)/0.25f; col = IM_COL32(0, 255, (int)((1-s)*255), 255); }
                  else if (t < 0.75f) { float s = (t-0.5f)/0.25f; col = IM_COL32((int)(s*255), 255, 0, 255); }
                  else { float s = (t-0.75f)/0.25f; col = IM_COL32(255, (int)((1-s)*255), 0, 255); }
                  dl->AddRectFilled(ImVec2(p.x + i, p.y), ImVec2(p.x + i + 1, p.y + 14), col);
                }
                ImGui::Dummy(ImVec2(legW, 14));
                ImGui::SameLine();
                ImGui::Text("%.1f", res.maxVonMises / 1e6);
              }
            }
          }
        }
        ImGui::PopStyleColor();
'''

lines.insert(insert_idx, fem_section)

new_content = '\n'.join(lines)
with open(filepath, 'w') as f:
    f.write(new_content)

new_count = len(lines)
print(f'New line count: {new_count}')
print('FEM section injected successfully!')
