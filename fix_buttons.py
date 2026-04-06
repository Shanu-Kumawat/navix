import sys

filepath = '/home/suyas/drawing_software/src/ui/PropertyPanel.cpp'
with open(filepath, 'r') as f:
    lines = f.readlines()

new_lines = []
i = 0
while i < len(lines):
    line = lines[i]

    # Find: Run Modal Analysis button line
    if 'Run Modal Analysis##bel' in line and 'femFullW' in line:
        # Change femFullW to femBtnW
        new_lines.append(line.replace('femFullW', 'femBtnW'))
        i += 1
        # Copy until we find EndDisabled
        while i < len(lines):
            new_lines.append(lines[i])
            if 'EndDisabled' in lines[i] and 'canModal' in lines[i]:
                i += 1
                # Insert Clear button after EndDisabled
                new_lines.append('\n')
                new_lines.append('            ImGui::SameLine();\n')
                new_lines.append('            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.2f, 0.2f, 1.0f));\n')
                new_lines.append('            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.3f, 0.3f, 1.0f));\n')
                new_lines.append('            if (ImGui::Button("Clear##modal", ImVec2(femBtnW, 28))) {\n')
                new_lines.append('              model2->clearModalResult();\n')
                new_lines.append('            }\n')
                new_lines.append('            ImGui::PopStyleColor(2);\n')
                break
            i += 1
        i += 1
        continue

    # Find: Run Convergence button line
    if 'Run Convergence##bel' in line and 'femFullW' in line:
        # Change femFullW to femBtnW
        new_lines.append(line.replace('femFullW', 'femBtnW'))
        i += 1
        # Copy until we find EndDisabled
        while i < len(lines):
            new_lines.append(lines[i])
            if 'EndDisabled' in lines[i] and 'canConv' in lines[i]:
                i += 1
                # Insert Clear button after EndDisabled
                new_lines.append('\n')
                new_lines.append('            ImGui::SameLine();\n')
                new_lines.append('            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.2f, 0.2f, 1.0f));\n')
                new_lines.append('            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.3f, 0.3f, 1.0f));\n')
                new_lines.append('            if (ImGui::Button("Clear##conv", ImVec2(femBtnW, 28))) {\n')
                new_lines.append('              model2->clearConvergenceData();\n')
                new_lines.append('            }\n')
                new_lines.append('            ImGui::PopStyleColor(2);\n')
                break
            i += 1
        i += 1
        continue

    new_lines.append(line)
    i += 1

with open(filepath, 'w') as f:
    f.writelines(new_lines)

print('Patched successfully')
