#include "imgview.hpp"
#include "path.hpp"

int main(const int argc, const char* const argv[]) {
    if(argc != 2) {
        return 0;
    }
    auto app = gawl::WaylandApplication();
    app.open_window<Imgview>(argv[1]);
    app.run();
    return 0;
}
