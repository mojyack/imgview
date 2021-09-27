#include <algorithm>
#include <cmath>
#include <iostream>

#include <linux/input-event-codes.h>

#include "imgview.hpp"
#include "path.hpp"
#include "sort.hpp"

namespace {
enum class PathCheckResult {
    EXISTS,
    RETRIEVED,
    FATAL,
};

auto check_and_retrieve_paths(const char* const root, IndexedPaths& files, const bool reverse) -> PathCheckResult {
    if(std::filesystem::exists(files.get_current())) {
        return PathCheckResult::EXISTS;
    }
    auto images  = true;
    auto missing = Path(files.get_current());
    auto path    = missing.parent_path();
    while(1) {
        do {
            if(!std::filesystem::is_directory(path)) {
                break;
            }
            auto updated = images ? get_sorted_images(path.string().data()) : get_sorted_directories(path.string().data());
            if(updated.empty()) {
                break;
            }
            const auto missing_name = missing.filename().string();
            auto       file_names   = updated.get_file_names();
            file_names.emplace_back(missing_name);
            sort_string(file_names);
            auto       index            = static_cast<size_t>(std::distance(file_names.begin(), std::find(file_names.begin(), file_names.end(), missing_name)));
            const auto selectable_front = (index != (file_names.size() - 1));
            const auto selectable_back  = (index != 0);
            file_names.erase(file_names.begin() + index);
            index += (reverse && selectable_back) ? -1 : (!reverse && selectable_front) ? 0
                                                     : selectable_front                 ? -1
                                                                                        : 0;
            const auto paths = IndexedPaths(path.string(), file_names, index);
            if(images) {
                files = std::move(paths);
            } else {
                auto viewable = find_viewable_directory(paths[index].data());
                if(!viewable) {
                    break;
                }
                files = get_sorted_images(viewable->data());
            }
            return PathCheckResult::RETRIEVED;
        } while(0);
        if(path == root) {
            return PathCheckResult::FATAL;
        }
        images  = false;
        missing = std::move(path);
        path    = missing.parent_path();
    }
}
} // namespace

auto Imgview::do_action(const Actions action, const uint32_t key) -> void {
    switch(action) {
    case Actions::NONE:
        break;
    case Actions::QUIT_APP:
        quit_application();
        break;
    case Actions::NEXT_WORK:
    case Actions::PREV_WORK: {
        const auto reverse = action == Actions::PREV_WORK;
        if(check_existence(reverse)) {
            if(const auto next_directory = get_next_directory(image_files.get_base().data(), action == Actions::PREV_WORK, root.data())) {
                image_files = get_sorted_images(next_directory->data());
                start_loading(false);
                refresh();
            }
        }
    } break;
    case Actions::NEXT_PAGE:
    case Actions::PREV_PAGE: {
        const auto reverse    = action == Actions::PREV_PAGE;
        auto       do_update  = true;
        auto       do_refresh = false;
        do {
            const auto current_index = image_files.get_index();
            if((!reverse && current_index + 1 >= image_files.size()) || (reverse && current_index <= 0)) {
                break;
            }
            const auto next_index = current_index + (!reverse ? 1 : -1);
            const auto next_path  = image_files[next_index];
            if(!std::filesystem::exists(next_path)) {
                break;
            }
            image_files.set_index(next_index);
            do_update  = false;
            do_refresh = true;
        } while(0);
        if(do_update) {
            if(check_existence(reverse)) {
                if(auto updated = get_next_image_file(image_files.get_current().data(), reverse)) {
                    image_files = std::move(updated.value());
                    do_refresh  = true;
                }
            }
        }
        if(do_refresh) {
            start_loading(reverse);
            refresh();
        }
    } break;
    case Actions::REFRESH_FILES:
        if(check_existence(false)) {
            image_files = get_sorted_images(Path(image_files.get_current()).parent_path().string().data());
        }
        break;
    case Actions::PAGE_SELECT_ON:
        page_select = true;
        page_select_buffer.clear();
        refresh();
        break;
    case Actions::PAGE_SELECT_OFF:
        page_select = false;
        refresh();
        break;
    case Actions::PAGE_SELECT_NUM:
        page_select_buffer += (key == KEY_0 ? '0' : static_cast<char>('1' + key - KEY_1));
        refresh();
        break;
    case Actions::PAGE_SELECT_NUM_DEL:
        if(!page_select_buffer.empty()) {
            page_select_buffer.pop_back();
        }
        refresh();
        break;
    case Actions::PAGE_SELECT_APPLY: {
        page_select = false;
        if(check_existence(false)) {
            const auto files = get_sorted_images(image_files.get_base().data());
            if(const auto p = std::atoi(page_select_buffer.data()) - 1; p >= 0 && static_cast<long unsigned int>(p) < files.size()) {
                image_files = std::move(files);
                image_files.set_index(p);
                start_loading(false);
            }
        }
        refresh();
    } break;
    case Actions::TOGGLE_SHOW_INFO:
        switch(info_format) {
        case InfoFormats::NONE:
            info_format = InfoFormats::SHORT;
            break;
        case InfoFormats::SHORT:
            info_format = InfoFormats::LONG;
            break;
        case InfoFormats::LONG:
            info_format = InfoFormats::NONE;
            break;
        }
        refresh();
        break;
    case Actions::MOVE_DRAW_POS: {
        constexpr double SPEED = 60;
        draw_offset[0] += key == KEY_L ? SPEED : key == KEY_H ? -SPEED
                                                              : 0;
        draw_offset[1] += key == KEY_J ? SPEED : key == KEY_K ? -SPEED
                                                              : 0;
        refresh();
    } break;
    case Actions::RESET_DRAW_POS:
        reset_draw_pos();
        refresh();
        break;
    case Actions::FIT_WIDTH:
    case Actions::FIT_HEIGHT: {
        {
            const auto  lock    = images.get_lock();
            const auto& current = images.data[0];
            if(current.wants_path != current.current_path) {
                break;
            }
            reset_draw_pos();
            const auto  size  = std::array{current.graphic.get_width(this), current.graphic.get_height(this)};
            const auto& wsize = get_window_size();
            const auto  area  = gawl::calc_fit_rect({0, 0, 1. * wsize[0], 1. * wsize[1]}, size[0], size[1]);
            draw_scale        = action == Actions::FIT_WIDTH ? (wsize[0] - area.width()) / size[0] : (wsize[1] - area.height()) / size[1];
        }
        refresh();
    } break;
    }
}
auto Imgview::reset_draw_pos() -> void {
    draw_offset[0] = 0;
    draw_offset[1] = 0;
    draw_scale     = 0.0;
}
auto Imgview::calc_draw_area(const gawl::Graphic& graphic) const -> gawl::Area {
    const auto size = std::array{graphic.get_width(this), graphic.get_height(this)};
    const auto exp  = std::array{size[0] * draw_scale / 2.0, size[1] * draw_scale / 2.0};
    auto       area = gawl::calc_fit_rect({0, 0, 1. * get_window_size()[0], 1. * get_window_size()[1]}, size[0], size[1]);
    area[0] += draw_offset[0] - exp[0];
    area[1] += draw_offset[1] - exp[1];
    area[2] += draw_offset[0] + exp[0];
    area[3] += draw_offset[1] + exp[1];
    return area;
}
auto Imgview::zoom_draw_pos(const double value, double (&origin)[2]) -> void {
    const auto  lock    = images.get_lock();
    const auto& current = images.data[0];
    if(current.wants_path != current.current_path) {
        return;
    }
    const auto area  = calc_draw_area(current.graphic);
    const auto delta = std::array{current.graphic.get_width(this) * value, current.graphic.get_height(this) * value};
    for(int i = 0; i < 2; i += 1) {
        const double center = area[i] + (area[i + 2] - area[i]) / 2;
        draw_offset[i] += ((center - origin[i]) / (area[i + 2] - area[i])) * delta[i];
    }
    draw_scale += value;
}
auto Imgview::check_existence(const bool reverse) -> bool {
    switch(check_and_retrieve_paths(root.data(), image_files, reverse)) {
    case PathCheckResult::EXISTS:
        return true;
    case PathCheckResult::RETRIEVED:
        start_loading(false);
        refresh();
        break;
    case PathCheckResult::FATAL:
        quit_application();
        break;
    }
    return false;
}
auto Imgview::start_loading(const bool reverse) -> void {
    const auto lock      = images.get_lock();
    auto&      current   = images.data[0];
    auto&      preload   = images.data[1];
    current.wants_path   = image_files.get_current();
    current.needs_reload = true;

    // check preload
    for(auto& i : images.data) {
        if(current.wants_path == i.current_path) {
            current.buffer       = std::move(i.buffer);
            current.current_path = std::move(i.current_path);
            break;
        }
    }

    // load preload
    const auto index = image_files.get_index();
    if(index != (reverse ? 0 : image_files.size() - 1)) {
        preload.wants_path   = image_files[index + (reverse ? -1 : 1)];
        preload.needs_reload = true;
    } else {
        preload.wants_path.clear();
    }

    loader_event.wakeup();
}
auto Imgview::refresh_callback() -> void {
    gawl::clear_screen({0, 0, 0, 0});

    auto current_loaded = true;
    {
        const auto lock    = images.get_lock();
        auto&      current = images.data[0];
        if(current.needs_reload) {
            if(current.wants_path == current.current_path) {
                current.graphic = gawl::Graphic(current.buffer);
                reset_draw_pos();
                current.needs_reload = false;
            } else {
                current_loaded = false;
            }
        }
    }
    auto& current = images.data[0].graphic;
    if(current) {
        current.draw_rect(this, calc_draw_area(current));
    }
    if(!current_loaded) {
        info_font.draw_fit_rect(this, {0, 0, 1. * get_window_size()[0], 1. * get_window_size()[1]}, {1, 1, 1, 1}, "loading...");
    }
    if(page_select) {
        constexpr auto pagestr       = "Page: ";
        static auto    pagestr_width = int(-1);

        constexpr auto dist = 45;
        if(pagestr_width == -1) {
            gawl::Area rec;
            page_select_font.get_rect(this, rec, pagestr);
            pagestr_width = rec[2] - rec[0];
        }
        const auto& size = get_window_size();
        page_select_font.draw(this, 5, size[1] - dist, {1, 1, 1, 1}, pagestr);
        page_select_font.draw(this, 5 + pagestr_width, size[1] - dist, {1, 1, 1, 1}, page_select_buffer.data());
    }
    if(info_format != InfoFormats::NONE) {
        const auto current_path = Path(image_files.get_current());
        auto       work_name    = std::string();
        switch(info_format) {
        case InfoFormats::SHORT:
            work_name = current_path.parent_path().filename().string() + "/" + current_path.filename().string();
            break;
        case InfoFormats::LONG:
            work_name = std::filesystem::relative(current_path, root).string();
            break;
        default:
            break;
        }
        auto infostr = std::ostringstream();
        infostr << "[" << image_files.get_index() + 1 << "/" << image_files.size() << "] " << work_name;
        constexpr auto dist = 7;
        const auto&    size = get_window_size();
        {
            auto rec = gawl::Area{5.0, static_cast<double>(size[1] - dist)};
            auto sp  = 3.0;
            info_font.get_rect(this, rec, infostr.str().data());
            gawl::draw_rect(this, {rec[0] - sp, rec[1] - sp, rec[2] + sp, rec[3] + sp}, {0, 0, 0, 0.5});
        }
        info_font.draw(this, 5, size[1] - dist, {1, 1, 1, 0.7}, infostr.str().data());
    }
}
auto Imgview::window_resize_callback() -> void {
    reset_draw_pos();
}
auto Imgview::keyboard_callback(const uint32_t key, const gawl::ButtonState state) -> void {
    const static auto num_keys = std::vector<uint32_t>{KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9};
    struct KeyBind {
        Actions                   action;
        std::vector<uint32_t>     keys;
        bool                      on_press;
        std::function<bool(void)> condition = nullptr;
    };
    const static KeyBind keybinds[] = {
        {Actions::QUIT_APP, {KEY_Q, KEY_BACKSLASH}, true},
        {Actions::NEXT_WORK, {KEY_X, KEY_RIGHT}, true, [this]() -> bool { return shift; }},
        {Actions::NEXT_WORK, {KEY_DOWN}, true},
        {Actions::PREV_WORK, {KEY_Z, KEY_LEFT}, true, [this]() -> bool { return shift; }},
        {Actions::PREV_WORK, {KEY_UP}, true},
        {Actions::NEXT_PAGE, {KEY_X, KEY_RIGHT, KEY_SPACE}, true},
        {Actions::PREV_PAGE, {KEY_Z, KEY_PAGEDOWN}, true},
        {Actions::REFRESH_FILES, {KEY_R}, false},
        {Actions::PAGE_SELECT_ON, {KEY_P}, false, [this]() -> bool { return !page_select; }},
        {Actions::PAGE_SELECT_OFF, {KEY_ESC, KEY_P}, false, [this]() -> bool { return page_select; }},
        {Actions::PAGE_SELECT_NUM, num_keys, true, [this]() -> bool { return page_select; }},
        {Actions::PAGE_SELECT_NUM_DEL, {KEY_BACKSPACE}, false, [this]() -> bool { return page_select; }},
        {Actions::PAGE_SELECT_APPLY, {KEY_ENTER}, false, [this]() -> bool { return page_select; }},
        {Actions::TOGGLE_SHOW_INFO, {KEY_F}, false},
        {Actions::MOVE_DRAW_POS, {KEY_H, KEY_J, KEY_K, KEY_L}, false},
        {Actions::RESET_DRAW_POS, {KEY_0}, false},
        {Actions::FIT_WIDTH, {KEY_1}, false},
        {Actions::FIT_HEIGHT, {KEY_2}, false},
    };
    auto action = Actions::NONE;
    if(key == KEY_LEFTSHIFT || key == KEY_RIGHTSHIFT) {
        shift = state == gawl::ButtonState::press;
        return;
    }
    for(auto& a : keybinds) {
        if((state == gawl::ButtonState::press && !a.on_press) || (state == gawl::ButtonState::release && a.on_press)) {
            continue;
        }
        for(auto k : a.keys) {
            if(k == key && (!a.condition || a.condition())) {
                action = a.action;
                break;
            }
        }
        if(action != Actions::NONE) break;
    }
    do_action(action, key);
}
auto Imgview::pointermove_callback(const double x, const double y) -> void {
    if(pointer_pos[0] != -1 && pointer_pos[1] != -1) {
        if(clicked[0]) {
            draw_offset[0] += x - pointer_pos[0];
            draw_offset[1] += y - pointer_pos[1];
            refresh();
        }
        if(clicked[1]) {
            zoom_draw_pos((pointer_pos[1] - y) * 0.001, clicked_pos[1]);
            refresh();
        }
    }
    pointer_pos[0] = x;
    pointer_pos[1] = y;
    moved          = true;
}
auto Imgview::click_callback(const uint32_t button, const gawl::ButtonState state) -> void {
    if(button != BTN_LEFT && button != BTN_RIGHT) {
        return;
    }
    clicked[button == BTN_RIGHT]        = state == gawl::ButtonState::press;
    clicked_pos[button == BTN_RIGHT][0] = pointer_pos[0];
    clicked_pos[button == BTN_RIGHT][1] = pointer_pos[1];
    if(clicked[button == BTN_RIGHT] == false && moved == false) {
        do_action(Actions::NEXT_PAGE);
    }
    moved = false;
}
auto Imgview::scroll_callback(gawl::WheelAxis /* axis */, double value) -> void {
    constexpr auto rate = 0.00001;
    if(pointer_pos[0] == -1 || pointer_pos[1] == -1) {
        return;
    }
    zoom_draw_pos(rate * std::pow(value, 3), pointer_pos);
    refresh();
}
Imgview::Imgview(gawl::GawlApplication& app, const char* const path) : gawl::WaylandWindow(app, {.title = "Imgview"}) {
    set_event_driven(true);
    if(!std::filesystem::exists(path)) {
        quit_application();
        return;
    }
    const auto arg   = std::filesystem::absolute(path);
    const auto dir   = std::filesystem::is_directory(arg) ? arg : arg.parent_path();
    auto       files = get_sorted_images(dir.string().data());
    if(is_regular_file(arg)) {
        const auto filename = arg.filename().string();
        if(!files.set_index_by_name(filename.data())) {
            std::cerr << "no such file." << std::endl;
            quit_application();
            return;
        }
    }
    image_files = std::move(files);
    start_loading(false);
    root = dir.parent_path().string();

    loader_thread = std::thread([this]() {
        while(is_running()) {
            auto loading = std::string();
            {
                const auto lock = images.get_lock();
                for(auto& i : images.data) {
                    if(i.wants_path == i.current_path || i.wants_path.empty()) {
                        continue;
                    }
                    loading = i.wants_path;
                }
            }
            if(!loading.empty()) {
                auto buffer = gawl::PixelBuffer(loading.data(), gawl::GraphicLoader::DEVIL);
                {
                    const auto lock = images.get_lock();
                    for(auto& i : images.data) {
                        if(i.wants_path == loading) {
                            i.buffer       = std::move(buffer);
                            i.current_path = std::move(loading);
                            refresh();
                            break;
                        }
                    }
                }
            } else {
                loader_event.wait();
            }
        }
    });

    page_select_font = gawl::TextRender({"/usr/share/fonts/cascadia-code/CascadiaCode.ttf"}, 16);
    info_font        = gawl::TextRender({"/usr/share/fonts/noto-cjk/NotoSansCJK-Black.ttc"}, 16);
}
Imgview::~Imgview() {
    if(loader_thread.joinable()) {
        loader_event.wakeup();
        loader_thread.join();
    }
}
