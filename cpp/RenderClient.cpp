#include <RenderClient.h>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32

#include <set>

namespace rdk {

    RenderClient::RenderClient(const AppInfo &appInfo, Window* window) : m_AppInfo(appInfo), m_Window(window) {
        // list device extensions to be supported
        m_Device.setExtensions({
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        });
        // Layers validation should be supported in DEBUG mode, otherwise throws Runtime error.
#ifdef VALIDATION_LAYERS
        rect_assert(m_Device.isLayerValidationSupported(), "Layer validation not supported!")
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
        createInfo.enabledLayerCount = static_cast<u32>(m_Device.getValidationLayers().size());
        createInfo.ppEnabledLayerNames = m_Device.getValidationLayers().data();
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        Debugger::setMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugCreateInfo;
#else
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
#endif
        // try to create instance, otherwise throws Runtime error
        VkResult instanceStatus = vkCreateInstance(&createInfo, nullptr, (VkInstance*) &m_Handle);
        rect_assert(instanceStatus == VK_SUCCESS, "Failed to create GraphicsInstance!")
#ifdef VALIDATION_LAYERS
        m_Debugger.create(m_Handle);
#endif
        createSurface();
        m_Device.create(m_Handle, m_Surface);
        m_CommandPool = CommandPool(m_Window->getHandle(), m_Surface, m_Device);
        // setup pipeline
        Pipeline pipeline;
        pipeline.setLogicalDevice(m_Device.getLogicalHandle());

        // setup swap chain
        SwapChain swapChain;
        swapChain.setLogicalDevice(m_Device.getLogicalHandle());
        swapChain.create(m_Window->getHandle(), m_Device.getPhysicalHandle(), m_Surface, m_Device.findQueueFamily(m_Surface));
        // render pass
        RenderPass renderPass;
        renderPass.setLogicalDevice(m_Device.getLogicalHandle());
        renderPass.setFormat(swapChain.getImageFormat());
        renderPass.create();
        swapChain.setRenderPass(renderPass);
        // image views
        swapChain.createImageViews();
        // frame buffers
        swapChain.createFrameBuffers();

        pipeline.setSwapChain(swapChain);
        // shaders
        Shader exampleShader(m_Device.getLogicalHandle(), "spirv/shader_vert.spv", "spirv/shader_frag.spv");
        exampleShader.setVertexFormat({
            VertexBindDescriptor { 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX },
            {
                VertexAttr { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) },
                VertexAttr { 0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) }
            }
        });
        pipeline.addShader(exampleShader);
        // create pipeline
        pipeline.create();
        // setup command pool
        m_CommandPool.setPipeline(pipeline);
        m_CommandPool.create();
    }

    RenderClient::~RenderClient() {
        m_Device.waitIdle();
        m_CommandPool.destroy();
#ifdef VALIDATION_LAYERS
        m_Debugger.destroy();
#endif
        destroySurface();
        m_Device.destroy();
        // this also destroys physical device associated with instance
        vkDestroyInstance((VkInstance) m_Handle, nullptr);
    }

    void RenderClient::printExtensions() {
        printf("Available extensions: \n");
        for (const auto &extension: m_ExtensionProps) {
            printf("\t %s \n", extension.name.c_str());
        }
    }

    void RenderClient::createSurface() {
        auto surfaceStatus = glfwCreateWindowSurface((VkInstance) m_Handle, (GLFWwindow*) m_Window->getHandle(), nullptr, (VkSurfaceKHR*) &m_Surface);
        rect_assert(surfaceStatus == VK_SUCCESS, "Failed to create Vulkan window surface")
    }

    void RenderClient::destroySurface() {
        vkDestroySurfaceKHR((VkInstance) m_Handle, (VkSurfaceKHR) m_Surface, nullptr);
    }

    void RenderClient::drawFrame(u32 vertexCount, u32 instanceCount) {
        m_CommandPool.drawFrame(vertexCount, instanceCount);
    }

    void RenderClient::onFrameBufferResized(int width, int height) {
        m_CommandPool.setFrameBufferResized(true);
    }

}