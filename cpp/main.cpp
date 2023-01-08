#include <Application.h>

using namespace rect;

int main() {
    auto* app = new Application();
    app->run();
    delete app;
    return 0;
}
