#include <sys/eventfd.h>
#include <unistd.h>

#include "wayland-application.hpp"
#include "wayland-window.hpp"

namespace gawl {
wayland::display_t& WaylandApplication::get_display() noexcept {
    return display;
}
void WaylandApplication::tell_event(GawlWindow* window) {
    {
        std::lock_guard<std::mutex> lock(to_handle.mutex);
        to_handle.data.emplace_back(dynamic_cast<WaylandWindow*>(window));
    }
    const static size_t count = 1;
    write(fds[0].fd, &count, sizeof(count));
}
void WaylandApplication::run() {
    running = true;
    for(auto& w : get_windows()) {
        w->refresh();
    }
    while(running) {
        auto read_intent = display.obtain_read_intent();
        display.flush();
        poll(fds, 3, -1);
        if(fds[0].revents & POLLIN) {
            static size_t count;
            read(fds[0].fd, &count, sizeof(count));
            std::vector<WaylandWindow*> handle_copy;
            {
                std::lock_guard<std::mutex> lock(to_handle.mutex);
                handle_copy = std::move(to_handle.data);
            }
            for(auto w : handle_copy) {
                if(w != nullptr) {
                    w->handle_event();
                } else {
                    auto windows = get_windows();
                    for(auto w = windows.begin(); w != windows.end(); w += 1) {
                        (*w)->is_close_pending();
                        if((*w)->is_close_pending()) {
                            unregister_window(*w);
                            delete *w;
                            break;
                        }
                    }
                }
            }
        }
        if(fds[1].revents & POLLIN) {
            read_intent.read();
        }
        if(fds[2].revents & POLLIN) {
            static size_t count;
            read(fds[2].fd, &count, sizeof(count));
            break;
        }
        display.dispatch_pending();
    }
    close_all_windows();
}
void WaylandApplication::quit() {
    running                   = false;
    const static size_t count = 1;
    write(fds[2].fd, &count, sizeof(count));
}
bool WaylandApplication::is_running() const {
    return running;
}
WaylandApplication::WaylandApplication() {
    fds[0] = pollfd{eventfd(0, 0), POLLIN, 0};
    fds[1] = pollfd{display.get_fd(), POLLIN, 0};
    fds[2] = pollfd{eventfd(0, 0), POLLIN, 0};
}
} // namespace gawl
