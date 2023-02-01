#include <Window.h>

#include <GLFW/glfw3.h>

namespace rdk {

    u8 Resizable::FALSE = GLFW_FALSE;
    u8 Resizable::TRUE = GLFW_TRUE;

    Window::Window(const char *title, int width, int height, WindowListener* listener)
    : m_Title(title), m_Width(width), m_Height(height) {
        int status = glfwInit();
        rect_assert(status, "Failed to setup GLFW\n")

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_Handle = glfwCreateWindow(m_Width, m_Height, m_Title, nullptr, nullptr);

        u32 extensionCount = 0;
        const char **extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
        for (u32 i = 0; i < extensionCount; i++) {
            m_Extensions.emplace_back(extensions[i]);
        }

        glfwSetWindowUserPointer((GLFWwindow*) m_Handle, listener);
        glfwSetFramebufferSizeCallback((GLFWwindow*) m_Handle, [](GLFWwindow* window, int width, int height) {
            WindowListener* listener = (WindowListener*) glfwGetWindowUserPointer(window);
            listener->onFrameBufferResized(width, height);
        });
    }

    Window::~Window() {
        glfwDestroyWindow((GLFWwindow *) m_Handle);
        glfwTerminate();
    }

    void Window::setResizable(u8 resizable) {
        glfwWindowHint(GLFW_RESIZABLE, resizable);
    }

    void Window::update() {
        glfwPollEvents();
    }

    bool Window::shouldClose() {
        return glfwWindowShouldClose((GLFWwindow*) m_Handle);
    }

    void Window::addExtension(const char *&&extension) {
        m_Extensions.emplace_back(extension);
    }

}