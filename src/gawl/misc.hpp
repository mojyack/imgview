#pragma once
#include "type.hpp"

namespace gawl {
class GawlWindow;

void convert_screen_to_viewport(const GawlWindow* window, Area& area);
Area calc_fit_rect(const Area& area, const double width, const double height, const Align horizontal = Align::center, const Align vertical = Align::center);
void clear_screen(const Color& color);
void draw_rect(const GawlWindow* window, Area area, const Color& color);
} // namespace gawl
