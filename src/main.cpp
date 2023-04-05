#define GAWL_KEYSYM
#define GAWL_MOUSE
#include <gawl/wayland/gawl.hpp>

#include "imgview.hpp"

int main(const int argc, const char* const argv[]) {
    if(argc != 2) {
        return 0;
    }

    auto app = gawl::Application();
    app.open_window<Imgview>({.title = "Imgview", .manual_refresh = true}, argv[1]);
    app.run();
    return 0;
}
