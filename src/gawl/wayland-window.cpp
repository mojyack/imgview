#include <iostream>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

#include "wayland-application.hpp"
#include "wayland-window.hpp"

namespace gawl {
namespace {
int        egl_count          = 0;
EGLSurface current_eglsurface = nullptr;
EGLDisplay egldisplay         = nullptr;
EGLConfig  eglconfig          = nullptr;
EGLContext eglcontext         = nullptr;
} // namespace
void WaylandWindow::init_egl() {
    if(egl_count == 0) {
        egldisplay = eglGetDisplay(display);
        if(egldisplay == EGL_NO_DISPLAY) {
            throw std::runtime_error("eglGetDisplay");
        }

        EGLint major = 0;
        EGLint minor = 0;
        if(eglInitialize(egldisplay, &major, &minor) == EGL_FALSE) {
            throw std::runtime_error("eglInitialize");
        }
        if(!((major == 1 && minor >= 4) || major >= 2)) {
            throw std::runtime_error("EGL version too old");
        }

        if(eglBindAPI(EGL_OPENGL_API) == EGL_FALSE) {
            throw std::runtime_error("eglBindAPI");
        }

        std::array<EGLint, 13> config_attribs = {{EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                                                  EGL_RED_SIZE, 8,
                                                  EGL_GREEN_SIZE, 8,
                                                  EGL_BLUE_SIZE, 8,
                                                  EGL_ALPHA_SIZE, 8,
                                                  EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
                                                  EGL_NONE}};

        EGLint num = 0;
        if(eglChooseConfig(egldisplay, config_attribs.data(), &eglconfig, 1, &num) == EGL_FALSE || num == 0) {
            throw std::runtime_error("eglChooseConfig");
        }

        std::array<EGLint, 3> context_attribs = {{EGL_CONTEXT_CLIENT_VERSION, 2,
                                                  EGL_NONE}};

        eglcontext = eglCreateContext(egldisplay, eglconfig, EGL_NO_CONTEXT, context_attribs.data());
        if(eglcontext == EGL_NO_CONTEXT) {
            throw std::runtime_error("eglCreateContext");
        }
    }
    eglsurface = eglCreateWindowSurface(egldisplay, eglconfig, egl_window, nullptr);
    if(eglsurface == EGL_NO_SURFACE) {
        throw std::runtime_error("eglCreateWindowSurface");
    }
    choose_surface();
    egl_count += 1;
}
void WaylandWindow::resize_buffer(int width, int height, int scale) {
    if((width != -1 && height != -1 && width == window_size[0] && height == window_size[1]) || (scale != -1 && scale == buffer_scale)) {
        return;
    }
    if(scale != -1 && scale != buffer_scale) {
        buffer_scale = scale;
        surface.set_buffer_scale(buffer_scale);

        // load cursor theme
        wayland::cursor_theme_t cursor_theme = wayland::cursor_theme_t("default", 24 * buffer_scale, shm);
        wayland::cursor_t       cursor       = cursor_theme.get_cursor("left_ptr");
        cursor_image                         = cursor.image(0);
        cursor_buffer                        = cursor_image.get_buffer();
        cursor_surface.set_buffer_scale(buffer_scale);
    }
    if(width != -1 && height != -1) {
        window_size[0] = width;
        window_size[1] = height;
    }
    // apply new scale
    int bw = window_size[0] * buffer_scale;
    int bh = window_size[1] * buffer_scale;
    if(is_running()) {
        choose_surface();
        egl_window.resize(bw, bh);
        glViewport(0, 0, bw, bh);
    }
    on_buffer_resize(bw, bh, buffer_scale);
    window_resize_callback();
    app.tell_event(this);
}
void WaylandWindow::handle_event() {
    uint32_t repeated = key_repeated.load();
    key_repeated.store(0);
    if(repeated != 0) {
        auto key = last_pressed_key.load();
        for(uint32_t i = 0; i < repeated; ++i) {
            keyboard_callback(key, ButtonState::repeat);
        }
    }
    if(do_refresh.load()) {
        refresh();
    }
}
void WaylandWindow::refresh() {
    if(!is_running()) {
        return;
    }
    if(std::this_thread::get_id() != main_thread_id) {
        do_refresh.store(true);
        app.tell_event(this);
        return;
    }
    if(!frame_ready) {
        current_frame.store(false);
        return;
    }
    do_refresh.store(false);
    frame_ready = false;
    choose_surface();
    refresh_callback();

    frame_cb           = surface.frame();
    frame_cb.on_done() = [&](uint32_t /* elapsed */) {
        frame_ready = true;
        bool current;
        {
            std::lock_guard<std::mutex> lock(current_frame.mutex);
            current = current_frame.data;
            current_frame.data = true;
        }
        if(!get_event_driven() || !current) {
            refresh();
        }
    };
    swap_buffer();
    app.tell_event(this);
}
void WaylandWindow::swap_buffer() {
    if(eglSwapBuffers(egldisplay, eglsurface) == EGL_FALSE) {
        throw std::runtime_error("eglSwapBuffers");
    }
}
void WaylandWindow::choose_surface() {
    if(current_eglsurface == eglsurface) {
        return;
    }
    if(eglMakeCurrent(egldisplay, eglsurface, eglsurface, eglcontext) == EGL_FALSE) {
        throw std::runtime_error("eglMakeCurrent");
    }
    current_eglsurface = surface;
}
void WaylandWindow::wait_for_key_repeater_exit() {
    last_pressed_key.store(-1);
    key_delay_timer.wakeup();
    if(key_repeater.joinable()) {
        key_repeater.join();
    }
}
WaylandWindow::WaylandWindow(GawlApplication& app, int initial_window_width, int initial_window_height)
    : GawlWindow(app), display(dynamic_cast<WaylandApplication*>(&app)->get_display()) {
    // retrieve global objects
    registry             = display.get_registry();
    registry.on_global() = [&](uint32_t name, const std::string& interface, uint32_t version) {
        if(interface == wayland::compositor_t::interface_name)
            registry.bind(name, compositor, version);
        else if(interface == wayland::xdg_wm_base_t::interface_name)
            registry.bind(name, xdg_wm_base, version);
        else if(interface == wayland::seat_t::interface_name)
            registry.bind(name, seat, version);
        else if(interface == wayland::shm_t::interface_name)
            registry.bind(name, shm, version);
        else if(interface == wayland::output_t::interface_name)
            registry.bind(name, output, version);
    };
    display.roundtrip();
    if(!xdg_wm_base) {
        throw std::runtime_error("xdg_wm_base not supported.");
    }
    output.on_scale() = [&](int32_t s) {
        resize_buffer(-1, -1, s);
    };
    seat.on_capabilities() = [&](const wayland::seat_capability& capability) {
        has_keyboard = capability & wayland::seat_capability::keyboard;
        has_pointer  = capability & wayland::seat_capability::pointer;
    };

    // create a surface
    surface                    = compositor.create_surface();
    xdg_wm_base.on_ping()      = [&](uint32_t serial) { xdg_wm_base.pong(serial); };
    xdg_surface                = xdg_wm_base.get_xdg_surface(surface);
    xdg_surface.on_configure() = [&](uint32_t serial) { xdg_surface.ack_configure(serial); };
    xdg_toplevel               = xdg_surface.get_toplevel();
    xdg_toplevel.set_title("Window");
    xdg_toplevel.on_close() = [&]() { close_request_callback(); };

    // create cursor surface
    cursor_surface = compositor.create_surface();

    surface.commit();
    display.roundtrip();

    // Get input devices
    if(!has_keyboard)
        throw std::runtime_error("No keyboard found.");
    if(!has_pointer)
        throw std::runtime_error("No pointer found.");
    pointer  = seat.get_pointer();
    keyboard = seat.get_keyboard();

    // draw cursor
    pointer.on_enter() = [&](uint32_t serial, const wayland::surface_t& /*unused*/, int32_t /*unused*/, int32_t /*unused*/) {
        cursor_surface.attach(cursor_buffer, 0, 0);
        cursor_surface.damage(0, 0, cursor_image.width() * buffer_scale, cursor_image.height() * buffer_scale);
        cursor_surface.commit();
        pointer.set_cursor(serial, cursor_surface, 0, 0);
    };

    // intitialize egl
    egl_window = wayland::egl_window_t(surface, initial_window_width, initial_window_height);
    init_egl();

    // other configuration
    xdg_toplevel.on_configure() = [&](int32_t w, int32_t h, wayland::array_t /*s*/) {
        resize_buffer(w, h, -1);
    };
    keyboard.on_key()         = [this](uint32_t /*unused*/, uint32_t /*unused*/, uint32_t key, wayland::keyboard_key_state state) {
        gawl::ButtonState s = state == wayland::keyboard_key_state::pressed ? gawl::ButtonState::press : gawl::ButtonState::release;
        keyboard_callback(key, s);
        if(!is_repeat_info_valid) return;
        if(s == gawl::ButtonState::press) {
            wait_for_key_repeater_exit();
            last_pressed_key.store(key);
            key_repeater                   = std::thread([this, key]() {
                key_delay_timer.wait_for(std::chrono::milliseconds(delay_in_milisec));
                while(last_pressed_key.load() == key) {
                    key_repeated.store(key_repeated.load() + 1);
                    this->app.tell_event(this);
                    key_delay_timer.wait_for(std::chrono::milliseconds(repeat_interval));
                }
            });
        } else if(last_pressed_key.load() == key){
            wait_for_key_repeater_exit();
        } };
    keyboard.on_repeat_info() = [&](uint32_t repeat_per_sec, uint32_t delay_in_milisec) {
        this->repeat_interval  = 1000 / repeat_per_sec;
        this->delay_in_milisec = delay_in_milisec;
        is_repeat_info_valid   = true;
    };
    keyboard.on_leave() = [this](uint32_t /* serial */, wayland::surface_t /* surface */) {
        wait_for_key_repeater_exit();
        keyboard_callback(-1, ButtonState::leave);
    };
    pointer.on_button() = [&](uint32_t /*serial*/, uint32_t /*time*/, uint32_t button, wayland::pointer_button_state state) {
        gawl::ButtonState s = state == wayland::pointer_button_state::pressed ? gawl::ButtonState::press : gawl::ButtonState::release;
        click_callback(button, s);
    };
    pointer.on_motion() = [&](uint32_t, double x, double y) {
        pointermove_callback(x, y);
    };
    pointer.on_axis() = [&](uint32_t, wayland::pointer_axis axis, double value) {
        gawl::WheelAxis w = axis == wayland::pointer_axis::horizontal_scroll ? gawl::WheelAxis::horizontal : gawl::WheelAxis::vertical;
        scroll_callback(w, value);
    };
    main_thread_id = std::this_thread::get_id();

    init_global();
    init_complete();
}
WaylandWindow::~WaylandWindow() {
    wait_for_key_repeater_exit();
    // finialize EGL.
    if(eglDestroySurface(egldisplay, eglsurface) == EGL_FALSE) {
        std::cerr << "eglDestroyContext failed." << std::endl;
    }
    if(egl_count == 1) {
        if(eglDestroyContext(egldisplay, eglcontext) == EGL_FALSE) {
            std::cerr << "eglDestroyContext failed." << std::endl;
        }

        if(eglTerminate(egldisplay) == EGL_FALSE) {
            std::cerr << "eglTerminate failed." << std::endl;
        }
    }
    egl_count -= 1;
}
} // namespace gawl
