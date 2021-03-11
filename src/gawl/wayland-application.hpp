#pragma once
#include <poll.h>
#include <wayland-client.hpp>

#include "gawl-application.hpp"
#include "type.hpp"

namespace gawl {
class WaylandWindow;

class WaylandApplication : public GawlApplication {
  private:
    wayland::display_t display;
    pollfd             fds[3];
    bool               running = false;

    SafeVar<std::vector<WaylandWindow*>> to_handle;

  public:
    wayland::display_t& get_display() noexcept;

    void tell_event(GawlWindow* window) final;
    void run() final;
    void quit() final;
    bool is_running() const final;

    WaylandApplication();
    virtual ~WaylandApplication(){};
};
} // namespace gawl
