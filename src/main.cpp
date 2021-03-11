#include "imgview.hpp"
#include "path.hpp"

int main(int argc, char* argv[]) {
    if(argc != 2) return 0;
    gawl::WaylandApplication app;
    app.open_window<Imgview>(argv[1]);
    app.run();
    return 0;
}
