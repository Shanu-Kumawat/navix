#pragma once

// Native save-file dialog via the system GTK3 library (dlopen — no build
// dependency). Works on Linux/WSLg where libgtk-3.so.0 is installed.
// Returns "" on cancel / failure; caller falls back to the text path field.

#include <string>

namespace UI {

// Whether the GTK3 backend could be loaded and initialized
bool gtkDialogAvailable();

// Open a native save dialog. Returns true if the dialog was shown (user may
// still have cancelled — out is then empty); false if GTK is unavailable,
// so callers can fall back to another backend.
// patterns = semicolon-separated, e.g. "*.step;*.stp"
bool gtkSaveFileDialog(const std::string& title,
                       const std::string& defaultPath,
                       const std::string& filterName,
                       const std::string& patterns,
                       std::string& out);

} // namespace UI
