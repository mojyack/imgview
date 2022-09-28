#pragma once
#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>

#include <gawl/wayland/gawl.hpp>

#include "fc.hpp"
#include "indexed-paths.hpp"
#include "path.hpp"
#include "util/error.hpp"
#include "util/thread.hpp"

class Imgview;

using Gawl = gawl::Gawl<Imgview>;

class Imgview {
  private:
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

    using Cache = std::unordered_map<std::string, Image>;

    struct CaptionDrawHint {
        const Caption&  caption;
        gawl::Rectangle area;
        double          ext;
    };

    Gawl::Window<Imgview>& window;
    gawl::TextRender       font;
    std::string            root;
    Critical<IndexedPaths> critical_files;
    Critical<Cache>        critical_cache;
    gawl::Graphic          displayed_graphic;
    std::thread            loader;
    Event                  loader_event;
    bool                   loader_exit = false;

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

    static auto read_captions(const char* const path) -> std::vector<Caption> {
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
        auto default_alignx = gawl::Align::Left;
        auto default_aligny = gawl::Align::Center;

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
                        align = gawl::Align::Center;
                    } else if(spec == "left") {
                        align = gawl::Align::Left;
                    } else if(spec == "right") {
                        align = gawl::Align::Right;
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

    auto do_action(const Actions action, const xkb_keysym_t key = XKB_KEY_VoidSymbol) -> void {
        switch(action) {
        case Actions::None:
            break;
        case Actions::QuitApp:
            std::quick_exit(0); // hack
            window.quit_application();
            break;
        case Actions::NextWork:
        case Actions::PrevWork: {
            const auto reverse           = action == Actions::PrevWork;
            auto       directory_changed = false;
            {
                auto [lock, files] = critical_files.access();
                if(check_existence(reverse, files)) {
                    const auto next_directory = get_next_directory(files.get_base().data(), action == Actions::PrevWork, root.data());
                    if(next_directory) {
                        files             = get_sorted_images(next_directory->data());
                        directory_changed = true;
                    }
                }
            }
            if(directory_changed) {
                loader_event.wakeup();
                window.refresh();
            }
        } break;
        case Actions::NextPage:
        case Actions::PrevPage: {
            const auto reverse    = action == Actions::PrevPage;
            auto       do_refresh = false;
            {
                auto do_update     = true;
                auto [lock, files] = critical_files.access();
                do {
                    const auto current_index = files.get_index();
                    if((!reverse && current_index + 1 >= files.size()) || (reverse && current_index <= 0)) {
                        break;
                    }
                    const auto next_index = current_index + (!reverse ? 1 : -1);
                    const auto next_path  = files[next_index];
                    if(!std::filesystem::exists(next_path)) {
                        break;
                    }
                    files.set_index(next_index);
                    do_update  = false;
                    do_refresh = true;
                } while(0);
                if(do_update) {
                    if(check_existence(reverse, files)) {
                        if(auto updated = get_next_image_file(files.get_current().data(), reverse)) {
                            files      = std::move(updated.value());
                            do_refresh = true;
                        }
                    }
                }
            }
            if(do_refresh) {
                loader_event.wakeup();
                window.refresh();
            }
        } break;
        case Actions::RefreshFiles: {
            auto [lock, files] = critical_files.access();
            if(check_existence(false, files)) {
                files = get_sorted_images(Path(files.get_current()).parent_path().string().data());
            }
        } break;
        case Actions::PageSelectOn:
            page_select = true;
            page_select_buffer.clear();
            window.refresh();
            break;
        case Actions::PageSelectOff:
            page_select = false;
            window.refresh();
            break;
        case Actions::PageSelectNum:
            page_select_buffer += (key == KEY_0 ? '0' : static_cast<char>('1' + key - KEY_1));
            window.refresh();
            break;
        case Actions::PageSelectNumDel:
            if(!page_select_buffer.empty()) {
                page_select_buffer.pop_back();
            }
            window.refresh();
            break;
        case Actions::PageSelectApply: {
            page_select     = false;
            auto do_loading = false;
            {
                auto [lock, files] = critical_files.access();
                if(check_existence(false, files)) {
                    const auto new_files = get_sorted_images(files.get_base().data());
                    if(const auto p = std::atoi(page_select_buffer.data()) - 1; p >= 0 && static_cast<long unsigned int>(p) < new_files.size()) {
                        files = std::move(new_files);
                        files.set_index(p);
                        do_loading = true;
                    }
                }
            }
            if(do_loading) {
                loader_event.wakeup();
            }
            window.refresh();
        } break;
        case Actions::ToggleShowInfo:
            switch(info_format) {
            case InfoFormats::None:
                info_format = InfoFormats::Short;
                break;
            case InfoFormats::Short:
                info_format = InfoFormats::Long;
                break;
            case InfoFormats::Long:
                info_format = InfoFormats::None;
                break;
            }
            window.refresh();
            break;
        case Actions::MoveDrawPos: {
            constexpr auto speed = 60.0;
            draw_offset[0] += key == KEY_L ? speed : key == KEY_H ? -speed
                                                                  : 0;
            draw_offset[1] += key == KEY_J ? speed : key == KEY_K ? -speed
                                                                  : 0;
            window.refresh();
        } break;
        case Actions::ResetDrawPos:
            reset_draw_pos();
            window.refresh();
            break;
        case Actions::FitWidth:
        case Actions::FitHeight: {
            {
                auto [lock0, files] = critical_files.access();
                auto [lock1, cache] = critical_cache.access();
                const auto path     = files.get_current();
                const auto p        = cache.find(path);
                if(p == cache.end()) {
                    break;
                }
                const auto& current = p->second;
                if(!current.graphic) {
                    break;
                }

                reset_draw_pos();
                const auto  size  = std::array{current.graphic.get_width(window), current.graphic.get_height(window)};
                const auto& wsize = window.get_window_size();
                const auto  area  = gawl::calc_fit_rect({{0, 0}, {1. * wsize[0], 1. * wsize[1]}}, size[0], size[1]);
                draw_scale        = action == Actions::FitWidth ? (wsize[0] - area.width()) / size[0] : (wsize[1] - area.height()) / size[1];
            }
            window.refresh();
        } break;
        }
    }
    auto reset_draw_pos() -> void {
        draw_offset[0] = 0;
        draw_offset[1] = 0;
        draw_scale     = 0.0;
    }
    auto calc_draw_area(const gawl::Graphic& graphic) const -> gawl::Rectangle {
        const auto size = std::array{graphic.get_width(window), graphic.get_height(window)};
        const auto exp  = std::array{size[0] * draw_scale / 2.0, size[1] * draw_scale / 2.0};
        auto       area = gawl::calc_fit_rect({{0, 0}, {1. * window.get_window_size()[0], 1. * window.get_window_size()[1]}}, size[0], size[1]);
        area.a.x += draw_offset[0] - exp[0];
        area.a.y += draw_offset[1] - exp[1];
        area.b.x += draw_offset[0] + exp[0];
        area.b.y += draw_offset[1] + exp[1];
        return area;
    }
    auto zoom_draw_pos(const double value, const gawl::Point& origin) -> void {
        auto [lock0, files] = critical_files.access();
        auto [lock1, cache] = critical_cache.access();
        const auto path     = files.get_current();
        const auto p        = cache.find(path);
        if(p == cache.end()) {
            return;
        }
        const auto& current = p->second;
        if(!current.graphic) {
            return;
        }

        const auto   area     = calc_draw_area(current.graphic);
        const auto   delta    = std::array{current.graphic.get_width(window) * value, current.graphic.get_height(window) * value};
        const double center_x = area.a.x + area.width() / 2;
        const double center_y = area.a.y + area.height() / 2;
        draw_offset[0] += (center_x - origin.x) / area.width() * delta[0];
        draw_offset[1] += (center_y - origin.y) / area.height() * delta[1];
        draw_scale += value;
    }

    auto check_existence(const bool reverse, IndexedPaths& files) -> bool {
        switch(check_and_retrieve_paths(root.data(), files, reverse)) {
        case PathCheckResult::Exists:
            return true;
        case PathCheckResult::Retrieved:
            loader_event.wakeup();
            window.refresh();
            break;
        case PathCheckResult::Fatal:
            window.quit_application();
            break;
        }
        return false;
    }
    auto is_point_in_caption(gawl::Point point, const Image& image) const -> std::optional<CaptionDrawHint> {
        if(!image.graphic || image.captions.empty()) {
            return std::nullopt;
        }
        const auto draw_area = calc_draw_area(image.graphic);
        const auto w         = image.graphic.get_width(gawl::NullScreen());
        const auto h         = image.graphic.get_height(gawl::NullScreen());
        const auto wr        = w / draw_area.width();
        const auto hr        = h / draw_area.height();
        point.x              = (point.x - draw_area.a.x) * wr;
        point.y              = (point.y - draw_area.a.y) * hr;

        for(const auto& c : image.captions) {
            if(c.area[0] <= point.x && c.area[1] <= point.y && c.area[2] >= point.x && c.area[3] >= point.y) {
                const auto x1 = c.area[0] / wr + draw_area.a.x;
                const auto y1 = c.area[1] / hr + draw_area.a.y;
                const auto x2 = c.area[2] / wr + draw_area.a.x;
                const auto y2 = c.area[3] / hr + draw_area.a.y;
                return CaptionDrawHint{c, gawl::Rectangle{{x1, y1}, {x2, y2}}, wr};
            }
        }
        return std::nullopt;
    }

  public:
    auto refresh_callback() -> void {
        gawl::clear_screen({0, 0, 0, 0});
        const auto [width, height] = window.get_window_size();
        const auto [lock, files]   = critical_files.access();
        if(files.empty()) {
            return;
        }
        const auto path = files.get_current();
        {
            const auto [lock, cache] = critical_cache.access();
            if(const auto p = cache.find(path); p != cache.end()) {
                auto& current = p->second;
                if(!current.graphic) {
                    font.draw_fit_rect(window, {{0, 0}, {1. * width, 1. * height}}, {1, 1, 1, 1}, "broken image");
                } else {
                    const auto area   = calc_draw_area(current.graphic);
                    displayed_graphic = current.graphic;
                    displayed_graphic.draw_rect(window, area);

                    if(pointer_pos.has_value()) {
                        const auto caption = is_point_in_caption(*pointer_pos, current);
                        if(caption.has_value()) {
                            const auto& c = caption.value().caption;
                            auto        a = caption.value().area;
                            const auto  e = caption.value().ext;
                            gawl::mask_alpha();
                            gawl::draw_rect(window, a, {0, 0, 0, 0.6});
                            font.draw_wrapped(window, a.expand(-4, -4), c.size * 1.3 / e, {1, 1, 1, 1}, c.text.data(), static_cast<int>(c.size / e), c.alignx, c.aligny);
                            gawl::unmask_alpha();
                        }
                    }
                }
            } else {
                if(displayed_graphic) {
                    const auto area = calc_draw_area(displayed_graphic);
                    displayed_graphic.draw_rect(window, area);
                }
                font.draw_fit_rect(window, {{0, 0}, {1. * width, 1. * height}}, {1, 1, 1, 1}, "loading...");
            }
        }
        auto top = 0.0;
        if(info_format != InfoFormats::None) {
            const auto current_path = Path(path);
            auto       work_name    = std::string();
            switch(info_format) {
            case InfoFormats::Short:
                work_name = current_path.parent_path().filename().string() + "/" + current_path.filename().string();
                break;
            case InfoFormats::Long:
                work_name = std::filesystem::relative(current_path, root).string();
                break;
            default:
                break;
            }
            const auto str  = build_string("[", files.get_index() + 1, "/", files.size(), "]", work_name);
            const auto rect = gawl::Rectangle(font.get_rect(window, str.data())).expand(2, 2);
            const auto box  = gawl::Rectangle{{0, height - rect.height() - top}, {rect.width(), height - top}};
            gawl::draw_rect(window, box, {0, 0, 0, 0.5});
            font.draw_fit_rect(window, box, {1, 1, 1, 0.7}, str.data());
            top += rect.height();
        }
        if(page_select) {
            const auto str  = std::string("Page: ") + page_select_buffer;
            const auto rect = font.get_rect(window, str.data());
            const auto box  = gawl::Rectangle{{0, height - rect.height() - top}, {rect.width(), height - top}};
            font.draw_fit_rect(window, box, {1, 1, 1, 0.7}, str.data());
            top += rect.height();
        }
    }
    auto window_resize_callback() -> void {
        reset_draw_pos();
    }
    auto keysym_callback(const xkb_keycode_t key, const gawl::ButtonState state, xkb_state* const /*xkb_state*/) -> void {
        const static auto num_keys = std::vector<uint32_t>{KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9};
        struct KeyBind {
            Actions                   action;
            std::vector<uint32_t>     keys;
            std::function<bool(void)> condition = nullptr;
        };
        const static KeyBind keybinds[] = {
            {Actions::QuitApp, {KEY_Q, KEY_BACKSLASH}},
            {Actions::NextWork, {KEY_DOWN}},
            {Actions::PrevWork, {KEY_UP}},
            {Actions::NextPage, {KEY_X, KEY_RIGHT, KEY_SPACE}},
            {Actions::PrevPage, {KEY_Z, KEY_PAGEDOWN}},
            {Actions::RefreshFiles, {KEY_R}},
            {Actions::PageSelectOn, {KEY_P}, [this]() -> bool { return !page_select; }},
            {Actions::PageSelectOff, {KEY_ESC, KEY_P}, [this]() -> bool { return page_select; }},
            {Actions::PageSelectNum, num_keys, [this]() -> bool { return page_select; }},
            {Actions::PageSelectNumDel, {KEY_BACKSPACE}, [this]() -> bool { return page_select; }},
            {Actions::PageSelectApply, {KEY_ENTER}, [this]() -> bool { return page_select; }},
            {Actions::ToggleShowInfo, {KEY_F}},
            {Actions::MoveDrawPos, {KEY_H, KEY_J, KEY_K, KEY_L}},
            {Actions::ResetDrawPos, {KEY_0}},
            {Actions::FitWidth, {KEY_1}},
            {Actions::FitHeight, {KEY_2}},
        };
        using enum gawl::ButtonState;
        if(state == Enter || state == Leave || state == Release) {
            return;
        }
        auto action = Actions::None;
        for(const auto& a : keybinds) {
            for(const auto k : a.keys) {
                if(k == key - 8 && (!a.condition || a.condition())) {
                    action = a.action;
                    break;
                }
            }
            if(action != Actions::None) {
                break;
            }
        }
        do_action(action, key - 8);
    }
    auto pointermove_callback(const gawl::Point& point) -> void {
        auto do_refresh = false;
        {

            auto [lock0, files] = critical_files.access();
            auto [lock1, cache] = critical_cache.access();
            const auto path     = files.get_current();
            const auto p        = cache.find(path);
            if(p == cache.end()) {
                return;
            }
            const auto& current = p->second;

            const auto c = is_point_in_caption(point, current);
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
                draw_offset[0] += point.x - pointer_pos->x;
                draw_offset[1] += point.y - pointer_pos->y;
                do_refresh = true;
            }
            if(clicked[1]) {
                zoom_draw_pos((pointer_pos->y - point.y) * 0.001, clicked_pos[1]);
                do_refresh = true;
            }
        }
        pointer_pos = point;
        moved       = true;
        if(do_refresh) {
            window.refresh();
        }
    }
    auto click_callback(const uint32_t button, const gawl::ButtonState state) -> void {
        if(button != BTN_LEFT && button != BTN_RIGHT) {
            return;
        }
        clicked[button == BTN_RIGHT] = state == gawl::ButtonState::Press;

        if(pointer_pos.has_value()) {
            clicked_pos[button == BTN_RIGHT].x = pointer_pos->x;
            clicked_pos[button == BTN_RIGHT].y = pointer_pos->y;
        }
        if(clicked[button == BTN_RIGHT] == false && moved == false) {
            do_action(Actions::NextPage);
        }
        moved = false;
    }
    auto scroll_callback(gawl::WheelAxis /* axis */, const double value) -> void {
        constexpr auto rate = 0.00001;
        if(!pointer_pos.has_value()) {
            return;
        }
        zoom_draw_pos(rate * std::pow(value, 3), pointer_pos.value());
        window.refresh();
    }

    Imgview(Gawl::Window<Imgview>& window, const char* const path) : window(window) {
        if(!std::filesystem::exists(path)) {
            exit(1);
            window.quit_application();
            return;
        }
        const auto arg       = std::filesystem::absolute(path);
        const auto dir       = std::filesystem::is_directory(arg) ? arg : arg.parent_path();
        auto       new_files = get_sorted_images(dir.string().data());
        if(new_files.empty()) {
            std::cerr << "empty directory." << std::endl;
            window.quit_application();
            return;
        }
        if(is_regular_file(arg)) {
            const auto filename = arg.filename().string();
            if(!new_files.set_index_by_name(filename.data())) {
                std::cerr << "no such file." << std::endl;
                window.quit_application();
                return;
            }
        }
        critical_files.unsafe_access() = std::move(new_files);
        root                           = dir.parent_path().string();

        loader = std::thread([this, &window]() {
            constexpr auto BUFFER_RANGE_LIMIT = 3;

            auto context = window.fork_context();
            while(!loader_exit) {
                auto target = std::string();
                {
                    auto [lock0, files] = critical_files.access();
                    auto [lock1, cache] = critical_cache.access();
                    for(auto i = 0; std::abs(i) < BUFFER_RANGE_LIMIT; i == 0 ? i = 1 : (i > 0 ? i *= -1 : i = i * -1 + 1)) {
                        const auto index = static_cast<int>(files.get_index()) + i;
                        if(index < 0 || index >= static_cast<int>(files.size())) {
                            continue;
                        }

                        auto name = files[index];
                        if(!cache.contains(name)) {
                            target = std::move(name);
                            break;
                        }
                    }
                }
                if(!target.empty()) {
                    auto graphic = gawl::Graphic(target.data());
                    context.wait();

                    auto [lock0, files] = critical_files.access();
                    auto [lock1, cache] = critical_cache.access();

                    auto new_cache = Cache();

                    for(auto i = 0; std::abs(i) < BUFFER_RANGE_LIMIT; i == 0 ? i = 1 : (i > 0 ? i *= -1 : i = i * -1 + 1)) {
                        const auto index = static_cast<int>(files.get_index()) + i;
                        if(index < 0 || index >= static_cast<int>(files.size())) {
                            continue;
                        }

                        const auto name = files[index];
                        if(auto p = cache.find(name); p != cache.end()) {
                            new_cache[name] = std::move(p->second);
                        } else if(name == target) {
                            new_cache[name] = Image{graphic, read_captions((Path(name).replace_extension(".txt")).c_str())};
                        }
                    }
                    cache = std::move(new_cache);
                    window.refresh();
                } else {
                    loader_event.wait();
                }
            }
        });

        const auto font_path = fc::find_fontpath_from_name("Noto Sans CJK JP:style=Bold");
        if(font_path.empty()) {
            throw std::runtime_error("failed to find font");
        }
        font = gawl::TextRender({font_path.data()}, 16);
    }
    ~Imgview() {
        if(loader.joinable()) {
            loader_exit = true;
            loader_event.wakeup();
            loader.join();
        }
    }
};
