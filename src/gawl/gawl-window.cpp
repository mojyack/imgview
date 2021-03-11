#include "gawl-window.hpp"
#include "gawl-application.hpp"
#include "global.hpp"
#include "graphic-base.hpp"
#include "shader-source.hpp"

namespace gawl {
GlobalVar*              global;
static int              global_count = 0;
static constexpr double MIN_SCALE = 0.01;

void GawlWindow::on_buffer_resize(const size_t width, const size_t height, const size_t scale) {
    buffer_size = BufferSize{{width, height}, scale};
    draw_scale  = specified_scale >= MIN_SCALE ? specified_scale : follow_buffer_scale ? buffer_size.scale
                                                                                       : 1;
    if(width != 0 || height != 0) {
        window_size[0] = width / draw_scale;
        window_size[1] = height / draw_scale;
    }
    window_resize_callback();
    if(app.is_running()) {
        refresh();
    }
}
bool GawlWindow::is_running() const noexcept {
    return status == Status::READY;
}
void GawlWindow::init_global() {
    if(global_count == 0) {
        init_graphics();
        global = new GlobalVar();
    }
    global_count += 1;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}
void GawlWindow::init_complete() {
    status = Status::READY;
}
const std::array<int, 2>& GawlWindow::get_window_size() const noexcept {
    return window_size;
}
void GawlWindow::set_follow_buffer_scale(const bool flag) {
    if(flag == follow_buffer_scale) {
        return;
    }
    follow_buffer_scale = flag;
    on_buffer_resize(0, 0, buffer_size.scale);
}
bool GawlWindow::get_follow_buffer_scale() const noexcept {
    return follow_buffer_scale;
}
void GawlWindow::set_scale(const double scale) {
    specified_scale = scale;
    on_buffer_resize(0, 0, buffer_size.scale);
}
void GawlWindow::set_event_driven(const bool flag) {
    if(event_driven == flag) {
        return;
    }
    if(event_driven) {
        event_driven = false;
    } else {
        event_driven = true;
        if(app.is_running()) {
            refresh();
        }
    }
}
bool GawlWindow::get_event_driven() const noexcept {
    return event_driven;
}
void GawlWindow::close_request_callback() {
    quit_application();
}
bool GawlWindow::is_close_pending() const noexcept {
    return status == Status::CLOSE;
}
double GawlWindow::get_scale() const noexcept {
    return draw_scale;
}
const BufferSize& GawlWindow::get_buffer_size() const noexcept {
    return buffer_size;
}
void GawlWindow::close_window() {
    app.close_window(this);
}
void GawlWindow::quit_application() {
    app.quit();
}
GawlWindow::GawlWindow(GawlApplication& app) : app(app) {}
GawlWindow::~GawlWindow() {
    if(global_count == 1) {
        delete global;
        finish_graphics();
    }
    global_count -= 1;
}
GlobalVar::GlobalVar() {
    FT_Init_FreeType(&freetype);
    graphic_shader    = new Shader(graphic_vertex_shader_source, graphic_fragment_shader_source);
    textrender_shader = new Shader(textrender_vertex_shader_source, textrender_fragment_shader_source);
}
GlobalVar::~GlobalVar() {
    delete graphic_shader;
    delete textrender_shader;
}
} // namespace gawl
