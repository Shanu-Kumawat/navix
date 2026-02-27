import re

def replace_line(file, line_num, new_text):
    with open(file, 'r') as f:
        lines = f.readlines()
    if 0 <= line_num - 1 < len(lines):
        lines[line_num - 1] = new_text + '\n'
        with open(file, 'w') as f:
            f.writelines(lines)

# src/shapes/Dimension.cpp
# 30: drawList->AddLine(Drawing::Math::toImVec2(Drawing::Math::toImVec2(arrowP1)), Drawing::Math::toImVec2(Drawing::Math::toImVec2(end)), color, thickness);
replace_line('src/shapes/Dimension.cpp', 30, '    drawList->AddLine(Drawing::Math::toImVec2(arrowP1), Drawing::Math::toImVec2(end), color, thickness);')
replace_line('src/shapes/Dimension.cpp', 31, '    drawList->AddLine(Drawing::Math::toImVec2(arrowP2), Drawing::Math::toImVec2(end), color, thickness);')
replace_line('src/shapes/Dimension.cpp', 43, '    glm::dvec2 textPos = center - glm::dvec2(textSize.x / 2.0f, textSize.y / 2.0f);')
replace_line('src/shapes/Dimension.cpp', 59, '    glm::dvec2 textPos = center - glm::dvec2(textSize.x / 2.0f, textSize.y / 2.0f);')
replace_line('src/shapes/Dimension.cpp', 98, '    glm::dvec2 textSize = Drawing::Math::toDVec2(ImGui::CalcTextSize(text.c_str()));')

# src/shapes/ComplexShapes.cpp
replace_line('src/shapes/ComplexShapes.cpp', 467, '        drawList->AddLine(Drawing::Math::toImVec2(points[i - 1]), Drawing::Math::toImVec2(points[i]), color, 1.0f);')
replace_line('src/shapes/ComplexShapes.cpp', 473, '    drawList->AddLine(Drawing::Math::toImVec2(point), Drawing::Math::toImVec2(tangent), color, 1.0f);')
replace_line('src/shapes/ComplexShapes.cpp', 474, '    drawList->AddCircleFilled(Drawing::Math::toImVec2(tangent), 3.0f, color);')

# src/shapes/ShockAbsorberBottomEnd.cpp
replace_line('src/shapes/ShockAbsorberBottomEnd.cpp', 69, '    drawList->AddConvexPolyFilled(tpIm.data(), tpIm.size(), color);\n}')
# Wait, for ShockAbsorberBottomEnd, I already used `{\nstd::vector...}`. I will just revert and fix properly.
