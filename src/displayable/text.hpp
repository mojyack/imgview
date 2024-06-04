#include "../gawl/textrender.hpp"
#include "displayable.hpp"

struct DisplayableText : Displayable {
    gawl::TextRender* font;
    std::string       text;
    gawl::WrappedText cache;

    auto load(std::string_view path) -> bool override;
    auto draw(gawl::Screen* screen, const DrawParameters& params) -> void override;

    DisplayableText(gawl::TextRender& font);
    DisplayableText(gawl::TextRender& font, std::string text);
};
