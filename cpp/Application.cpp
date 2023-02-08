#include <Application.h>

namespace rdk {

    void Application::run() {
        onCreate();
        do {
            onUpdate();
            m_Running = !m_Window->shouldClose();
        } while (m_Running);
        onDestroy();
    }

    void Application::onFrameBufferResized(int width, int height) {
        m_Renderer->onFrameBufferResized(width, height);
    }

    void Application::onCreate() {
        m_Window = new Window("Rect", 800, 600, this);

        AppInfo appInfo {};
        appInfo.appName = "Rect";
        appInfo.appVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.engineName = "RectEngine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;
        m_Renderer = new Renderer(appInfo, m_Window);
        m_Renderer->listener = this;

        m_Renderer->addShader("shaders/shader.vert", "shaders/shader.frag");

        // todo initialize render client only after adding all shaders and objects, otherwise it's not working
        m_Renderer->initialize();

        m_Renderer->createRect();
        m_Renderer->createTexture2D("textures/statue.jpg");
        m_MVP = m_Renderer->createMVP(m_Window->getAspectRatio());
    }

    void Application::onDestroy() {
        delete m_Renderer;
        delete m_Window;
    }

    void Application::onUpdate() {
        m_Window->update();
        m_Renderer->update();
    }

    void Application::onRenderUI(float dt) {
        static bool open = true;
        ImGui::ShowDemoWindow(&open);
    }

    void Application::onRender(float dt) {
        m_Renderer->updateMVP(m_MVP);
        m_Renderer->drawIndices(Rect::INDEX_COUNT, 1);
    }
}