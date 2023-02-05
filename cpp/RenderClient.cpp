#include <RenderClient.h>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE   // change depth buffer range from [-1.0, 1.0] (OpenGL) -> [0.0, 1.0] (Vulkan)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <set>

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
        auto validationLayers = m_Device.getValidationLayers();
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

        m_ImageSamplers.clear();
        m_ImageViews.clear();
        m_Images.clear();

        m_DescriptorPool.destroy();

        for (auto& buffer : m_UniformBuffers) {
            buffer.destroy();
        }
        m_UniformBuffers.clear();

        m_IndexBuffer.destroy();
        m_VertexBuffer.destroy();

        m_Shaders->clear();

        m_SwapChain.destroy();

        m_Pipeline.destroy();

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
        m_SwapChain.setLogicalDevice(m_Device.getLogicalHandle());
        m_SwapChain.create(m_Window->getHandle(), m_Device.getPhysicalHandle(), m_Surface, m_Device.findQueueFamily(m_Surface));

        // render pass
        RenderPass renderPass;
        renderPass.setLogicalDevice(m_Device.getLogicalHandle());
        renderPass.setFormat(m_SwapChain.getImageFormat());
        renderPass.create();

        m_SwapChain.setRenderPass(renderPass);
        // image views
        m_SwapChain.createImageViews();
        // frame buffers
        m_SwapChain.createFrameBuffers();

        VkVertexInputBindingDescription vertexBindDesc;
        vertexBindDesc.binding = 0;
        vertexBindDesc.stride = sizeof(Vertex);
        vertexBindDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        std::vector<VkVertexInputAttributeDescription> attrs {
                { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) },
                { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) },
                { 2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv) }
        };

        // setup pipeline
        m_Pipeline.setLogicalDevice(m_Device.getLogicalHandle());
        m_Pipeline.setVertexBuffer(&m_VertexBuffer);
        m_Pipeline.setIndexBuffer(&m_IndexBuffer);
        m_Pipeline.setAssemblyInput();
        m_Pipeline.setVertexInput({ vertexBindDesc, attrs });
        m_Pipeline.setDynamicStates();
        m_Pipeline.setViewport(m_SwapChain.getExtent());
        m_Pipeline.setScissor(m_SwapChain.getExtent());
        m_Pipeline.setShader(m_Shaders->at(0));
        m_Pipeline.setSwapChain(&m_SwapChain);
        m_Pipeline.setRasterizer();
        m_Pipeline.setMultisampling();

        m_Pipeline.setColorBlendAttachment();
        m_Pipeline.setColorBlending();

        // setup descriptor layout bindings
        VkDescriptorSetLayoutBinding layoutBindings[] = {
                m_Pipeline.createBinding(0, VERTEX_UNIFORM_BUFFER),
                m_Pipeline.createBinding(1, FRAG_SAMPLER)
        };
        int bindings = sizeof(layoutBindings) / sizeof(layoutBindings[0]);
        VkDescriptorSetLayout descriptorSetLayout = m_Pipeline.createDescriptorLayout(layoutBindings, bindings);

        // setup descriptor pool
        u32 maxFramesInFlight = m_CommandPool.getMaxFramesInFlight();

        VkDescriptorPoolSize uboPoolSize{};
        uboPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboPoolSize.descriptorCount = maxFramesInFlight;

        VkDescriptorPoolSize samplerPoolSize{};
        samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerPoolSize.descriptorCount = maxFramesInFlight;

        VkDescriptorPoolSize poolSizes[] = {
                uboPoolSize,
                samplerPoolSize
        };
        int poolSizeCount = sizeof(poolSizes) / sizeof(poolSizes[0]);

        m_DescriptorPool.create(m_Device.getLogicalHandle(), poolSizes, poolSizeCount, maxFramesInFlight);
        m_DescriptorPool.createSets(maxFramesInFlight, descriptorSetLayout);

        m_Pipeline.setLayout();
        m_Pipeline.createLayout();

        m_Pipeline.create();

        // setup command pool
        m_CommandPool.setPipeline(&m_Pipeline);
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

        VkImageView imageView = m_ImageViews[0].getHandle();
        VkSampler imageSampler = m_ImageSamplers[0].getHandle();

        for (int i = 0 ; i < maxFramesInFlight ; i++) {

            // -------------------- uniform buffer setup

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

            VkDescriptorBufferInfo uboInfo{};
            uboInfo.buffer = uniformBuffer.getHandle();
            uboInfo.offset = 0;
            uboInfo.range = VK_WHOLE_SIZE;

            VkWriteDescriptorSet uboWriteDescriptor{};
            uboWriteDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            uboWriteDescriptor.dstSet = m_DescriptorPool[i];
            uboWriteDescriptor.dstBinding = 0;
            uboWriteDescriptor.dstArrayElement = 0;
            uboWriteDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uboWriteDescriptor.descriptorCount = 1;
            uboWriteDescriptor.pBufferInfo = &uboInfo;
            uboWriteDescriptor.pImageInfo = nullptr; // Optional
            uboWriteDescriptor.pTexelBufferView = nullptr; // Optional

            // ---------------------- combined image sampler setup

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = imageView;
            imageInfo.sampler = imageSampler;

            VkWriteDescriptorSet imageWriteDescriptor{};
            imageWriteDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            imageWriteDescriptor.dstSet = m_DescriptorPool[i];
            imageWriteDescriptor.dstBinding = 1;
            imageWriteDescriptor.dstArrayElement = 0;
            imageWriteDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            imageWriteDescriptor.descriptorCount = 1;
            imageWriteDescriptor.pImageInfo = &imageInfo;

            VkWriteDescriptorSet writeDescriptors[] = {
                    uboWriteDescriptor,
                    imageWriteDescriptor
            };
            int writeDescriptorCount = sizeof(writeDescriptors) / sizeof(writeDescriptors[0]);

            vkUpdateDescriptorSets(device, writeDescriptorCount, writeDescriptors, 0, nullptr);
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
        VkExtent2D extent = m_SwapChain.getExtent();
        float aspect = (float) extent.width / (float) extent.height;

        mvp.model = glm::rotate(glm::mat4(1.0f), m_DeltaTime * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        mvp.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        mvp.proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 10.0f);

        memcpy(m_UniformBufferBlocks[currentFrame], &mvp, sizeof(MVP));
    }

    void RenderClient::createTexture2D(const char *filepath) {
        VkDevice device = m_Device.getLogicalHandle();
        VkPhysicalDevice physicalDevice = m_Device.getPhysicalHandle();

        ImageData imageData = ImageLoader::load(filepath,device,physicalDevice);
        VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
        VkBuffer stageBuffer = imageData.stageBuffer.getHandle();
        u32 width = imageData.width;
        u32 height = imageData.height;

        m_Images.emplace_back(
                device,
                physicalDevice,
                imageData.width, imageData.height,
                format,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkImage texture2D = m_Images.rbegin()->getHandle();

        m_CommandPool.transitionImageLayout(
                texture2D, format,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        );
        m_CommandPool.copyBufferImage(stageBuffer, texture2D, width, height);
        m_CommandPool.transitionImageLayout(
            texture2D, format,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );

        imageData.stageBuffer.destroy();

        m_ImageViews.emplace_back(device, texture2D, format);

        m_ImageSamplers.emplace_back(m_Device);
    }

}