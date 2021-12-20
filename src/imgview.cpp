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
auto read_captions(const char* const path) -> std::vector<Caption> {
    auto r = std::vector<Caption>();
    auto s = std::ifstream(path);

    if(s.fail()) {
        return r;
    }

    auto l = std::string();

    // read version
    if(!getline(s, l) || l != "caption 0") {
        return r;
    }

    auto default_size   = 18;
    auto default_alignx = gawl::Align::left;
    auto default_aligny = gawl::Align::center;

    while(getline(s, l)) {
        if(l.starts_with("#")) {
            l += ",";
            const auto size = l.size();
            auto       pos  = std::string::size_type(1);
            auto       prev = std::string::size_type(1);
            auto       arr  = std::vector<size_t>();
            for(; pos < size && (pos = l.find(",", pos)) != l.npos; prev = (pos += 1)) {
                try {
                    arr.emplace_back(std::stoull(l.substr(prev, pos - prev)));
                } catch(const std::invalid_argument&) {
                    return r;
                }
            }
            if(arr.size() != 4) {
                return r;
            }
            r.emplace_back(Caption{.size = default_size, .alignx = default_alignx, .aligny = default_aligny});
            std::copy_n(arr.begin(), 4, r.back().area.begin());
        } else if(l.starts_with("$")) {
            if(l.starts_with("$size")) {
                const auto s = l.find(' ');
                if(s == l.npos) {
                    continue;
                }
                auto size = 0;
                try {
                    size = std::stoull(l.substr(s + 1));
                } catch(const std::invalid_argument&) {
                    continue;
                }
                if(r.empty()) {
                    default_size = size;
                } else {
                    r.back().size = size;
                }
            } else if(l.starts_with("$align")) {
                if(l.size() < 7) {
                    continue;
                }
                auto align = gawl::Align();
                if(const auto spec = l.substr(8); spec == "center") {
                    align = gawl::Align::center;
                } else if(spec == "left") {
                    align = gawl::Align::left;
                } else if(spec == "right") {
                    align = gawl::Align::right;
                } else {
                    continue;
                }
                if(l[6] == 'x') {
                    if(r.empty()) {
                        default_alignx = align;
                    } else {
                        r.back().alignx = align;
                    }
                } else if(l[6] == 'y') {
                    if(r.empty()) {
                        default_aligny = align;
                    } else {
                        r.back().aligny = align;
                    }
                } else {
                    continue;
                }
            }
        } else if(l.starts_with("\\")) {
            continue;
        } else if(l.starts_with("<!>")) {
            break;
        } else {
            if(r.empty()) {
                return r;
            }
            auto& caption = r.back();
            caption.text += l + "\\n";
        }
    }
    for(auto& c : r) {
        c.text = c.text.substr(0, c.text.size() - 2);
    }
    return r;
}
} // namespace

auto Imgview::do_action(const Actions action, const uint32_t key) -> void {
    switch(action) {
    case Actions::NONE:
        break;
    case Actions::QUIT_APP:
        std::quick_exit(0); // hack
        quit_application();
        break;
    case Actions::NEXT_WORK:
    case Actions::PREV_WORK: {
        const auto reverse           = action == Actions::PREV_WORK;
        auto       directory_changed = false;
        {
            const auto lock = image_files.get_lock();
            if(check_existence(reverse)) {
                const auto next_directory = get_next_directory(image_files->get_base().data(), action == Actions::PREV_WORK, root.data());
                if(next_directory) {
                    image_files.data  = get_sorted_images(next_directory->data());
                    directory_changed = true;
                }
            }
        }
        if(directory_changed) {
            loader_event.wakeup();
            refresh();
        }
    } break;
    case Actions::NEXT_PAGE:
    case Actions::PREV_PAGE: {
        const auto reverse    = action == Actions::PREV_PAGE;
        auto       do_refresh = false;
        {
            auto       do_update = true;
            const auto lock      = image_files.get_lock();
            do {
                const auto current_index = image_files->get_index();
                if((!reverse && current_index + 1 >= image_files->size()) || (reverse && current_index <= 0)) {
                    break;
                }
                const auto next_index = current_index + (!reverse ? 1 : -1);
                const auto next_path  = image_files.data[next_index];
                if(!std::filesystem::exists(next_path)) {
                    break;
                }
                image_files->set_index(next_index);
                do_update  = false;
                do_refresh = true;
            } while(0);
            if(do_update) {
                if(check_existence(reverse)) {
                    if(auto updated = get_next_image_file(image_files->get_current().data(), reverse)) {
                        image_files.data = std::move(updated.value());
                        do_refresh       = true;
                    }
                }
            }
        }
        if(do_refresh) {
            loader_event.wakeup();
            refresh();
        }
    } break;
    case Actions::REFRESH_FILES: {
        const auto lock = image_files.get_lock();
        if(check_existence(false)) {
            const auto lock  = image_files.get_lock();
            image_files.data = get_sorted_images(Path(image_files->get_current()).parent_path().string().data());
        }
    } break;
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
        page_select     = false;
        auto do_loading = false;
        {
            const auto lock = image_files.get_lock();
            if(check_existence(false)) {
                const auto files = get_sorted_images(image_files->get_base().data());
                if(const auto p = std::atoi(page_select_buffer.data()) - 1; p >= 0 && static_cast<long unsigned int>(p) < files.size()) {
                    image_files.data = std::move(files);
                    image_files->set_index(p);
                    do_loading = true;
                }
            }
        }
        if(do_loading) {
            loader_event.wakeup();
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
            const auto if_lock = image_files.get_lock();
            const auto im_lock = image_cache.get_lock();
            const auto path    = image_files->get_current();
            if(!image_cache->contains(path)) {
                break;
            }
            const auto& current = image_cache.data[path];

            reset_draw_pos();
            const auto  size  = std::array{current.graphic.get_width(this), current.graphic.get_height(this)};
            const auto& wsize = get_window_size();
            const auto  area  = gawl::calc_fit_rect({{0, 0}, {1. * wsize[0], 1. * wsize[1]}}, size[0], size[1]);
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
auto Imgview::calc_draw_area(const gawl::Graphic& graphic) const -> gawl::Rectangle {
    const auto size = std::array{graphic.get_width(this), graphic.get_height(this)};
    const auto exp  = std::array{size[0] * draw_scale / 2.0, size[1] * draw_scale / 2.0};
    auto       area = gawl::calc_fit_rect({{0, 0}, {1. * get_window_size()[0], 1. * get_window_size()[1]}}, size[0], size[1]);
    area.a.x += draw_offset[0] - exp[0];
    area.a.y += draw_offset[1] - exp[1];
    area.b.x += draw_offset[0] + exp[0];
    area.b.y += draw_offset[1] + exp[1];
    return area;
}
auto Imgview::zoom_draw_pos(const double value, const std::array<double, 2>& origin) -> void {
    const auto if_lock = image_files.get_lock();
    const auto im_lock = image_cache.get_lock();
    const auto path    = image_files->get_current();
    if(!image_cache->contains(path)) {
        return;
    }
    const auto& current = image_cache.data[path];

    const auto   area     = calc_draw_area(current.graphic);
    const auto   delta    = std::array{current.graphic.get_width(this) * value, current.graphic.get_height(this) * value};
    const double center_x = area.a.x + area.width() / 2;
    const double center_y = area.a.y + area.height() / 2;
    draw_offset[0] += (center_x - origin[0]) / area.width() * delta[0];
    draw_offset[1] += (center_y - origin[1]) / area.height() * delta[1];
    draw_scale += value;
}
auto Imgview::check_existence(const bool reverse) -> bool {
    switch(check_and_retrieve_paths(root.data(), image_files.data, reverse)) {
    case PathCheckResult::EXISTS:
        return true;
    case PathCheckResult::RETRIEVED:
        loader_event.wakeup();
        refresh();
        break;
    case PathCheckResult::FATAL:
        quit_application();
        break;
    }
    return false;
}
auto Imgview::is_point_in_caption(double x, double y, const Image& image) const -> std::optional<CaptionDrawHint> {
    if(!image.graphic || image.captions.empty()) {
        return std::nullopt;
    }
    const auto draw_area = calc_draw_area(image.graphic);
    const auto w         = image.graphic.get_width(gawl::nullscreen);
    const auto h         = image.graphic.get_height(gawl::nullscreen);
    const auto wr        = w / draw_area.width();
    const auto hr        = h / draw_area.height();
    x                    = (x - draw_area.a.x) * wr;
    y                    = (y - draw_area.a.y) * hr;

    for(const auto& c : image.captions) {
        if(c.area[0] <= x && c.area[1] <= y && c.area[2] >= x && c.area[3] >= y) {
            double x1 = c.area[0] / wr + draw_area.a.x;
            double y1 = c.area[1] / hr + draw_area.a.y;
            double x2 = c.area[2] / wr + draw_area.a.x;
            double y2 = c.area[3] / hr + draw_area.a.y;
            return CaptionDrawHint{c, gawl::Rectangle{{x1, y1}, {x2, y2}}, wr};
        }
    }
    return std::nullopt;
}
auto Imgview::refresh_callback() -> void {
    gawl::clear_screen({0, 0, 0, 0});
    const auto if_lock = image_files.get_lock();
    {
        const auto im_lock = image_cache.get_lock();
        const auto path    = image_files->get_current();
        if(image_cache->contains(path)) {
            auto&      current = image_cache.data[path];
            const auto area    = calc_draw_area(current.graphic);
            displayed_graphic  = current.graphic;
            displayed_graphic.draw_rect(this, area);

            if(pointer_pos.has_value()) {
                const auto& pointer = pointer_pos.value();
                const auto  caption = is_point_in_caption(pointer[0], pointer[1], current);
                if(caption.has_value()) {
                    const auto& c = caption.value().caption;
                    auto        a = caption.value().area;
                    const auto  e = caption.value().ext;
                    gawl::mask_alpha();
                    gawl::draw_rect(this, a, {0, 0, 0, 0.6});
                    caption_font.draw_wrapped(this, a.expand(-4, -4), c.size * 1.3 / e, {1, 1, 1, 1}, c.text.data(), {.alignx = c.alignx, .aligny = c.aligny, .size = static_cast<int>(c.size / e)});
                    gawl::unmask_alpha();
                }
            }
        } else {
            if(displayed_graphic) {
                const auto area = calc_draw_area(displayed_graphic);
                displayed_graphic.draw_rect(this, area);
            }
            info_font.draw_fit_rect(this, {{0, 0}, {1. * get_window_size()[0], 1. * get_window_size()[1]}}, {1, 1, 1, 1}, "loading...");
        }
    }
    if(page_select) {
        constexpr auto pagestr       = "Page: ";
        static auto    pagestr_width = int(-1);

        constexpr auto dist = 45;
        if(pagestr_width == -1) {
            pagestr_width = page_select_font.get_rect(this, {0, 0}, pagestr).width();
        }
        const auto& size = get_window_size();
        page_select_font.draw(this, {5, 1. * size[1] - dist}, {1, 1, 1, 1}, pagestr);
        page_select_font.draw(this, {1. * 5 + pagestr_width, 1. * size[1] - dist}, {1, 1, 1, 1}, page_select_buffer.data());
    }
    if(info_format != InfoFormats::NONE) {
        const auto current_path = Path(image_files->get_current());
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
        infostr << "[" << image_files->get_index() + 1 << "/" << image_files->size() << "] " << work_name;
        constexpr auto dist = 7;
        const auto&    size = get_window_size();
        {
            constexpr auto sp  = 3.0;
            const auto     rec = info_font.get_rect(this, {5.0, static_cast<double>(size[1] - dist)}, infostr.str().data());
            gawl::draw_rect(this, {{rec.a.x - sp, rec.a.y - sp}, {rec.b.x + sp, rec.b.y + sp}}, {0, 0, 0, 0.5});
        }
        info_font.draw(this, {5, 1. * size[1] - dist}, {1, 1, 1, 0.7}, infostr.str().data());
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
    auto do_refresh = false;
    {

        const auto if_lock = image_files.get_lock();
        const auto im_lock = image_cache.get_lock();
        const auto path    = image_files->get_current();
        if(!image_cache->contains(path)) {
            return;
        }
        const auto& current = image_cache.data[path];

        const auto c = is_point_in_caption(x, y, current);
        if(c.has_value()) {
            if(current_caption != &c->caption) {
                current_caption = &c->caption;
                do_refresh      = true;
            }
        } else {
            if(current_caption != nullptr) {
                current_caption = nullptr;
                do_refresh      = true;
            }
        }
    }
    if(pointer_pos.has_value()) {
        if(clicked[0]) {
            draw_offset[0] += x - pointer_pos.value()[0];
            draw_offset[1] += y - pointer_pos.value()[1];
            do_refresh = true;
        }
        if(clicked[1]) {
            zoom_draw_pos((pointer_pos.value()[1] - y) * 0.001, clicked_pos[1]);
            do_refresh = true;
        }
    }
    pointer_pos = std::array<double, 2>{x, y};
    moved       = true;
    if(do_refresh) {
        refresh();
    }
}
auto Imgview::click_callback(const uint32_t button, const gawl::ButtonState state) -> void {
    if(button != BTN_LEFT && button != BTN_RIGHT) {
        return;
    }
    clicked[button == BTN_RIGHT] = state == gawl::ButtonState::press;

    if(pointer_pos.has_value()) {
        clicked_pos[button == BTN_RIGHT][0] = pointer_pos.value()[0];
        clicked_pos[button == BTN_RIGHT][1] = pointer_pos.value()[1];
    }
    if(clicked[button == BTN_RIGHT] == false && moved == false) {
        do_action(Actions::NEXT_PAGE);
    }
    moved = false;
}
auto Imgview::scroll_callback(gawl::WheelAxis /* axis */, double value) -> void {
    constexpr auto rate = 0.00001;
    if(!pointer_pos.has_value()) {
        return;
    }
    zoom_draw_pos(rate * std::pow(value, 3), pointer_pos.value());
    refresh();
}
auto Imgview::user_callback(void* /* data */) -> void {
    auto current = std::string();
    {
        const auto lock = image_files.get_lock();
        current         = image_files->get_current();
    }

    auto       new_image_cache = ImageCache();
    const auto bf_lock         = buffer_cache.get_lock();
    const auto im_lock         = image_cache.get_lock();

    bool do_refresh = false;
    // first, reuse loaded image cache
    for(const auto& buf : buffer_cache.data) {
        if(image_cache->contains(buf.first)) {
            new_image_cache[buf.first] = std::move(image_cache.data[buf.first]);
            if(buf.first == current) {
                do_refresh = true;
            }
        }
    }

    auto itr = buffer_cache->begin(); // used later to reduce loop count

    // then, if current image is not loaded, load it
    if(!image_cache->contains(current) && buffer_cache->contains(current)) {
        new_image_cache[current] = {gawl::Graphic(buffer_cache.data[current]), read_captions((Path(current).replace_extension(".txt")).c_str())};
        do_refresh               = true;
    }

    // finally, if no image is loaded in this invocation, load single preload image
    else {
        for(; itr != buffer_cache->end(); itr++) {
            if(image_cache->contains(itr->first)) {
                continue;
            }
            new_image_cache[itr->first] = {gawl::Graphic(itr->second), read_captions((Path(itr->first).replace_extension(".txt")).c_str())};
            break;
        }
    }

    if(do_refresh) {
        refresh();
    }

    // if there is buffer_cache to load, call self later
    auto not_loaded_image_left = false;
    if(itr != buffer_cache->end()) {
        itr++;
    }
    for(; itr != buffer_cache->end(); itr++) {
        if(image_cache->contains(itr->first)) {
            continue;
        }
        not_loaded_image_left = true;
        break;
    }
    if(not_loaded_image_left) {
        invoke_user_callback();
    }

    image_cache.data = std::move(new_image_cache);
}
Imgview::Imgview(gawl::GawlApplication& app, const char* const path) : gawl::WaylandWindow({.app = app, .title = "Imgview", .manual_refresh = true}) {
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
    image_files.data = std::move(files);
    root             = dir.parent_path().string();

    loader_thread = std::thread([this]() {
        constexpr auto BUFFER_RANGE_LIMIT = 3;
        while(is_running()) {
            auto target = std::string();
            {
                const auto lock = image_files.get_lock();
                for(auto i = 0; std::abs(i) < BUFFER_RANGE_LIMIT; i == 0 ? i = 1 : (i > 0 ? i *= -1 : i = i * -1 + 1)) {
                    const auto index = static_cast<int>(image_files.data.get_index()) + i;
                    if(index < 0 || index >= static_cast<int>(image_files->size())) {
                        continue;
                    }
                    const auto name = image_files.data[index];

                    // read buffer_cache without locking
                    // since there is no thread modifying buffer_cache except loader_thread
                    if(buffer_cache->contains(name)) {
                        continue;
                    }
                    target = std::move(name);
                    break;
                }
            }
            if(!target.empty()) {
                auto buffer = gawl::PixelBuffer(target.data());

                auto       new_buffer_cache = BufferCache();
                const auto if_lock          = image_files.get_lock();
                const auto bf_lock          = buffer_cache.get_lock();

                buffer_cache.data[target] = std::move(buffer);
                for(auto i = 0; std::abs(i) < BUFFER_RANGE_LIMIT; i == 0 ? i = 1 : (i > 0 ? i *= -1 : i = i * -1 + 1)) {
                    const auto index = static_cast<int>(image_files->get_index()) + i;
                    if(index < 0 || index >= static_cast<int>(image_files->size())) {
                        continue;
                    }
                    const auto name = image_files.data[index];

                    if(buffer_cache->contains(name)) {
                        new_buffer_cache[name] = std::move(buffer_cache.data[name]);
                    }
                }
                buffer_cache.data = std::move(new_buffer_cache);
                invoke_user_callback();
            } else {
                loader_event.wait();
            }
        }
    });

    page_select_font = gawl::TextRender({"/usr/share/fonts/cascadia-code/CascadiaCode.ttf"}, 16);
    info_font        = gawl::TextRender({"/usr/share/fonts/noto-cjk/NotoSansCJK-Black.ttc"}, 16);
    caption_font     = info_font;
}
Imgview::~Imgview() {
    if(loader_thread.joinable()) {
        loader_event.wakeup();
        loader_thread.join();
    }
}
