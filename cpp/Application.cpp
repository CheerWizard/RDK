#include <Application.h>

namespace rdk {

    Application *Application::s_Instance = nullptr;

    Application::Application() {
        s_Instance = this;
    }

    Application::~Application() {
        s_Instance = nullptr;
    }

    void Application::run() {
        onCreate();
        do {
            onUpdate();
            m_Running = !m_Window->shouldClose();
        } while (m_Running);
        onDestroy();
    }

    void Application::onFrameBufferResized(int width, int height) {
        m_RenderClient->onFrameBufferResized(width, height);
    }

    void Application::onCreate() {
        m_Window = new Window("Rect", 800, 600, this);

        AppInfo appInfo {};
        appInfo.appName = "Rect";
        appInfo.appVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.engineName = "RectEngine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        m_RenderClient = new RenderClient(appInfo, m_Window);

        m_RenderClient->addShader("spirv/shader_vert.spv", "spirv/shader_frag.spv");

        // todo initialize render client only after adding all shaders and objects, otherwise it's not working
        m_RenderClient->initialize();

        m_RenderClient->createRect();
        m_MVP = m_RenderClient->createMVP(m_Window->getAspectRatio());
    }

    void Application::onDestroy() {
        delete m_RenderClient;
        delete m_Window;
    }

    void Application::onUpdate() {
        m_Window->update();
        // render 3D rect
        m_RenderClient->updateMVP(m_MVP);
        m_RenderClient->beginFrame();
        m_RenderClient->drawIndices(Rect::INDEX_COUNT, 1);
        m_RenderClient->endFrame();
    }
}