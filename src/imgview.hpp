#pragma once
#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>

#include <gawl/wayland/gawl.hpp>

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

class Imgview;
using Gawl = gawl::Gawl<Imgview>;
class Imgview : public Gawl::Window {
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

    gawl::Point                clicked_pos[2];
    std::optional<gawl::Point> pointer_pos;

    const Caption* current_caption = nullptr; // only used in is_point_in_caption()

    auto do_action(Actions action, uint32_t key = KEY_RESERVED) -> void;
    auto reset_draw_pos() -> void;
    auto calc_draw_area(const gawl::Graphic& graphic) const -> gawl::Rectangle;
    auto zoom_draw_pos(double value, const gawl::Point& origin) -> void;
    auto check_existence(bool reverse) -> bool; // lock image_files before call this

    struct CaptionDrawHint {
        const Caption&  caption;
        gawl::Rectangle area;
        double          ext;
    };
    auto is_point_in_caption(gawl::Point point, const Image& image) const -> std::optional<CaptionDrawHint>;

  public:
    auto refresh_callback() -> void;
    auto window_resize_callback() -> void;
    auto keyboard_callback(uint32_t key, gawl::ButtonState state) -> void;
    auto pointermove_callback(const gawl::Point& point) -> void;
    auto click_callback(uint32_t button, gawl::ButtonState state) -> void;
    auto scroll_callback(gawl::WheelAxis axis, double value) -> void;
    auto user_callback(void* data) -> void;
    Imgview(Gawl::WindowCreateHint& hint, const char* path);
    ~Imgview();
};
