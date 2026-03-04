#pragma once
#include <glm/glm.hpp>
#include <imgui.h>

namespace Drawing {
namespace Math {
    using DVec2 = glm::dvec2;
    using DVec3 = glm::dvec3;

    inline ImVec2 toImVec2(const glm::dvec2& v) { return ImVec2((float)v.x, (float)v.y); }
    inline glm::dvec2 toDVec2(const ImVec2& v) { return glm::dvec2((double)v.x, (double)v.y); }
    inline double cross(const glm::dvec2& a, const glm::dvec2& b) { return a.x * b.y - a.y * b.x; }
}}
