#include <Application.h>

using namespace rdk;

int main() {
    auto* app = new Application();
    app->run();
    delete app;
    return 0;
}
