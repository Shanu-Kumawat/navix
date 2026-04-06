import sys

filepath = '/home/suyas/drawing_software/src/ui/PropertyPanel.cpp'

with open(filepath, 'r') as f:
    lines = f.readlines()

# Find the exact insertion point: the line with res.maxVonMises / 1e6 in the legend
target_line = -1
for i, line in enumerate(lines):
    if 'res.maxVonMises / 1e6' in line and 'Text' in line and 'SeparatorText' not in line:
        if i > 900:
            target_line = i
            break

if target_line == -1:
    print('ERROR: Could not find legend text line')
    sys.exit(1)

print(f'Found legend text at line {target_line + 1}')

# Verify the closing brace pattern after this line
ok = True
for offset in [1, 2, 3, 4]:
    stripped = lines[target_line + offset].strip()
    if stripped != '}':
        print(f'Line {target_line + 1 + offset}: expected }}, got: {stripped!r}')
        ok = False

if not ok:
    print('Pattern mismatch, showing context:')
    for i in range(target_line, min(target_line + 8, len(lines))):
        print(f'{i+1}: {lines[i].rstrip()}')
    sys.exit(1)

# Check for PopStyleColor
pop_line = target_line + 5
has_pop = 'PopStyleColor' in lines[pop_line]
print(f'Line {pop_line + 1} has PopStyleColor: {has_pop}')

if has_pop:
    end_replace = pop_line + 1
else:
    end_replace = target_line + 5

print('Pattern verified. Injecting new code...')

new_code = """              }

              // ── Deformed shape overlay ──
              ImGui::Spacing();
              bool showDef = model->getShowDeformed();
              if (ImGui::Checkbox("Show Deformed Shape##bel", &showDef)) {
                model->setShowDeformed(showDef);
                if (showDef) model->setDeformScale(model->getDeformScale());
              }
              if (showDef) {
                float dScale = model->getDeformScale();
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::DragFloat("Deform Scale##fem", &dScale, 1.0f, 1.0f, 5000.0f, "%.0fx")) {
                  model->setDeformScale(dScale);
                }
                ImGui::PopItemWidth();
              }

              // ── Reaction forces ──
              if (!res.reactionForces.empty()) {
                ImGui::Spacing();
                ImGui::SeparatorText("Reaction Forces");
                double rfx = 0, rfy = 0, rfz = 0;
                for (const auto& rf : res.reactionForces) {
                  rfx += rf.force.x(); rfy += rf.force.y(); rfz += rf.force.z();
                }
                ImGui::Text("Total |R| = %.2f N", res.totalReactionMagnitude);
                ImGui::Text("  Rx = %.2f N", rfx);
                ImGui::Text("  Ry = %.2f N", rfy);
                ImGui::Text("  Rz = %.2f N", rfz);
                ImGui::Text("  (%d support nodes)", static_cast<int>(res.reactionForces.size()));
              }
            }

            // ── Modal Analysis ──
            ImGui::Spacing();
            ImGui::SeparatorText("Modal Analysis");
            static int numModes = 6;
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
            ImGui::DragInt("Num Modes##fem", &numModes, 0.1f, 1, 20);
            ImGui::PopItemWidth();

            bool canModal = model->hasFEMMesh();
            if (!canModal) ImGui::BeginDisabled();
            if (ImGui::Button("Run Modal Analysis##bel", ImVec2(femFullW, 28))) {
              model->runModalAnalysis(numModes);
            }
            if (!canModal) ImGui::EndDisabled();

            if (model->hasModalResult()) {
              const auto& mr = model->getModalResult();
              ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f), "%s", mr.statusMessage.c_str());
              if (!mr.modes.empty()) {
                int activeM = model->getActiveMode();
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
                if (ImGui::SliderInt("View Mode##fem", &activeM, 0, static_cast<int>(mr.modes.size()) - 1)) {
                  model->setActiveMode(activeM);
                }
                ImGui::PopItemWidth();
                if (ImGui::BeginTable("##modes", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit)) {
                  ImGui::TableSetupColumn("Mode", ImGuiTableColumnFlags_WidthFixed, 50);
                  ImGui::TableSetupColumn("Frequency (Hz)", ImGuiTableColumnFlags_WidthStretch);
                  ImGui::TableHeadersRow();
                  for (size_t i = 0; i < mr.modes.size(); ++i) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    if (static_cast<int>(i) == model->getActiveMode())
                      ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, 1.0f), "%d", static_cast<int>(i + 1));
                    else
                      ImGui::Text("%d", static_cast<int>(i + 1));
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.2f", mr.modes[i].frequency);
                  }
                  ImGui::EndTable();
                }
              }
            }

            // ── Mesh Convergence Study ──
            ImGui::Spacing();
            ImGui::SeparatorText("Convergence Study");
            static int convPoints = 5;
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
            ImGui::DragInt("Refinements##conv", &convPoints, 0.1f, 3, 6);
            ImGui::PopItemWidth();

            bool canConv = model->hasFEMMesh();
            if (!canConv) ImGui::BeginDisabled();
            if (ImGui::Button("Run Convergence##bel", ImVec2(femFullW, 28))) {
              Core::FEM::AnalysisConfig cfg;
              cfg.pressure = static_cast<double>(pressureKPa) * 1e3;
              cfg.axialForce = static_cast<double>(axialForceN);
              model->runConvergenceStudy(cfg, convPoints);
            }
            if (!canConv) ImGui::EndDisabled();

            if (model->hasConvergenceData()) {
              const auto& cd = model->getConvergenceData();
              if (ImGui::BeginTable("##conv", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit)) {
                ImGui::TableSetupColumn("h", ImGuiTableColumnFlags_WidthFixed, 40);
                ImGui::TableSetupColumn("Nodes", ImGuiTableColumnFlags_WidthFixed, 50);
                ImGui::TableSetupColumn("s MPa", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("u mm", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();
                for (const auto& pt : cd) {
                  ImGui::TableNextRow();
                  ImGui::TableSetColumnIndex(0); ImGui::Text("%.3f", pt.elementSize);
                  ImGui::TableSetColumnIndex(1); ImGui::Text("%d", pt.numNodes);
                  ImGui::TableSetColumnIndex(2); ImGui::Text("%.1f", pt.maxStress);
                  ImGui::TableSetColumnIndex(3); ImGui::Text("%.4f", pt.maxDisplacement);
                }
                ImGui::EndTable();
              }
            }
          }
        }
        ImGui::PopStyleColor();
"""

# Build new file
new_lines = lines[:target_line + 1]  # up to and including the legend text line
new_lines.append(new_code)
new_lines.extend(lines[end_replace:])

with open(filepath, 'w') as f:
    f.writelines(new_lines)

print('SUCCESS: FEM UI code injected')
