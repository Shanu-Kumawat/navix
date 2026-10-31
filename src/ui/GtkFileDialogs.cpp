#include "ui/GtkFileDialogs.hpp"

#ifndef _WIN32
#include <dlfcn.h>
#include <iostream>
#include <mutex>

namespace UI {

namespace {

// Everything is resolved via dlsym so we keep no build/link dependency on GTK.
// All GTK handles are void* to stay ABI-neutral.

void* g_gtkLib = nullptr;
bool  g_initTried = false;
bool  g_initOk = false;
std::mutex g_mutex;

bool loadGtk() {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_initTried) return g_initOk;
    g_initTried = true;

    g_gtkLib = dlopen("libgtk-3.so.0", RTLD_LAZY | RTLD_LOCAL);
    if (!g_gtkLib) {
        std::cerr << "[FileDialogs] libgtk-3.so.0 not loadable: " << dlerror() << std::endl;
        return false;
    }

    using InitFn = int (*)(int*, char***);
    auto gtk_init_check = reinterpret_cast<InitFn>(dlsym(g_gtkLib, "gtk_init_check"));
    if (!gtk_init_check) return false;

    int argc = 0;
    char** argv = nullptr;
    g_initOk = gtk_init_check(&argc, &argv) != 0;
    if (!g_initOk) {
        std::cerr << "[FileDialogs] gtk_init_check failed (no display?)" << std::endl;
        g_initOk = false; // retryable next call if display appears later
        g_initTried = false;
    }
    return g_initOk;
}

} // anonymous namespace

bool gtkDialogAvailable() {
    return loadGtk();
}

bool gtkSaveFileDialog(const std::string& title,
                       const std::string& defaultPath,
                       const std::string& filterName,
                       const std::string& patterns,
                       std::string& out) {
    out.clear();
    if (!loadGtk()) return false;

    // Resolve symbols
    using FileChooserNewFn = void* (*)(const char*, void*, int, ...);
    using DialogRunFn      = int (*)(void*);
    using WidgetDestroyFn  = void (*)(void*);
    using SetFilenameFn    = void (*)(void*, const char*);
    using GetFilenameFn    = char* (*)(void*);
    using SetOverwriteFn   = void (*)(void*, int);
    using FilterNewFn      = void* (*)();
    using FilterSetNameFn  = void (*)(void*, const char*);
    using FilterAddPatternFn = void (*)(void*, const char*);
    using AddFilterFn      = void (*)(void*, void*);
    using UnrefFn          = void (*)(void*);

    auto chooserNew = reinterpret_cast<FileChooserNewFn>(dlsym(g_gtkLib, "gtk_file_chooser_dialog_new"));
    auto dialogRun  = reinterpret_cast<DialogRunFn>(dlsym(g_gtkLib, "gtk_dialog_run"));
    auto widgetDestroy = reinterpret_cast<WidgetDestroyFn>(dlsym(g_gtkLib, "gtk_widget_destroy"));
    auto setFilename = reinterpret_cast<SetFilenameFn>(dlsym(g_gtkLib, "gtk_file_chooser_set_filename"));
    auto getFilename = reinterpret_cast<GetFilenameFn>(dlsym(g_gtkLib, "gtk_file_chooser_get_filename"));
    auto setOverwrite = reinterpret_cast<SetOverwriteFn>(dlsym(g_gtkLib, "gtk_file_chooser_set_do_overwrite_confirmation"));
    auto filterNew = reinterpret_cast<FilterNewFn>(dlsym(g_gtkLib, "gtk_file_filter_new"));
    auto filterSetName = reinterpret_cast<FilterSetNameFn>(dlsym(g_gtkLib, "gtk_file_filter_set_name"));
    auto filterAddPattern = reinterpret_cast<FilterAddPatternFn>(dlsym(g_gtkLib, "gtk_file_filter_add_pattern"));
    auto chooserAddFilter = reinterpret_cast<AddFilterFn>(dlsym(g_gtkLib, "gtk_file_chooser_add_filter"));
    auto g_free = reinterpret_cast<UnrefFn>(dlsym(g_gtkLib, "g_free"));
    if (!chooserNew || !dialogRun || !widgetDestroy || !getFilename || !filterNew) return "";

    std::lock_guard<std::mutex> lock(g_mutex);

    // GTK_FILE_CHOOSER_ACTION_SAVE = 1, GTK_RESPONSE_ACCEPT = -3
    void* dialog = chooserNew(title.c_str(), nullptr, 1,
                              "_Cancel", -6 /*GTK_RESPONSE_CANCEL*/,
                              "_Save", -3 /*GTK_RESPONSE_ACCEPT*/,
                              static_cast<const char*>(nullptr));
    if (!dialog) return false;

    if (setOverwrite) setOverwrite(dialog, 1);

    // Filter: name + semicolon-separated patterns
    if (filterNew && filterAddPattern && !patterns.empty()) {
        void* filter = filterNew();
        if (filterSetName) filterSetName(filter, filterName.c_str());
        std::string pat = patterns;
        size_t start = 0;
        while (start < pat.size()) {
            size_t semi = pat.find(';', start);
            std::string one = pat.substr(start, semi == std::string::npos ? std::string::npos : semi - start);
            if (!one.empty()) filterAddPattern(filter, one.c_str());
            if (semi == std::string::npos) break;
            start = semi + 1;
        }
        if (chooserAddFilter) chooserAddFilter(dialog, filter);
    }

    if (setFilename && !defaultPath.empty()) setFilename(dialog, defaultPath.c_str());

    int response = dialogRun(dialog);
    if (response == -3 /*ACCEPT*/) {
        char* chosen = getFilename(dialog);
        if (chosen) {
            out = chosen;
            if (g_free) g_free(chosen);
        }
    }
    widgetDestroy(dialog);
    return true;  // dialog was shown (accepted or cancelled)
}

} // namespace UI

#else // _WIN32 — no GTK; native dialogs unavailable (pfd handles Windows)

namespace UI {
bool gtkDialogAvailable() { return false; }
bool gtkSaveFileDialog(const std::string&, const std::string&,
                       const std::string&, const std::string&, std::string& out) {
    out.clear();
    return false;
}
} // namespace UI

#endif // _WIN32
