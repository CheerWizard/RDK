#include <Application.h>
#include <Core.h>

#include <vulkan/vulkan.h>
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32

#include <stdexcept>
#include <set>
#include <limits>
#include <fstream>

namespace rect {

    Application *Application::s_Instance = nullptr;

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
        rect_assert(status, "Failed to setup GLFW\n")

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_Handle = glfwCreateWindow(m_Width, m_Height, m_Title, nullptr, nullptr);

        u32 extensionCount = 0;
        const char **extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
        for (u32 i = 0; i < extensionCount; i++) {
            m_Extensions.emplace_back(extensions[i]);
        }
    }

    Window::~Window() {
        glfwDestroyWindow((GLFWwindow *) m_Handle);
        glfwTerminate();
    }

    void Window::setResizable(Resizable resizable) {
        glfwWindowHint(GLFW_RESIZABLE, resizable);
    }

    void Window::update() {
        glfwPollEvents();
    }

    bool Window::shouldClose() {
        return glfwWindowShouldClose((GLFWwindow *) m_Handle);
    }

    const std::vector<const char *> &Window::getExtensions() {
        return m_Extensions;
    }

    void Window::addExtension(const char *&&extension) {
        m_Extensions.emplace_back(extension);
    }

    void *Window::getHandle() {
        return m_Handle;
    }

#if DEBUG
#define VALIDATION_LAYERS
#endif

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
            void *pUserData
    ) {
        printf("Vulkan DEBUG callback: %s\n", pCallbackData->pMessage);
        if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            breakpoint();
        }
        return VK_FALSE;
    }

    static VkResult createDebugUtilsMessenger(
            VkInstance instance,
            const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
            const VkAllocationCallbacks *pAllocator,
            VkDebugUtilsMessengerEXT *pDebugMessenger
    ) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
                                                                               "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    static void destroyDebugUtilsMessenger(
            VkInstance instance,
            VkDebugUtilsMessengerEXT debugMessenger,
            const VkAllocationCallbacks *pAllocator
    ) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
                                                                                "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }

    static void setDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
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

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    static SwapChainSupportDetails querySwapChainSupport(void* physicalDevice, void* surfaceHandle) {
        SwapChainSupportDetails details;
        auto device = (VkPhysicalDevice) physicalDevice;
        auto surface = (VkSurfaceKHR) surfaceHandle;
        // query base surface capabilities
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
        // query supported surface format
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }
        // query presentation modes
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    static VkSurfaceFormatKHR selectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }
        return availableFormats[0];
    }

    static VkPresentModeKHR selectSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    template<typename T>
    static T clamp(const T& value, const T& min, const T& max) {
        if (value < min)
            return min;
        else if (value >= min && value <= max)
            return value;
        else
            return max;
    }

    static VkExtent2D selectSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<u32>::max()) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {
                    static_cast<u32>(width),
                    static_cast<u32>(height)
            };

            actualExtent.width = clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    GraphicsInstance::GraphicsInstance(const AppInfo &appInfo, Window* window) : m_AppInfo(appInfo), m_Window(window) {
        // list device extensions to be supported
        m_DeviceExtensions = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
        // Layers validation should be supported in DEBUG mode, otherwise throws Runtime error.
#ifdef VALIDATION_LAYERS
        rect_assert(isLayerValidationSupported(), "Layer validation not supported!")
#endif
        // setup AppInfo
        VkApplicationInfo vkAppInfo{};
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
        for (const auto &vkProp: extensionProps) {
            m_ExtensionProps.emplace_back(vkProp.extensionName, vkProp.specVersion);
        }
#ifdef VALIDATION_LAYERS
        window->addExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
        // setup creation info
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &vkAppInfo;
        createInfo.ppEnabledExtensionNames = window->getExtensions().data();
        createInfo.enabledExtensionCount = static_cast<u32>(window->getExtensions().size());
#ifdef VALIDATION_LAYERS
        createInfo.enabledLayerCount = static_cast<u32>(m_ValidationLayers.size());
        createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        setDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugCreateInfo;
#else
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
#endif
        // try to create instance, otherwise throws Runtime error
        VkResult instanceStatus = vkCreateInstance(&createInfo, nullptr, (VkInstance*) &m_Instance);
        rect_assert(instanceStatus == VK_SUCCESS, "Failed to create GraphicsInstance!")
#ifdef VALIDATION_LAYERS
        createDebugger();
#endif
        createSurface();
        createPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createPipeline();
    }

    GraphicsInstance::~GraphicsInstance() {
        destroyPipeline();
        destroyRenderPass();
        destroyImageViews();
        destroySwapChain();
#ifdef VALIDATION_LAYERS
        destroyDebugger();
#endif
        destroySurface();
        destroyLogicalDevice();
        // this also destroys physical device associated with instance
        vkDestroyInstance((VkInstance) m_Instance, nullptr);
    }

    void GraphicsInstance::printExtensions() {
        printf("Available extensions: \n");
        for (const auto &extension: m_ExtensionProps) {
            printf("\t %s \n", extension.name.c_str());
        }
    }

    bool GraphicsInstance::isLayerValidationSupported() {
        m_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };

        u32 layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char *layerName: m_ValidationLayers) {
            bool layerFound = false;

            for (const auto &layerProperties: availableLayers) {
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
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        setDebugMessengerCreateInfo(createInfo);
        VkResult debuggerStatus = createDebugUtilsMessenger((VkInstance) m_Instance, &createInfo, nullptr,
                                                            (VkDebugUtilsMessengerEXT*) &m_Debugger);
        rect_assert(debuggerStatus == VK_SUCCESS, "Failed to setup Vulkan debugger")
    }

    void GraphicsInstance::destroyDebugger() {
        destroyDebugUtilsMessenger((VkInstance) m_Instance, (VkDebugUtilsMessengerEXT) m_Debugger, nullptr);
    }

    void GraphicsInstance::createSurface() {
        auto surfaceStatus = glfwCreateWindowSurface((VkInstance) m_Instance, (GLFWwindow*) m_Window->getHandle(), nullptr, (VkSurfaceKHR*) &m_Surface);
        rect_assert(surfaceStatus == VK_SUCCESS, "Failed to create Vulkan window surface")
    }

    void GraphicsInstance::destroySurface() {
        vkDestroySurfaceKHR((VkInstance) m_Instance, (VkSurfaceKHR) m_Surface, nullptr);
    }

    bool QueueFamilyIndices::completed() const {
        return graphicsFamily != NONE_FAMILY && presentationFamily != NONE_FAMILY;
    }

    void GraphicsInstance::createPhysicalDevice() {
        m_PhysicalDevice = VK_NULL_HANDLE;
        // eval devices count
        u32 deviceCount = 0;
        vkEnumeratePhysicalDevices((VkInstance) m_Instance, &deviceCount, nullptr);
        rect_assert(deviceCount != 0, "Failed to setup Vulkan physical device")
        // eval devices handles
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices((VkInstance) m_Instance, &deviceCount, devices.data());
        // find suitable device
        for (const auto &device: devices) {
            if (isDeviceSuitable(device)) {
                m_PhysicalDevice = device;
                break;
            }
        }
        rect_assert(m_PhysicalDevice != VK_NULL_HANDLE, "Failed to find a suitable GPU")
    }

    bool GraphicsInstance::isDeviceSuitable(void* physicalDevice) {
        bool extensionSupport = isDeviceExtensionSupported(physicalDevice);
        bool swapChainSupport = false;
        if (extensionSupport) {
            SwapChainSupportDetails swapChainSupportDetails = querySwapChainSupport(physicalDevice, m_Surface);
            swapChainSupport = !swapChainSupportDetails.formats.empty() && !swapChainSupportDetails.presentModes.empty();
        }
        return findQueueFamilies(physicalDevice).completed() && extensionSupport && swapChainSupport;
    }

    QueueFamilyIndices GraphicsInstance::findQueueFamilies(void* physicalDevice) {
        QueueFamilyIndices indices;
        auto vkPhysicalDevice = (VkPhysicalDevice) physicalDevice;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            // check for graphics support
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                indices.graphicsFamily = i;
            // check for presentation support
            VkBool32 presentationSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(vkPhysicalDevice, i, (VkSurfaceKHR) m_Surface, &presentationSupport);
            if (presentationSupport)
                indices.presentationFamily = i;

            if (indices.completed())
                break;

            i++;
        }

        return indices;
    }

    void GraphicsInstance::createLogicalDevice() {
        // setup queue commands
        QueueFamilyIndices indices = findQueueFamilies(m_PhysicalDevice);
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<int> uniqueQueueFamilies = {
                indices.graphicsFamily,
                indices.presentationFamily
        };
        float queuePriority = 1.0f;
        for (int queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }
        // setup device features
        VkPhysicalDeviceFeatures deviceFeatures{};
        // setup logical device
        VkDeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceCreateInfo.queueCreateInfoCount = queueCreateInfos.size();
        deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
#ifdef VALIDATION_LAYERS
        deviceCreateInfo.enabledLayerCount = static_cast<u32>(m_ValidationLayers.size());
        deviceCreateInfo.ppEnabledLayerNames = m_ValidationLayers.data();
#else
        deviceCreateInfo.enabledLayerCount = 0;
#endif
        deviceCreateInfo.enabledExtensionCount = static_cast<u32>(m_DeviceExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();
        // create and assert logical device
        auto logicalDeviceStatus = vkCreateDevice((VkPhysicalDevice) m_PhysicalDevice, &deviceCreateInfo, nullptr, (VkDevice*) &m_LogicalDevice);
        rect_assert(logicalDeviceStatus == VK_SUCCESS, "Failed to create Vulkan logical device")
        // setup queue interfaces
        vkGetDeviceQueue((VkDevice) m_LogicalDevice, indices.graphicsFamily, 0, (VkQueue*) &m_GraphicsQueue);
        vkGetDeviceQueue((VkDevice) m_LogicalDevice, indices.presentationFamily, 0, (VkQueue*) &m_PresentationQueue);
    }

    void GraphicsInstance::destroyLogicalDevice() {
        vkDestroyDevice((VkDevice) m_LogicalDevice, nullptr);
    }

    bool GraphicsInstance::isDeviceExtensionSupported(void *physicalDevice) {
        u32 extensionCount;
        vkEnumerateDeviceExtensionProperties((VkPhysicalDevice) physicalDevice, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties((VkPhysicalDevice) physicalDevice, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(m_DeviceExtensions.begin(), m_DeviceExtensions.end());

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    void GraphicsInstance::createSwapChain() {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_PhysicalDevice, m_Surface);
        // select swap chain format
        VkSurfaceFormatKHR surfaceFormat = selectSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = selectSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = selectSwapExtent((GLFWwindow*) m_Window->getHandle(), swapChainSupport.capabilities);
        // eval image count
        u32 imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }
        // setup swap chain info
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = (VkSurfaceKHR) m_Surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        // setup queue families
        QueueFamilyIndices indices = findQueueFamilies(m_PhysicalDevice);
        int queueFamilyIndices[] = { indices.graphicsFamily, indices.presentationFamily };

        if (indices.graphicsFamily != indices.presentationFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = (u32*) &queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }
        // setup images transform
        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        // setup window blending
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        // setup presentation mode
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        // setup swap chain lifecycle
        createInfo.oldSwapchain = VK_NULL_HANDLE;
        // create swap chain
        auto swapChainStatus = vkCreateSwapchainKHR((VkDevice) m_LogicalDevice, &createInfo, nullptr, (VkSwapchainKHR*) &m_SwapChain);
        rect_assert(swapChainStatus == VK_SUCCESS, "Failed to create Vulkan swap chain");
        querySwapChainImages(imageCount);
        m_SwapChainImageFormat = surfaceFormat.format;
        m_SwapChainExtent.width = extent.width;
        m_SwapChainExtent.height = extent.height;
    }

    void GraphicsInstance::destroySwapChain() {
        vkDestroySwapchainKHR((VkDevice) m_LogicalDevice, (VkSwapchainKHR) m_SwapChain, nullptr);
    }

    void GraphicsInstance::querySwapChainImages(u32 imageCount) {
        std::vector<VkImage> swapChainImages;
        vkGetSwapchainImagesKHR((VkDevice) m_LogicalDevice, (VkSwapchainKHR) m_SwapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR((VkDevice) m_LogicalDevice, (VkSwapchainKHR) m_SwapChain, &imageCount, swapChainImages.data());
        for (const auto& image : swapChainImages) {
            m_SwapChainImages.emplace_back(image);
        }
    }

    void GraphicsInstance::createImageViews() {
        m_ImageViews.resize(m_SwapChainImages.size());
        for (u32 i = 0 ; i < m_SwapChainImages.size() ; i++) {
            // setup image view info
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = (VkImage) m_SwapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = (VkFormat) m_SwapChainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;
            // create image view
            auto imageViewStatus = vkCreateImageView((VkDevice) m_LogicalDevice, &createInfo, nullptr, (VkImageView*) &m_ImageViews[i]);
            rect_assert(imageViewStatus == VK_SUCCESS, "Failed to create Vulkan image view")
        }
    }

    void GraphicsInstance::destroyImageViews() {
        for (const auto& imageView : m_ImageViews) {
            vkDestroyImageView((VkDevice) m_LogicalDevice, (VkImageView) imageView, nullptr);
        }
    }

    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        rect_assert(file.is_open(), "Failed to open file %s\n", filename.c_str())

        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();
        return buffer;
    }

    void GraphicsInstance::createPipeline() {
        // setup shaders
        m_Shaders.emplace_back(m_LogicalDevice, "spirv/shader_vert.spv", "spirv/shader_frag.spv");
        // setup dynamic state for pipeline
        std::vector<VkDynamicState> dynamicStates = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<u32>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();
        // setup vertex input state
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional
        // setup input assembly state
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;
        // setup viewport
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) m_SwapChainExtent.width;
        viewport.height = (float) m_SwapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        // setup scissor
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        VkExtent2D extent2D{};
        extent2D.width = m_SwapChainExtent.width;
        extent2D.height = m_SwapChainExtent.height;
        scissor.extent = extent2D;
        // setup viewport state
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;
        // setup rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
        // setup multisampling
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional
        // setup depth/stencil state
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        // setup color blend attachments
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        // setup color blend state
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional
        // setup pipeline layout info
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0; // Optional
        pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
        // create pipeline layout
        auto pipelineLayoutStatus = vkCreatePipelineLayout((VkDevice) m_LogicalDevice, &pipelineLayoutInfo, nullptr, (VkPipelineLayout*) &m_PipelineLayout);
        rect_assert(pipelineLayoutStatus == VK_SUCCESS, "Failed to create Vulkan pipeline layout")
        // setup pipeline info
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        // setup shader stages
        const Shader& exampleShader = m_Shaders.at(0);
        // vertex shader stage
        VkPipelineShaderStageCreateInfo vkVertStage {};
        ShaderStage vertStage = exampleShader.getVertStage();
        vkVertStage.sType = (VkStructureType) vertStage.sType;
        vkVertStage.pNext = vertStage.next;
        vkVertStage.module = (VkShaderModule) vertStage.module;
        vkVertStage.flags = vertStage.flags;
        vkVertStage.pName = vertStage.name;
        vkVertStage.pSpecializationInfo = (VkSpecializationInfo*) vertStage.specInfo;
        vkVertStage.stage = (VkShaderStageFlagBits) vertStage.stage;
        // fragment shader stage
        VkPipelineShaderStageCreateInfo vkFragStage {};
        ShaderStage fragStage = exampleShader.getFragStage();
        vkFragStage.sType = (VkStructureType) fragStage.sType;
        vkFragStage.pNext = fragStage.next;
        vkFragStage.module = (VkShaderModule) fragStage.module;
        vkFragStage.flags = fragStage.flags;
        vkFragStage.pName = fragStage.name;
        vkFragStage.pSpecializationInfo = (VkSpecializationInfo*) fragStage.specInfo;
        vkFragStage.stage = (VkShaderStageFlagBits) fragStage.stage;
        // setup all components into pipeline info
        VkPipelineShaderStageCreateInfo shaderStages[2] = {
                vkVertStage,
                vkFragStage
        };
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil; // Optional
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = (VkPipelineLayout) m_PipelineLayout;
        pipelineInfo.renderPass = (VkRenderPass) m_RenderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional
        // create pipeline
        auto pipelineStatus = vkCreateGraphicsPipelines((VkDevice) m_LogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, (VkPipeline*) &m_Pipeline);
        rect_assert(pipelineStatus == VK_SUCCESS, "Failed to create Vulkan pipeline")
    }

    void GraphicsInstance::destroyPipeline() {
        vkDestroyPipeline((VkDevice) m_LogicalDevice, (VkPipeline) m_Pipeline, nullptr);
        vkDestroyPipelineLayout((VkDevice) m_LogicalDevice, (VkPipelineLayout) m_PipelineLayout, nullptr);
        m_Shaders.clear();
    }

    void GraphicsInstance::createRenderPass() {
        // setup color attachment
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = (VkFormat) m_SwapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        // setup attachment reference
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        // setup subpass
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        // setup render pass info
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        // create render pass
        auto renderPassStatus = vkCreateRenderPass((VkDevice) m_LogicalDevice, &renderPassInfo, nullptr, (VkRenderPass*) &m_RenderPass);
        rect_assert(renderPassStatus == VK_SUCCESS, "Failed to create Vulkan render pass");
    }

    void GraphicsInstance::destroyRenderPass() {
        vkDestroyRenderPass((VkDevice) m_LogicalDevice, (VkRenderPass) m_RenderPass, nullptr);
    }

    Shader::Shader(void* logicalDevice, const std::string &vertFilepath, const std::string &fragFilepath) {
        m_LogicalDevice = logicalDevice;
        // read shader files into sources
        auto vertSrc = readFile("spirv/shader_vert.spv");
        auto fragSrc = readFile("spirv/shader_frag.spv");
        // create shader modules from sources
        m_VertStage = ShaderStage(m_LogicalDevice, vertSrc);
        m_FragStage = ShaderStage(m_LogicalDevice, fragSrc);
        // setup shader into pipeline
        // vertex shader
        m_VertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        m_VertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        m_VertStage.name = "main";
        // fragment shader
        m_FragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        m_FragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        m_FragStage.name = "main";
    }

    Shader::~Shader() {
        cleanup();
    }

    void Shader::cleanup() {
        m_VertStage.cleanup(m_LogicalDevice);
        m_FragStage.cleanup(m_LogicalDevice);
    }

    const ShaderStage& Shader::getVertStage() const {
        return m_VertStage;
    }

    const ShaderStage& Shader::getFragStage() const {
        return m_FragStage;
    }

    void ShaderStage::cleanup(void* logicalDevice) {
        if (module)
            vkDestroyShaderModule((VkDevice) logicalDevice, (VkShaderModule) module, nullptr);
        module = nullptr;
    }

    ShaderStage::ShaderStage(void* logicalDevice, const std::vector<char>& code) {
        // setup shader module info
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const u32*>(code.data());
        // create shader module
        auto shaderModuleStatus = vkCreateShaderModule((VkDevice) logicalDevice, &createInfo, nullptr, (VkShaderModule*) &module);
        rect_assert(shaderModuleStatus == VK_SUCCESS, "Failed to create Vulkan shader module \nCode: %s\n", code.data())
    }
}