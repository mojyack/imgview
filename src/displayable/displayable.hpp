#pragma once
#include <string_view>

#include "../gawl/screen.hpp"

struct DrawParameters {
    int    screen_size[2];
    double offset[2];
    double scale;
};

struct Displayable {
    bool loaded = false;

    virtual auto load(std::string_view path) -> bool                              = 0;
    virtual auto draw(gawl::Screen* screen, const DrawParameters& params) -> void = 0;
    virtual auto zoom_by_drag(gawl::Screen* /*screen*/, const gawl::Point& /*clicked*/, double /*value*/, DrawParameters& /*params*/) -> void{};

    virtual ~Displayable() {}
};
