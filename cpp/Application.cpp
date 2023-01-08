#include <Application.h>

namespace rect {

    Application* Application::s_Instance = nullptr;

    Application::Application() {
        s_Instance = this;
    }

    Application::~Application() {
        s_Instance = nullptr;
    }

    void Application::run() {
        onCreate();
        while (m_Running) {
            onUpdate();
        }
        onDestroy();
    }

    void Application::onCreate() {

    }

    void Application::onDestroy() {

    }

    void Application::onUpdate() {

    }

}