#pragma once
#include <map>
#include <thread>

#include <wayland-client-core.h>
#include <wayland-client-protocol-extra.hpp>
#include <wayland-client-protocol.hpp>
#include <wayland-cursor.hpp>
#include <wayland-egl.hpp>

#include "gawl-window.hpp"

namespace gawl {
class WaylandWindow : public GawlWindow {
  friend class WaylandApplication;

  private:
    // global objects
    wayland::display_t&    display;
    wayland::registry_t    registry;
    wayland::compositor_t  compositor;
    wayland::xdg_wm_base_t xdg_wm_base;
    wayland::seat_t        seat;
    wayland::shm_t         shm;
    wayland::output_t      output;

    // local objects
    wayland::surface_t       surface;
    wayland::shell_surface_t shell_surface;
    wayland::xdg_surface_t   xdg_surface;
    wayland::xdg_toplevel_t  xdg_toplevel;
    wayland::pointer_t       pointer;
    wayland::keyboard_t      keyboard;
    wayland::callback_t      frame_cb;
    wayland::cursor_image_t  cursor_image;
    wayland::buffer_t        cursor_buffer;
    wayland::surface_t       cursor_surface;

    // EGL
    wayland::egl_window_t egl_window;
    EGLSurface            eglsurface = nullptr;

    std::thread::id     main_thread_id;
    bool                frame_ready = true;
    SafeVar<bool>       current_frame = true;
    bool                has_pointer;
    bool                has_keyboard;
    int                 window_size[2] = {0, 0};
    int                 buffer_scale   = 0;
    ConditionalVariable key_delay_timer;
    std::thread         key_repeater;
    SafeVar<uint32_t>   last_pressed_key     = -1;
    bool                is_repeat_info_valid = false;
    uint32_t            repeat_interval      = -1;
    uint32_t            delay_in_milisec     = -1;
    SafeVar<uint32_t>   key_repeated         = 0;
    std::map<int, bool> keypress_info; // first=linux-syscall-code second=pressed?
    SafeVar<bool>       do_refresh = false;

    void init_egl();
    void resize_buffer(int width, int height, int scale);
    void handle_event();
    void swap_buffer();
    void choose_surface();
    void wait_for_key_repeater_exit();

  public:
    void refresh() final;

    WaylandWindow(GawlApplication& app, int initial_window_width = 800, int initial_window_height = 600);
    virtual ~WaylandWindow();
};
} // namespace gawl
