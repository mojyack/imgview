#include <algorithm>

#include "gawl-application.hpp"
#include "gawl-window.hpp"

namespace gawl {
void GawlApplication::register_window(GawlWindow* window) {
    if(std::find(windows.begin(), windows.end(), window) == windows.end()) {
        windows.emplace_back(window);
    }
}
const std::vector<GawlWindow*>& GawlApplication::get_windows() const noexcept {
    return windows;
}
void GawlApplication::unregister_window(GawlWindow* window) {
    if(auto w = std::find(windows.begin(), windows.end(), window); w != windows.end()) {
        windows.erase(w);
    }
}
void GawlApplication::close_all_windows() {
    for(auto w : windows) {
        w->status = GawlWindow::Status::CLOSE;
        delete w;
    }
    windows.clear();
}
void GawlApplication::close_window(GawlWindow* window) {
    if(auto w = std::find(windows.begin(), windows.end(), window); w != windows.end()) {
        (*w)->status = GawlWindow::Status::CLOSE;
        tell_event(nullptr);
    }
}
} // namespace gawl
