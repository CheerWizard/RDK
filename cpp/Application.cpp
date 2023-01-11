#include <Application.h>

#include <vulkan/vulkan.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include <cstring>

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
        if (!status) {
            std::cerr << "Failed to setup GLFW" << std::endl;
            breakpoint();
        }

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

#if DEBUG
#define VALIDATION_LAYERS
#endif

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData
    ) {
        std::cerr << "Vulkan DEBUG callback: " << pCallbackData->pMessage << std::endl;
        breakpoint();
        return VK_FALSE;
    }

    static VkResult createDebugUtilsMessenger(
            VkInstance instance,
            const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkDebugUtilsMessengerEXT* pDebugMessenger
    ) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    static void destroyDebugUtilsMessenger(
            VkInstance instance,
            VkDebugUtilsMessengerEXT debugMessenger,
            const VkAllocationCallbacks* pAllocator
    ) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }

    static void setDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                                     | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                     | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                                 | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                 | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    GraphicsInstance::GraphicsInstance(const AppInfo &appInfo, Window* window) : m_AppInfo(appInfo) {
        // Layers validation should be supported in DEBUG mode, otherwise throws Runtime error.
#ifdef VALIDATION_LAYERS
        if (!isLayerValidationSupported()) {
            throw std::runtime_error("Layer validation not supported!");
        }
#endif
        // setup AppInfo
        VkApplicationInfo vkAppInfo {};
        vkAppInfo.sType = static_cast<VkStructureType>(m_AppInfo.type);
        vkAppInfo.pNext = m_AppInfo.next;
        vkAppInfo.pApplicationName = m_AppInfo.appName;
        vkAppInfo.applicationVersion = m_AppInfo.appVersion;
        vkAppInfo.pEngineName = m_AppInfo.engineName;
        vkAppInfo.engineVersion = m_AppInfo.engineVersion;
        vkAppInfo.apiVersion = m_AppInfo.apiVersion;
        // eval instance extensions
        u32 extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensionProps(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionProps.data());
        for (const auto& vkProp : extensionProps) {
            m_ExtensionProps.emplace_back(vkProp.extensionName, vkProp.specVersion);
        }
#ifdef VALIDATION_LAYERS
        window->addExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
        // setup creation info
        VkInstanceCreateInfo createInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &vkAppInfo;
        createInfo.ppEnabledExtensionNames = window->getExtensions().data();
        createInfo.enabledExtensionCount = static_cast<u32>(window->getExtensions().size());
#ifdef VALIDATION_LAYERS
        createInfo.enabledLayerCount = static_cast<u32>(m_ValidationLayers.size());
        createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        setDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
#else
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
#endif
        // try to create instance, otherwise throws Runtime error
        if (vkCreateInstance(&createInfo, nullptr, (VkInstance*) &m_Instance) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create GraphicsInstance!");
        }
#ifdef VALIDATION_LAYERS
        createDebugger();
#endif
    }

    GraphicsInstance::~GraphicsInstance() {
#ifdef VALIDATION_LAYERS
        destroyDebugger();
#endif
        vkDestroyInstance((VkInstance) m_Instance, nullptr);
    }

    void GraphicsInstance::printExtensions() {
        std::cout << "available extensions:\n";
        for (const auto& extension : m_ExtensionProps) {
            std::cout << '\t' << extension.name << '\n';
        }
    }

    bool GraphicsInstance::isLayerValidationSupported() {
        m_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };

        u32 layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : m_ValidationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }

    void GraphicsInstance::createDebugger() {
        VkDebugUtilsMessengerEXT debugMessenger;
        m_Debugger = &debugMessenger;

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        setDebugMessengerCreateInfo(createInfo);

        if (createDebugUtilsMessenger((VkInstance) m_Instance,&createInfo,nullptr,(VkDebugUtilsMessengerEXT*) m_Debugger) != VK_SUCCESS) {
            throw std::runtime_error("Failed to setup Vulkan debugger");
        }
    }

    void GraphicsInstance::destroyDebugger() {
//        destroyDebugUtilsMessenger((VkInstance) m_Instance, (VkDebugUtilsMessengerEXT) m_Debugger, nullptr);
    }

}