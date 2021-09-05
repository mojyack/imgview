#pragma once
#include <array>
#include <filesystem>
#include <fstream>
#include <string>

#include <gawl.hpp>

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

    void       do_action(Actions action, uint32_t key = KEY_RESERVED);
    void       reset_draw_pos();
    gawl::Area calc_draw_area(const gawl::Graphic& graphic) const;
    void       zoom_draw_pos(double value, double (&origin)[2]);
    bool       check_existence(const bool reverse);
    void       start_loading(const bool reverse);

    void refresh_callback() override;
    void window_resize_callback() override;
    void keyboard_callback(uint32_t key, gawl::ButtonState state) override;
    void pointermove_callback(double x, double y) override;
    void click_callback(uint32_t button, gawl::ButtonState state) override;
    void scroll_callback(gawl::WheelAxis axis, double value) override;

  public:
    Imgview(gawl::GawlApplication& app, const char* path);
    ~Imgview();
};
