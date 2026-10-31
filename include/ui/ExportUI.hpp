#pragma once

// Shared ImGui export section: path field + Browse (native dialog when a
// backend exists) + one button per supported format. Each format callback
// receives a path with the proper extension appended.

#include "ui/FileDialogs.hpp"
#include <imgui.h>
#include <string>
#include <map>
#include <functional>
#include <cstring>
#include <cctype>

namespace UI {

using ExportFn = std::function<bool(const std::string& path)>;

inline bool endsWithExtCI(const std::string& s, const char* ext) {
    size_t el = std::strlen(ext), sl = s.size();
    if (sl < el) return false;
    for (size_t i = 0; i < el; ++i)
        if (std::tolower((unsigned char)s[sl - el + i]) != std::tolower((unsigned char)ext[i]))
            return false;
    return true;
}

inline std::string ensureExt(std::string p, const char* ext) {
    if (p.empty() || endsWithExtCI(p, ext)) return p;
    return p + ext;
}

// Renders "GEOMETRY EXPORT" section. Null callbacks hide their button.
// messageInOut receives user-facing status (also mirrored to return).
inline void renderExportSection(const char* id,
                                const std::string& defaultName,
                                const ExportFn& stlFn,
                                const ExportFn& objFn,
                                const ExportFn& stepFn,
                                const ExportFn& igesFn,
                                const ExportFn& inpFn,
                                std::string& messageInOut) {
    static std::map<std::string, std::string> s_paths;
    std::string& path = s_paths[id];
    if (path.empty()) path = defaultName;

    if (ImGui::CollapsingHeader("Export Geometry")) {
        char buf[512];
        snprintf(buf, sizeof(buf), "%s", path.c_str());

        bool browsed = false;
        if (fileDialogAvailable()) {
            if (ImGui::SmallButton(("Browse...##" + std::string(id)).c_str())) {
                std::string picked = saveFileDialog("Export geometry",
                                                    path,
                                                    "CAD & Mesh files",
                                                    "*.stl *.obj *.step *.igs *.inp");
                if (!picked.empty()) { path = picked; browsed = true; }
            }
            if (!browsed) ImGui::SameLine();
        }
        if (!browsed) {
            ImGui::PushItemWidth(-110);
            if (ImGui::InputText(("##expath" + std::string(id)).c_str(), buf, sizeof(buf)))
                path = buf;
            ImGui::PopItemWidth();
        }

        auto button = [&](const char* label, const char* ext, const ExportFn& fn) {
            if (!fn) return;
            if (ImGui::Button(label)) {
                std::string out = ensureExt(path, ext);
                path = out;
                bool ok = fn(out);
                messageInOut = ok ? ("Exported: " + out) : ("Export failed: " + out);
            }
            ImGui::SameLine();
        };

        button("STL",   ".stl",  stlFn);
        button("OBJ",   ".obj",  objFn);
        button("STEP",  ".step", stepFn);
        button("IGES",  ".igs",  igesFn);
        button("INP",   ".inp",  inpFn);
        ImGui::NewLine();

        if (!messageInOut.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.85f, 0.55f, 1.0f));
            ImGui::TextWrapped("%s", messageInOut.c_str());
            ImGui::PopStyleColor();
        }
        ImGui::Separator();
    }
}

} // namespace UI
