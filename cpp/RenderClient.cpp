#include <RenderClient.h>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32

#include <set>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace rdk {

    RenderClient::RenderClient(const AppInfo &appInfo, Window* window) : m_AppInfo(appInfo), m_Window(window) {
        m_Shaders = std::make_shared<std::vector<Shader>>();
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
        vkAppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
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
        std::vector<const char*> windowExtensions = window->getExtensions();
        // setup creation info
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &vkAppInfo;
        createInfo.ppEnabledExtensionNames = windowExtensions.data();
        createInfo.enabledExtensionCount = static_cast<u32>(windowExtensions.size());
#ifdef VALIDATION_LAYERS
        std::vector<const char*> validationLayers = m_Device.getValidationLayers();
        createInfo.enabledLayerCount = static_cast<u32>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        Debugger::setMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugCreateInfo;
#else
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
#endif
        // try to create instance, otherwise throws Runtime error
        VkResult instanceStatus = vkCreateInstance(&createInfo, nullptr, &m_Handle);
        rect_assert(instanceStatus == VK_SUCCESS, "Failed to create GraphicsInstance!")
#ifdef VALIDATION_LAYERS
        m_Debugger.create(m_Handle);
#endif
        createSurface();
        m_Device.create(m_Handle, m_Surface);
        m_CommandPool = CommandPool(m_Window->getHandle(), m_Surface, m_Device, &m_DescriptorPool);
    }

    RenderClient::~RenderClient() {
        m_Device.waitIdle();
        m_DescriptorPool.destroy();
        for (auto& buffer : m_UniformBuffers) {
            buffer.destroy();
        }
        m_UniformBuffers.clear();
        m_IndexBuffer.destroy();
        m_VertexBuffer.destroy();
        m_Shaders->clear();
        m_CommandPool.destroy();
#ifdef VALIDATION_LAYERS
        m_Debugger.destroy();
#endif
        destroySurface();
        m_Device.destroy();
        // this also destroys physical device associated with instance
        vkDestroyInstance(m_Handle, nullptr);
    }

    void RenderClient::printExtensions() {
        printf("Available extensions: \n");
        for (const auto &extension: m_ExtensionProps) {
            printf("\t %s \n", extension.name.c_str());
        }
    }

    void RenderClient::createSurface() {
        auto surfaceStatus = glfwCreateWindowSurface(m_Handle, (GLFWwindow*) m_Window->getHandle(), nullptr, &m_Surface);
        rect_assert(surfaceStatus == VK_SUCCESS, "Failed to create Vulkan window surface")
    }

    void RenderClient::destroySurface() {
        vkDestroySurfaceKHR(m_Handle, m_Surface, nullptr);
    }

    void RenderClient::beginFrame() {
        static auto beginTime = std::chrono::high_resolution_clock::now();
        m_BeginTime = beginTime;
        m_CommandPool.beginFrame();
    }

    void RenderClient::endFrame() {
        m_CommandPool.endFrame();
        auto endTime = std::chrono::high_resolution_clock::now();
        m_DeltaTime = std::chrono::duration<float, std::chrono::seconds::period>(endTime - m_BeginTime).count();
    }

    void RenderClient::drawVertices(u32 vertexCount, u32 instanceCount) {
        m_CommandPool.drawVertices(vertexCount, instanceCount);
    }

    void RenderClient::drawIndices(u32 indexCount, u32 instanceCount) {
        m_CommandPool.drawIndices(indexCount, instanceCount);
    }

    void RenderClient::onFrameBufferResized(int width, int height) {
        m_CommandPool.setFrameBufferResized(true);
    }

    void RenderClient::addShader(const std::string& vertFilepath, const std::string& fragFilepath) {
        m_Shaders->emplace_back(m_Device.getLogicalHandle(), vertFilepath, fragFilepath);
    }

    void RenderClient::createVertexBuffer(const VertexData& vertexData) {
        VkDeviceSize size = vertexData.size;
        Buffer stageBuffer;

        stageBuffer.create(
                size,
                m_Device.getLogicalHandle(),
                m_Device.getPhysicalHandle(),
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        m_VertexBuffer.create(
                size,
                m_Device.getLogicalHandle(),
                m_Device.getPhysicalHandle(),
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        // copy CPU -> stage buffer
        void* block = stageBuffer.mapMemory(size);
        memcpy(block, vertexData.data, size);
        stageBuffer.unmapMemory();
        // copy stage buffer - GPU device local buffer
        m_CommandPool.copyBuffer(stageBuffer.getHandle(), m_VertexBuffer.getHandle(), size);
        // free stage buffer
        stageBuffer.destroy();
    }

    void RenderClient::createIndexBuffer(const IndexData& indexData) {
        VkDeviceSize size = indexData.size;
        Buffer stageBuffer;

        stageBuffer.create(
                size,
                m_Device.getLogicalHandle(),
                m_Device.getPhysicalHandle(),
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        m_IndexBuffer.create(
                size,
                m_Device.getLogicalHandle(),
                m_Device.getPhysicalHandle(),
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        // copy CPU -> stage buffer
        void* block = stageBuffer.mapMemory(size);
        memcpy(block, indexData.data, size);
        stageBuffer.unmapMemory();
        // copy stage buffer - GPU device local buffer
        m_CommandPool.copyBuffer(stageBuffer.getHandle(), m_IndexBuffer.getHandle(), size);
        // free stage buffer
        stageBuffer.destroy();
    }

    void RenderClient::initialize() {
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

        VkVertexInputBindingDescription vertexBindDesc;
        vertexBindDesc.binding = 0;
        vertexBindDesc.stride = sizeof(Vertex);
        vertexBindDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        std::vector<VkVertexInputAttributeDescription> attrs {
                { 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, position) },
                { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) }
        };

        // setup pipeline
        Pipeline pipeline;
        pipeline.setLogicalDevice(m_Device.getLogicalHandle());
        pipeline.setVertexBuffer(&m_VertexBuffer);
        pipeline.setIndexBuffer(&m_IndexBuffer);
        pipeline.setAssemblyInput();
        pipeline.setVertexInput({ vertexBindDesc, attrs });
        pipeline.setDynamicStates();
        pipeline.setViewport(swapChain.getExtent());
        pipeline.setScissor(swapChain.getExtent());
        pipeline.setShader(m_Shaders->at(0));
        pipeline.setSwapChain(swapChain);
        pipeline.setRasterizer();
        pipeline.setMultisampling();

        pipeline.setColorBlendAttachment();
        pipeline.setColorBlending();

        VkDescriptorSetLayout descriptorSetLayout = pipeline.createDescriptorLayout();
        // setup descriptor pool
        VkDescriptorPoolSize poolSize {};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = m_CommandPool.getMaxFramesInFlight();
        m_DescriptorPool.create(m_Device.getLogicalHandle(), poolSize, poolSize.descriptorCount);
        m_DescriptorPool.createSets(poolSize.descriptorCount, descriptorSetLayout);

        pipeline.setLayout();
        pipeline.createLayout();

        pipeline.create();

        // setup command pool
        m_CommandPool.setPipeline(pipeline);
        m_CommandPool.create();
    }

    void RenderClient::createRect() {
        Rect rect;
        VertexData vertexData = { rect.vertex_size(), rect.data() };
        IndexData indexData = { rect.index_size(), rect.indices };
        createVertexBuffer(vertexData);
        createIndexBuffer(indexData);
    }

    void RenderClient::createUniformBuffers(VkDeviceSize size) {
        u32 maxFramesInFlight = m_CommandPool.getMaxFramesInFlight();
        VkDevice device = m_Device.getLogicalHandle();
        VkPhysicalDevice physicalDevice = m_Device.getPhysicalHandle();

        m_UniformBuffers.resize(maxFramesInFlight);
        m_UniformBufferBlocks.resize(maxFramesInFlight);

        for (int i = 0 ; i < maxFramesInFlight ; i++) {
            Buffer& uniformBuffer = m_UniformBuffers[i];
            uniformBuffer.create(
                    size,
                    device,
                    physicalDevice,
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );
            m_UniformBufferBlocks[i] = uniformBuffer.mapMemory(size);

            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffer.getHandle();
            bufferInfo.offset = 0;
            bufferInfo.range = VK_WHOLE_SIZE;

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = m_DescriptorPool[i];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;
            descriptorWrite.pImageInfo = nullptr; // Optional
            descriptorWrite.pTexelBufferView = nullptr; // Optional

            vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
        }
    }

    MVP RenderClient::createMVP(float aspect) {
        MVP mvp;
        createUniformBuffers(sizeof(MVP));
        mvp.model = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        mvp.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        mvp.proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 10.0f);
        return mvp;
    }

    void RenderClient::updateMVP(MVP &mvp) {
        u32 currentFrame = m_CommandPool.getCurrentFrame();
        VkExtent2D extent = m_CommandPool.getExtent();
        float aspect = (float) extent.width / (float) extent.height;

        mvp.model = glm::rotate(glm::mat4(1.0f), m_DeltaTime * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        mvp.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        mvp.proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 10.0f);

        memcpy(m_UniformBufferBlocks[currentFrame], &mvp, sizeof(MVP));
    }

}