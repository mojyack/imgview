#pragma once
#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>

#include <gawl/gawl.hpp>

#include "indexed-paths.hpp"

struct Caption {
    std::array<size_t, 4> area;
    std::string           text;

    int         size;
    gawl::Align alignx;
    gawl::Align aligny;
};

struct Image {
    gawl::Graphic        graphic;
    std::vector<Caption> captions;
};

using BufferCache = std::unordered_map<std::string, gawl::PixelBuffer>;
using ImageCache  = std::unordered_map<std::string, Image>;

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

    gawl::TextRender             page_select_font;
    gawl::TextRender             info_font;
    gawl::TextRender             caption_font;
    std::string                  root;
    gawl::Critical<IndexedPaths> image_files;
    gawl::Critical<BufferCache>  buffer_cache;
    gawl::Critical<ImageCache>   image_cache;
    gawl::Graphic                displayed_graphic;
    std::thread                  loader_thread;
    gawl::Event                  loader_event;
    bool                         finish_loader_thread_flag = false;

    bool        shift       = false;
    bool        page_select = false;
    std::string page_select_buffer;
    InfoFormats info_format    = InfoFormats::SHORT;
    double      draw_offset[2] = {0, 0};
    double      draw_scale     = 1.0;
    bool        clicked[2]     = {false};
    bool        moved          = false;

    std::array<double, 2>                clicked_pos[2] = {};
    std::optional<std::array<double, 2>> pointer_pos;

    const Caption* current_caption = nullptr; // only used in is_point_in_caption()

    auto do_action(Actions action, uint32_t key = KEY_RESERVED) -> void;
    auto reset_draw_pos() -> void;
    auto calc_draw_area(const gawl::Graphic& graphic) const -> gawl::Rectangle;
    auto zoom_draw_pos(double value, const std::array<double, 2>& pointer) -> void;
    auto check_existence(bool reverse) -> bool; // lock image_files before call this

    struct CaptionDrawHint {
        const Caption&  caption;
        gawl::Rectangle area;
        double          ext;
    };
    auto is_point_in_caption(double x, double y, const Image& image) const -> std::optional<CaptionDrawHint>;

    auto refresh_callback() -> void override;
    auto window_resize_callback() -> void override;
    auto keyboard_callback(uint32_t key, gawl::ButtonState state) -> void override;
    auto pointermove_callback(double x, double y) -> void override;
    auto click_callback(uint32_t button, gawl::ButtonState state) -> void override;
    auto scroll_callback(gawl::WheelAxis axis, double value) -> void override;
    auto user_callback(void* data) -> void override;

  public:
    Imgview(gawl::GawlApplication& app, const char* path);
    ~Imgview();
};
