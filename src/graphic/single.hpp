#pragma once
#include <gawl/graphic.hpp>

#include "../util/error.hpp"

namespace graphic::single {
class SingleGraphic {
  private:
    gawl::Graphic graphic;

    SingleGraphic(const gawl::PixelBuffer& pixelbuffer) : graphic(pixelbuffer) {}

  public:
    auto get_graphic() -> gawl::Graphic* {
        return &graphic;
    }

    static auto from_file(const char* const path) -> Result<SingleGraphic> {
        const auto pixelbuffer = gawl::PixelBuffer::from_file(path);
        if(!pixelbuffer) {
            return Error(pixelbuffer.as_error().cstr());
        }
        return SingleGraphic(pixelbuffer.as_value());
    }
};
} // namespace graphic::single
