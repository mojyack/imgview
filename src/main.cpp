#include "gawl/wayland/application.hpp"

#include "imgview.hpp"
#include "macros/assert.hpp"

auto main(const int argc, const char* const argv[]) -> int {
    auto cbs = std::shared_ptr<Callbacks>(new Callbacks());
    ensure(cbs->init(argc, argv));

    auto runner = coop::Runner();
    auto app    = gawl::WaylandApplication();
    runner.push_task(app.run(), app.open_window({.manual_refresh = true}, std::move(cbs)));
    runner.run();
    return 0;
}
