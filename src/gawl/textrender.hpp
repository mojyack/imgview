#pragma once
#include <functional>
#include <map>
#include <memory>
#include <string>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "graphic-base.hpp"
#include "type.hpp"

namespace gawl {
class TextRenderPrivate;
class Character;
class TextRender {
  private:
    std::shared_ptr<TextRenderPrivate> data;
    Character*                         get_chara_graphic(int size, char32_t c);

  public:
    using DrawFunc = std::function<bool(size_t, Area const&, GraphicBase&)>;
    static void set_char_color(Color const& color);

    Area draw(const GawlWindow* window, double x, double y, Color const& color, const char* text, DrawFunc func = nullptr);
    Area draw_fit_rect(const GawlWindow* window, Area rect, Color const& color, const char* text, Align alignx = Align::center, Align aligny = Align::center, DrawFunc func = nullptr);
    void get_rect(const GawlWindow* window, Area& rect, const char* text);
    TextRender(const std::vector<const char*>&& font_names, int size);
    TextRender();
    ~TextRender();
};
} // namespace gawl
