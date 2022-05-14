#pragma once
#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>

#include <gawl/wayland/gawl.hpp>

#include "indexed-paths.hpp"
#include "util/thread.hpp"

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

class Imgview {
  private:
    using Window = gawl::wl::Window<Imgview, Imgview>;

    enum class Actions {
        None,
        QuitApp,
        NextWork,
        PrevWork,
        NextPage,
        PrevPage,
        RefreshFiles,
        PageSelectOn,
        PageSelectOff,
        PageSelectNum,
        PageSelectNumDel,
        PageSelectApply,
        ToggleShowInfo,
        MoveDrawPos,
        ResetDrawPos,
        FitWidth,
        FitHeight,
    };

    enum class InfoFormats {
        None,
        Short,
        Long,
    };

    Window&                window;
    gawl::TextRender       font;
    std::string            root;
    Critical<IndexedPaths> image_files;
    Critical<BufferCache>  buffer_cache;
    Critical<ImageCache>   image_cache;
    gawl::Graphic          displayed_graphic;
    std::thread            loader_thread;
    Event                  loader_event;
    bool                   finish_loader_thread_flag = false;

    bool        page_select = false;
    std::string page_select_buffer;
    InfoFormats info_format    = InfoFormats::Short;
    double      draw_offset[2] = {0, 0};
    double      draw_scale     = 1.0;
    bool        clicked[2]     = {false};
    bool        moved          = false;

    gawl::Point                clicked_pos[2];
    std::optional<gawl::Point> pointer_pos;

    const Caption* current_caption = nullptr; // only used in is_point_in_caption()

    auto do_action(Actions action, xkb_keysym_t key = XKB_KEY_VoidSymbol) -> void;
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
    auto keysym_callback(xkb_keycode_t key, gawl::ButtonState state, xkb_state* xkb_state) -> void;
    auto pointermove_callback(const gawl::Point& point) -> void;
    auto click_callback(uint32_t button, gawl::ButtonState state) -> void;
    auto scroll_callback(gawl::WheelAxis axis, double value) -> void;
    auto user_callback(void* data) -> void;
    Imgview(Window& window, const char* path);
    ~Imgview();
};

using Gawl = gawl::Gawl<Imgview>;
