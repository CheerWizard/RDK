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

        AppInfo appInfo;
        appInfo.type = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.appName = "Rect";
        appInfo.appVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.engineName = "RectEngine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        m_RenderClient = new RenderClient(appInfo, m_Window);
        m_RenderClient->printExtensions();

        m_RenderClient->addShader("spirv/shader_vert.spv", "spirv/shader_frag.spv");

        VkVertexInputBindingDescription vertexBindDesc;
        vertexBindDesc.binding = 0;
        vertexBindDesc.stride = sizeof(Vertex);
        vertexBindDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        std::vector<VkVertexInputAttributeDescription> attrs {
                { 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, position) },
                { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) }
        };

        m_RenderClient->createVBO(VertexFormat { vertexBindDesc, attrs }, 3);

        // todo initialize render client only after adding all shaders and objects, otherwise it's not working
        m_RenderClient->initialize();

        std::vector<Vertex> vertices = {
                {{0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}},
                {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
                {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
        };

        m_DrawData = { vertices.size(), vertices.data(), 0, nullptr };

        m_RenderClient->uploadDrawData(m_DrawData);
    }

    void Application::onDestroy() {
        delete m_RenderClient;
        delete m_Window;
    }

    void Application::onUpdate() {
        m_Window->update();
        m_RenderClient->drawFrame(m_DrawData, 1);
    }
}