#pragma once
#include <coop/generator.hpp>
#include <coop/multi-event.hpp>

#include "displayable/displayable.hpp"
#include "file-list.hpp"
#include "gawl/textrender.hpp"
#include "gawl/window-no-touch-callbacks.hpp"

class Callbacks : public gawl::WindowNoTouchCallbacks {
  private:
    using Cache = std::vector<std::shared_ptr<Displayable>>;

    gawl::TextRender                font;
    FileList                        list;
    Cache                           cache;
    std::shared_ptr<Displayable>    last_displayed;
    std::string                     page_jump_buffer;
    gawl::Point                     clicked_pos[2];
    std::optional<gawl::Point>      pointer_pos;
    coop::MultiEvent                worker_event;
    std::array<coop::TaskHandle, 4> workers;

    constexpr static auto move_speed  = 60.0;
    constexpr static auto cache_range = 4;

    double draw_offset[2] = {0, 0};
    double draw_scale     = 0.0;

    bool page_jump  = false;
    bool clicked[2] = {false, false};
    bool moved      = false;
    bool hide_info  = false;

    auto check_existence(bool reverse, FileList& files) -> bool;
    auto change_page(bool reverse) -> void;
    auto set_index_by_page_jump_buffer() -> bool;
    auto reset_draw_pos() -> void;
    auto worker_main() -> coop::Async<void>;

  public:
    auto close() -> void override;
    auto refresh() -> void override;
    auto on_keycode(uint32_t keycode, gawl::ButtonState state) -> coop::Async<bool> override;
    auto on_pointer(gawl::Point /*pos*/) -> coop::Async<bool> override;
    auto on_click(uint32_t /*button*/, gawl::ButtonState /*state*/) -> coop::Async<bool> override;
    auto on_created(gawl::Window* /*window*/) -> coop::Async<bool> override;

    auto init(int argc, const char* const argv[]) -> bool;

    Callbacks();
    ~Callbacks();
};
