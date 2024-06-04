#include <filesystem>
#include <linux/input.h>

#include "displayable/image.hpp"
#include "displayable/text.hpp"
#include "file-list.hpp"
#include "gawl/application.hpp"
#include "gawl/fc.hpp"
#include "gawl/misc.hpp"
#include "gawl/wayland/window.hpp"
#include "imgview.hpp"
#include "macros/unwrap.hpp"
#include "util/assert.hpp"
#include "util/charconv.hpp"

namespace {
auto find_current_index(const std::filesystem::path dir) -> std::optional<FileList> {
    unwrap_oo_mut(list, list_files(dir.parent_path().string()));
    filter_regular_files(list);
    for(auto i = size_t(0); i < list.files.size(); i += 1) {
        if(list.files[i] == dir.filename().string()) {
            list.index = i;
            return list;
        }
    }
    return std::nullopt;
}

auto find_next_directory(const std::filesystem::path dir, const bool reverse) -> std::optional<std::string> {
    unwrap_oo(list, find_current_index(dir));
    if((reverse && list.index == 0) || (!reverse && list.index + 1 >= list.files.size())) {
        if(dir.parent_path() == dir) { // dir == "/"
            return std::nullopt;
        }
        return find_next_directory(dir.parent_path(), reverse);
    }
    return list.prefix / list.files[list.index + (reverse ? -1 : 1)];
}

auto find_next_displayable_directory(std::filesystem::path dir, const bool reverse) -> std::optional<FileList> {
loop:
    unwrap_oo_mut(next_dir, find_next_directory(dir, reverse));
    unwrap_oo_mut(list, list_files(next_dir));
    filter_non_image_files(list);
    if(!list.files.empty()) {
        return list;
    }
    dir = std::move(next_dir);
    goto loop;
}
} // namespace

auto Callbacks::change_page(const bool reverse) -> void {
    {
        auto [lock, list] = critical_files.access();
        if((reverse && list.index == 0) || (!reverse && list.index + 1 == list.files.size())) {
            return;
        }
        list.index += reverse ? -1 : 1;
    }
    worker_event.wakeup();
    window->refresh();
}

auto Callbacks::set_index_by_page_jump_buffer() -> bool {
    unwrap_ob(page, from_chars<size_t>(page_jump_buffer));
    auto [lock_f, list] = critical_files.access();
    assert_b(page < list.files.size());
    list.index = page;
    return true;
}

auto Callbacks::reset_draw_pos() -> void {
    draw_offset[0] = 0;
    draw_offset[1] = 0;
    draw_scale     = 0.0;
}

auto Callbacks::worker_main() -> void {
    auto context = std::bit_cast<gawl::WaylandWindow*>(window)->fork_context();
loop:
    if(!running) {
        return;
    }
    // find target
    auto displayable = std::shared_ptr<Displayable>();
    auto work        = std::filesystem::path();
    auto file        = std::string();
    auto index       = int();
    {
        const auto [lock_f, list]  = critical_files.access();
        const auto [lock_c, cache] = critical_cache.access();

        const auto range = std::min(cache_range, int(list.files.size() / 2) + 1);
        for(auto distance = 0; distance < range; distance += 1) {
            for(auto backward = (distance == 0 ? 1 : 0); backward < 2; backward += 1) {
                const auto i = int(list.index) + (backward == 0 ? distance : -distance);
                if(i < 0 || size_t(i) >= list.files.size()) {
                    continue;
                }
                if(cache[i]) {
                    continue;
                }
                const auto ext = std::filesystem::path(list.files[i]).extension();
                auto       ptr = (Displayable*)(nullptr);
                if(ext == ".txt") {
                    ptr = new DisplayableText(font);
                } else {
                    ptr = new DisplayableImage();
                }
                displayable = std::shared_ptr<Displayable>(ptr);
                cache[i]    = displayable;
                work        = list.prefix;
                file        = list.files[i];
                index       = i;
                goto search_end;
            }
        }
    }
search_end:
    if(!displayable) {
        worker_event.wait();
        worker_event.clear();
        goto loop;
    }

    // load file
    const auto path = work / file;
    if(!displayable->load(path.string())) {
        displayable = std::shared_ptr<Displayable>(new DisplayableText(font, "broken image"));
    }
    context.wait();

    // store
    {
        const auto [lock_f, list] = critical_files.access();

        if(list.prefix != work) {
            // work changed
            goto loop;
        }

        const auto [lock_c, cache] = critical_cache.access();

        displayable->loaded = true;
        cache[index]        = std::move(displayable);
        window->refresh();

        // clean cache
        const auto begin = list.index > cache_range ? list.index - cache_range : 0;
        const auto end   = std::min(list.index + cache_range, list.files.size() - 1);

        for(auto i = size_t(0); i < begin; i += 1) {
            cache[i].reset();
        }
        for(auto i = end + 1; i < cache.size(); i += 1) {
            cache[i].reset();
        }
    }

    goto loop;
}

auto Callbacks::refresh() -> void {
    gawl::clear_screen({0, 0, 0, 0});
    const auto [width, height] = window->get_window_size();
    const auto [lock, list]    = critical_files.access();
    if(list.files.empty()) {
        return;
    }

    const auto draw_params = DrawParameters{{width, height}, {draw_offset[0], draw_offset[1]}, draw_scale};
    const auto path        = list.prefix / list.files[list.index];
    {
        const auto [lock, cache] = critical_cache.access();
        const auto dable         = cache[list.index];
        if(dable && dable->loaded) {
            dable->draw(window, draw_params);
        } else {
            if(last_displayed) {
                last_displayed->draw(window, draw_params);
            }
            font.draw_fit_rect(*window, {{0, 0}, {1. * width, 1. * height}}, {1, 1, 1, 1}, "loading...");
        }
    }
    auto top = 0.0;
    if(!hide_info) {
        const auto info = path.parent_path().filename() / path.filename();
        const auto str  = build_string("[", list.index + 1, "/", list.files.size(), "]", info.string());
        const auto rect = gawl::Rectangle(font.get_rect(*window, str.data())).expand(2, 2);
        const auto box  = gawl::Rectangle{{0, height - rect.height() - top}, {rect.width(), height - top}};
        gawl::draw_rect(*window, box, {0, 0, 0, 0.5});
        font.draw_fit_rect(*window, box, {1, 1, 1, 0.7}, str.data());
        top += rect.height();
    }
    if(page_jump) {
        const auto str  = std::string("Page: ") + page_jump_buffer;
        const auto rect = font.get_rect(*window, str.data());
        const auto box  = gawl::Rectangle{{0, height - rect.height() - top}, {rect.width(), height - top}};
        font.draw_fit_rect(*window, box, {1, 1, 1, 0.7}, str.data());
        top += rect.height();
    }
}

auto Callbacks::on_keycode(const uint32_t keycode, const gawl::ButtonState state) -> void {
    if(state == gawl::ButtonState::Enter || state == gawl::ButtonState::Leave || state == gawl::ButtonState::Release) {
        return;
    }

    switch(keycode) {
    case KEY_Q:
    case KEY_BACKSLASH:
        // quit
        std::quick_exit(0); // hack
        application->quit();
        break;
    case KEY_DOWN:
    case KEY_UP: {
        // next/prev work
        const auto reverse = keycode == KEY_UP;
        {
            auto [lock_f, list] = critical_files.access();
            unwrap_on_mut(next_list, find_next_displayable_directory(list.prefix, reverse), "cannot find next directory");
            list                 = std::move(next_list);
            auto [lock_c, cache] = critical_cache.access();
            cache                = Cache(list.files.size());
        }
        worker_event.wakeup();
        window->refresh();
    } break;
    case KEY_SPACE:
    case KEY_RIGHT:
    case KEY_LEFT: {
        // next/prev page
        const auto reverse = keycode == KEY_LEFT;
        change_page(reverse);
    } break;
    case KEY_P:
        // page jump begin
        page_jump = true;
        page_jump_buffer.clear();
        window->refresh();
        break;
    case KEY_ESC:
        // page jump cancel
        page_jump = false;
        window->refresh();
        break;
    case KEY_BACKSPACE:
        // page jump delete
        if(!page_jump_buffer.empty()) {
            page_jump_buffer.pop_back();
        }
        window->refresh();
        break;
    case KEY_ENTER:
        // page jump apply
        if(set_index_by_page_jump_buffer()) {
            worker_event.wakeup();
        }
        page_jump = false;
        window->refresh();
        break;
    case KEY_H:
    case KEY_L:
        // move horizontal position
        draw_offset[0] += keycode == KEY_H ? move_speed : -move_speed;
        window->refresh();
        break;
    case KEY_K:
    case KEY_J:
        // move vertical position
        draw_offset[1] += keycode == KEY_K ? move_speed : -move_speed;
        window->refresh();
        break;
    case KEY_O:
        // reset position
        reset_draw_pos();
        break;
    }

    if(page_jump) {
        if(keycode >= KEY_1 && keycode <= KEY_0) {
            // page jump input
            page_jump_buffer += (keycode == KEY_0 ? '0' : char('1' + keycode - KEY_1));
            window->refresh();
        }
    }
}

auto Callbacks::on_pointer(const gawl::Point& pos) -> void {
    auto do_refresh = false;
    if(pointer_pos.has_value()) {
        if(clicked[0]) {
            draw_offset[0] += pos.x - pointer_pos->x;
            draw_offset[1] += pos.y - pointer_pos->y;
            do_refresh = true;
        }
        if(clicked[1]) {
            do {
                auto [lock_f, list]  = critical_files.access();
                auto [lock_c, cache] = critical_cache.access();
                const auto path      = (list.prefix / list.files[list.index]).string();
                if(!cache[list.index] || !cache[list.index]->loaded) {
                    break;
                }
                const auto [width, height] = window->get_window_size();
                const auto value           = (pos.y - pointer_pos->y) * 0.01;
                auto       draw_params     = DrawParameters{{width, height}, {draw_offset[0], draw_offset[1]}, draw_scale};
                cache[list.index]->zoom_by_drag(window, clicked_pos[1], value, draw_params);
                draw_offset[0] = draw_params.offset[0];
                draw_offset[1] = draw_params.offset[1];
                draw_scale     = draw_params.scale;
                do_refresh     = true;
            } while(0);
        }
    }
    pointer_pos = pos;
    moved       = true;
    if(do_refresh) {
        window->refresh();
    }
}

auto Callbacks::on_click(const uint32_t button, const gawl::ButtonState state) -> void {
    if(button != BTN_LEFT && button != BTN_RIGHT) {
        return;
    }

    const auto i = button == BTN_LEFT ? 0 : 1;

    clicked[i] = state == gawl::ButtonState::Press;
    if(pointer_pos.has_value()) {
        clicked_pos[i] = *pointer_pos;
    }
    if(clicked[i] == false && moved == false) {
        change_page(false);
    }
    moved = false;
}

auto Callbacks::on_scroll(gawl::WheelAxis /*axis*/, double /*value*/) -> void {
}

auto Callbacks::init(const int argc, const char* const argv[]) -> bool {
    assert_b(argc > 1);

    const auto abs  = std::filesystem::absolute(argv[1]);
    auto       list = FileList();
    if(argc == 2) {
        if(std::filesystem::is_directory(argv[1])) {
            unwrap_ob_mut(l, list_files(abs.string()));
            list = std::move(l);
            filter_non_image_files(list);
            assert_b(!list.files.empty());
        } else if(std::filesystem::is_regular_file(argv[1])) {
            unwrap_ob_mut(l, list_files(abs.parent_path().string()));
            list = std::move(l);
            filter_non_image_files(list);
            assert_b(!list.files.empty());
            for(auto i = size_t(0); i < list.files.size(); i += 1) {
                if(list.files[i] == abs.filename()) {
                    list.index = i;
                }
            }
        } else {
            warn("no such file");
            return false;
        }
    } else {
        list.prefix = abs.parent_path();
        for(auto i = 1; i < argc; i += 1) {
            list.files.push_back(std::filesystem::path(argv[i]).filename());
        }
    }

    critical_cache.unsafe_access() = Cache(list.files.size());
    critical_files.unsafe_access() = std::move(list);
    return true;
}

auto Callbacks::run() -> void {
    running = true;
    for(auto& worker : workers) {
        worker = std::thread(&Callbacks::worker_main, this);
    }
}

Callbacks::Callbacks()
    : font(gawl::TextRender({gawl::find_fontpath_from_name("Noto Sans CJK JP:style=Bold").value().data()}, 16)) {
}

Callbacks::~Callbacks() {
    if(!running) {
        return;
    }

    running = false;
    worker_event.wakeup();
    for(auto& worker : workers) {
        worker.join();
    }
}
