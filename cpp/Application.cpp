#include <Application.h>

#include <vulkan/vulkan.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>

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
        while (!m_Window->shouldClose()) {
            onUpdate();
        }
        onDestroy();
    }

    void Application::onCreate() {
        m_Window = new Window("Rect", 800, 600);

        AppInfo appInfo;
        appInfo.type = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.appName = "Rect";
        appInfo.appVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.engineName = "RectEngine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        m_GraphicsInstance = new GraphicsInstance(appInfo, m_Window);
        m_GraphicsInstance->printExtensions();
    }

    void Application::onDestroy() {
        delete m_GraphicsInstance;
        delete m_Window;
    }

    void Application::onUpdate() {
        m_Window->update();
    }

    Window::Window(const char *title, int width, int height) : m_Title(title), m_Width(width), m_Height(height) {
        int status = glfwInit();
        assert(status);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_Handle = glfwCreateWindow(m_Width, m_Height, m_Title, nullptr, nullptr);

        u32 extensionCount = 0;
        const char** extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
        for (u32 i = 0 ; i < extensionCount ; i++) {
            m_Extensions.emplace_back(extensions[i]);
        }
    }

    Window::~Window() {
        glfwDestroyWindow((GLFWwindow*) m_Handle);
        glfwTerminate();
    }

    void Window::setResizable(Resizable resizable) {
        glfwWindowHint(GLFW_RESIZABLE, resizable);
    }

    void Window::update() {
        glfwPollEvents();
    }

    bool Window::shouldClose() {
        return glfwWindowShouldClose((GLFWwindow*) m_Handle);
    }

    const std::vector<const char *>& Window::getExtensions() {
        return m_Extensions;
    }

    void Window::addExtension(const char *&&extension) {
        m_Extensions.emplace_back(extension);
    }

    GraphicsInstance::GraphicsInstance(const AppInfo &appInfo, Window* window) : m_AppInfo(appInfo) {
        VkApplicationInfo vkAppInfo;
        vkAppInfo.sType = static_cast<VkStructureType>(m_AppInfo.type);
        vkAppInfo.pNext = m_AppInfo.next;
        vkAppInfo.pApplicationName = m_AppInfo.appName;
        vkAppInfo.applicationVersion = m_AppInfo.appVersion;
        vkAppInfo.pEngineName = m_AppInfo.engineName;
        vkAppInfo.engineVersion = m_AppInfo.engineVersion;
        vkAppInfo.apiVersion = m_AppInfo.apiVersion;

        u32 extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensionProps(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionProps.data());
        for (const auto& vkProp : extensionProps) {
            m_ExtensionProps.emplace_back(vkProp.extensionName, vkProp.specVersion);
        }

        VkInstanceCreateInfo createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.pApplicationInfo = &vkAppInfo;
        createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

        createInfo.ppEnabledExtensionNames = window->getExtensions().data();
        createInfo.enabledExtensionCount = static_cast<u32>(window->getExtensions().size());

        createInfo.enabledLayerCount = 0;

        if (vkCreateInstance(&createInfo, nullptr, (VkInstance*) &m_Instance) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create GraphicsInstance!");
        }
    }

    GraphicsInstance::~GraphicsInstance() {
        vkDestroyInstance((VkInstance) m_Instance, nullptr);
    }

    void GraphicsInstance::printExtensions() {
        std::cout << "available extensions:\n";
        for (const auto& extension : m_ExtensionProps) {
            std::cout << '\t' << extension.name << '\n';
        }
    }
}