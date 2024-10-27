#include "image.hpp"
#include "../gawl/misc.hpp"
#include "../macros/unwrap.hpp"

namespace {
auto calc_draw_area(const gawl::Graphic& graphic, gawl::Screen* const screen, const DrawParameters& params) -> gawl::Rectangle {
    const auto size = std::array{graphic.get_width(*screen), graphic.get_height(*screen)};
    auto       area = gawl::calc_fit_rect({{0, 0}, {1. * params.screen_size[0], 1. * params.screen_size[1]}}, size[0], size[1]);
    for(auto i = 0; i < 2; i += 1) {
        const auto exp = size[i] * params.scale / 2;
        (i == 0 ? area.a.x : area.a.y) += params.offset[i] - exp;
        (i == 0 ? area.b.x : area.b.y) += params.offset[i] + exp;
    }
    return area;
}
} // namespace

auto DisplayableImage::load(const std::string_view path) -> bool {
    unwrap(pixbuf, gawl::PixelBuffer::from_file(std::string(path).data()));
    image = gawl::Graphic(pixbuf);
    return true;
}

auto DisplayableImage::draw(gawl::Screen* const screen, const DrawParameters& params) -> void {
    const auto rect = calc_draw_area(image, screen, params);
    image.draw_rect(*screen, rect);
}

auto DisplayableImage::zoom_by_drag(gawl::Screen* screen, const gawl::Point& clicked, const double value, DrawParameters& params) -> void {
    const auto   area     = calc_draw_area(image, screen, params);
    const auto   delta    = std::array{image.get_width(*screen) * value, image.get_height(*screen) * value};
    const double center_x = area.a.x + area.width() / 2;
    const double center_y = area.a.y + area.height() / 2;
    params.offset[0] += (center_x - clicked.x) / area.width() * delta[0];
    params.offset[1] += (center_y - clicked.y) / area.height() * delta[1];
    params.scale += value;
}
