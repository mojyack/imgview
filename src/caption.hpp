#pragma once
#include "type.hpp"

inline auto read_captions(const char* const path) -> std::vector<Caption> {
    if(!std::filesystem::is_regular_file(path)) {
        return std::vector<Caption>();
    }

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
