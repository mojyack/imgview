#include "gawl/wayland/application.hpp"

#include "imgview.hpp"
#include "macros/assert.hpp"
#include "util/assert.hpp"

auto main(const int argc, const char* const argv[]) -> int {
    auto callbacks = std::shared_ptr<Callbacks>(new Callbacks());
    assert_v(callbacks->init(argc, argv), 1);
    auto app = gawl::WaylandApplication();
    app.open_window({.manual_refresh = true}, callbacks);
    callbacks->run();
    app.run();
    return 0;
}
