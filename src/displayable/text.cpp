#include <fstream>

#include "../gawl/misc.hpp"
#include "../macros/assert.hpp"
#include "text.hpp"

auto DisplayableText::load(const std::string_view path) -> bool {
    auto in = std::ifstream();
    try {
        in.open(path, std::ios::binary);
    } catch(const std::runtime_error& e) {
        bail(e.what());
    }

    text = std::string(std::istreambuf_iterator<char>(in), {});
    return true;
}

auto DisplayableText::draw(gawl::Screen* const screen, const DrawParameters& params) -> void {
    constexpr auto font_size = 16;

    const auto content_height = font->calc_wrapped_text_height(*screen, params.screen_size[0], font_size * 1.3, text, cache, font_size);
    const auto rect           = gawl::Rectangle{{0, params.offset[1]}, {1. * params.screen_size[0], params.offset[1] + content_height}};

    gawl::draw_rect(*screen, rect, {0, 0, 0, 0.6});
    gawl::mask_alpha();
    font->draw_wrapped(*screen, rect, font_size * 1.3, {1, 1, 1, 1}, text, cache, {
                                                                                      .size    = font_size,
                                                                                      .align_x = gawl::Align::Left,
                                                                                      .align_y = gawl::Align::Left,
                                                                                  });
    gawl::unmask_alpha();
}

DisplayableText::DisplayableText(gawl::TextRender& font)
    : font(&font) {}

DisplayableText::DisplayableText(gawl::TextRender& font, std::string text)
    : font(&font), text(std::move(text)) {}
