#pragma once
#include <cstdint>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "graphic-base.hpp"
#include "type.hpp"

namespace gawl {
struct GlobalVar {
    Shader*    graphic_shader; // dirty way
    Shader*    textrender_shader;
    FT_Library freetype = nullptr;
    GlobalVar();
    ~GlobalVar();
};
} // namespace gawl
