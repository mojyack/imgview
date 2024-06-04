#include "../gawl/graphic.hpp"
#include "displayable.hpp"

struct DisplayableImage : Displayable {
    gawl::Graphic image;

    auto load(std::string_view path) -> bool override;
    auto draw(gawl::Screen* screen, const DrawParameters& params) -> void override;
    auto zoom_by_drag(gawl::Screen* screen, const gawl::Point& from, double value, DrawParameters& params) -> void override;
};
