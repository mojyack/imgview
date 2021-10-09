#pragma once
#include <array>
#include <filesystem>
#include <fstream>
#include <string>

#include <gawl/gawl.hpp>

#include "indexed-paths.hpp"

struct Image {
    bool              needs_reload = false;
    std::string       wants_path;
    std::string       current_path;
    gawl::PixelBuffer buffer;
    gawl::Graphic     graphic;
};

class Imgview : public gawl::WaylandWindow {
  private:
    enum class Actions {
        NONE,
        QUIT_APP,
        NEXT_WORK,
        PREV_WORK,
        NEXT_PAGE,
        PREV_PAGE,
        REFRESH_FILES,
        PAGE_SELECT_ON,
        PAGE_SELECT_OFF,
        PAGE_SELECT_NUM,
        PAGE_SELECT_NUM_DEL,
        PAGE_SELECT_APPLY,
        TOGGLE_SHOW_INFO,
        MOVE_DRAW_POS,
        RESET_DRAW_POS,
        FIT_WIDTH,
        FIT_HEIGHT,
    };

    enum class InfoFormats {
        NONE,
        SHORT,
        LONG,
    };

    gawl::TextRender page_select_font;
    gawl::TextRender info_font;
    std::string      root;
    IndexedPaths     image_files;

    std::thread                         loader_thread;
    gawl::ConditionalVariable           loader_event;
    gawl::SafeVar<std::array<Image, 2>> images;

    bool        shift       = false;
    bool        page_select = false;
    std::string page_select_buffer;
    InfoFormats info_format       = InfoFormats::SHORT;
    double      draw_offset[2]    = {0, 0};
    double      draw_scale        = 1.0;
    bool        clicked[2]        = {false};
    bool        moved             = false;
    double      clicked_pos[2][2] = {};
    double      pointer_pos[2]    = {-1, -1};

    auto do_action(Actions action, uint32_t key = KEY_RESERVED) -> void;
    auto reset_draw_pos() -> void;
    auto calc_draw_area(const gawl::Graphic& graphic) const -> gawl::Rectangle;
    auto zoom_draw_pos(double value, double (&origin)[2]) -> void;
    auto check_existence(bool reverse) -> bool;
    auto start_loading(bool reverse) -> void;

    auto refresh_callback() -> void override;
    auto window_resize_callback() -> void override;
    auto keyboard_callback(uint32_t key, gawl::ButtonState state) -> void override;
    auto pointermove_callback(double x, double y) -> void override;
    auto click_callback(uint32_t button, gawl::ButtonState state) -> void override;
    auto scroll_callback(gawl::WheelAxis axis, double value) -> void override;

  public:
    Imgview(gawl::GawlApplication& app, const char* path);
    ~Imgview();
};
