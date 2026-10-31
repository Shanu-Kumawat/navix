#pragma once

// Native save-file dialog helper built on portable-file-dialogs (header-only).
// On Linux it uses zenity; if no backend is available every call degrades
// gracefully (returns "") so callers can fall back to a text path field.
//
// To enable native dialogs on Ubuntu/WSL: sudo apt install -y zenity

#include <string>
#include "ui/GtkFileDialogs.hpp"

#if defined(__has_include)
#  if __has_include(<pfd/portable-file-dialogs.h>)
#    include <pfd/portable-file-dialogs.h>
#    define NAVIX_HAS_PFD 1
#  endif
#endif

namespace UI {

// True when a native dialog backend is available (Browse button worth showing)
inline bool fileDialogAvailable() {
    if (gtkDialogAvailable()) return true;
#ifdef NAVIX_HAS_PFD
    return pfd::settings::available();
#else
    return false;
#endif
}

// Open a save dialog. Returns the chosen path (may lack the extension —
// callers append their own), or "" on cancel / no backend.
// Backend order: system GTK3 (dlopen), then portable-file-dialogs (zenity).
inline std::string saveFileDialog(const std::string& title,
                                  const std::string& defaultPath,
                                  const std::string& filterName,
                                  const std::string& filterPattern) {
    // 1. Native GTK3 chooser (system libgtk-3, no extra packages needed)
    std::string gtk;
    if (gtkSaveFileDialog(title, defaultPath, filterName, filterPattern, gtk))
        return gtk;  // dialog shown (accept or cancel) — don't fall through

#ifdef NAVIX_HAS_PFD
    // 2. zenity/kdialog via portable-file-dialogs (if installed)
    try {
        pfd::save_file dlg(title, defaultPath,
                           { filterName, filterPattern, "All Files", "*" },
                           pfd::opt::force_overwrite);
        std::string r = dlg.result();
        if (!r.empty()) return r;
    } catch (...) {}
#endif

    return "";
}

} // namespace UI
