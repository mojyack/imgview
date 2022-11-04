#pragma once
#include <gawl/wayland/gawl.hpp>

namespace graphic::single {
class SingleGraphic {
  private:
    gawl::Graphic graphic;

  public:
    auto get_graphic() -> gawl::Graphic {
        return graphic;
    }

    SingleGraphic() = default;

    SingleGraphic(const char* const path) : graphic(path) {}
};
}
