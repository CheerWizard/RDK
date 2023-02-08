#include <Renderer.h>

#include <FontAwesome.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE   // change depth buffer range from [-1.0, 1.0] (OpenGL) -> [0.0, 1.0] (Vulkan)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <set>
#include <iostream>

namespace rdk {

    Renderer::Renderer(const AppInfo &appInfo, Window* window) : m_AppInfo(appInfo), m_Window(window) {
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
        m_Queue.create(m_Device.getLogicalHandle(), m_Device.findQueueFamily(m_Surface));
        m_CommandPool = CommandPool(
                m_Handle,
                m_Window, m_Surface,
                &m_Device, &m_DescriptorPool,
                &m_Queue, &m_Pipeline
        );
    }

    Renderer::~Renderer() {
#ifdef IMGUI
        destroyUI();
#endif

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

        delete m_SwapChain;

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

    void Renderer::printExtensions() {
        printf("Available extensions: \n");
        for (const auto &extension: m_ExtensionProps) {
            printf("\t %s \n", extension.name.c_str());
        }
    }

    void Renderer::createSurface() {
        auto surfaceStatus = glfwCreateWindowSurface(m_Handle, (GLFWwindow*) m_Window->getHandle(), nullptr, &m_Surface);
        rect_assert(surfaceStatus == VK_SUCCESS, "Failed to create Vulkan window surface")
    }

    void Renderer::destroySurface() {
        vkDestroySurfaceKHR(m_Handle, m_Surface, nullptr);
    }

    void Renderer::update() {
        static auto beginTime = std::chrono::high_resolution_clock::now();
        m_BeginTime = beginTime;

#ifdef IMGUI
        m_CommandPool.beginUI();
        listener->onRenderUI(m_DeltaTime);
#endif

        m_CommandPool.beginFrame();
        listener->onRender(m_DeltaTime);
        m_CommandPool.endFrame();

        auto endTime = std::chrono::high_resolution_clock::now();
        m_DeltaTime = std::chrono::duration<float, std::chrono::seconds::period>(endTime - m_BeginTime).count();
    }

    void Renderer::drawVertices(u32 vertexCount, u32 instanceCount) {
        m_CommandPool.drawVertices(vertexCount, instanceCount);
    }

    void Renderer::drawIndices(u32 indexCount, u32 instanceCount) {
        m_CommandPool.drawIndices(indexCount, instanceCount);
    }

    void Renderer::onFrameBufferResized(int width, int height) {
        m_CommandPool.setFrameBufferResized(true);
    }

    void Renderer::addShader(const std::string& vertFilepath, const std::string& fragFilepath) {
        m_Shaders->emplace_back(m_Device.getLogicalHandle(), vertFilepath, fragFilepath);
    }

    void Renderer::createVertexBuffer(const VertexData& vertexData) {
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

    void Renderer::createIndexBuffer(const IndexData& indexData) {
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

    void Renderer::initialize() {
        m_SwapChain = new SwapChain(m_Window->getHandle(), &m_Device, m_Surface, m_Device.findDepthFormat());
        m_RenderPass = &m_SwapChain->getRenderPass();

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
        m_Pipeline = Pipeline(m_Device.getLogicalHandle(), m_SwapChain);
        m_Pipeline.setVertexBuffer(&m_VertexBuffer);
        m_Pipeline.setIndexBuffer(&m_IndexBuffer);
        m_Pipeline.setAssemblyInput();
        m_Pipeline.setVertexInput({ vertexBindDesc, attrs });
        m_Pipeline.setDynamicStates();
        m_Pipeline.setViewport(m_SwapChain->getExtent());
        m_Pipeline.setScissor(m_SwapChain->getExtent());
        m_Pipeline.setShader(m_Shaders->at(0));
        m_Pipeline.setRasterizer();
        m_Pipeline.setMultisampling();

        m_Pipeline.setColorBlendAttachment();
        m_Pipeline.setColorBlending();

        m_Pipeline.setDepthStencil();

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

        m_CommandPool.create();

        m_CommandPool.transitionImageLayout(
                m_SwapChain->getDepthImage(),
                m_SwapChain->getDepthFormat(),
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        );

#ifdef IMGUI
        createUI();
#endif
    }

    void Renderer::createRect() {
        Rect rect;
        VertexData vertexData = { rect.vertex_size(), rect.data() };
        IndexData indexData = { rect.index_size(), rect.indices };
        createVertexBuffer(vertexData);
        createIndexBuffer(indexData);
    }

    void Renderer::createUniformBuffers(VkDeviceSize size) {
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

    MVP Renderer::createMVP(float aspect) {
        MVP mvp;
        createUniformBuffers(sizeof(MVP));
        mvp.model = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        mvp.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        mvp.proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 10.0f);
        return mvp;
    }

    void Renderer::updateMVP(MVP &mvp) {
        u32 currentFrame = m_CommandPool.getCurrentFrame();
        VkExtent2D extent = m_SwapChain->getExtent();
        float aspect = (float) extent.width / (float) extent.height;

        mvp.model = glm::rotate(glm::mat4(1.0f), m_DeltaTime * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        mvp.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        mvp.proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 10.0f);

        memcpy(m_UniformBufferBlocks[currentFrame], &mvp, sizeof(MVP));
    }

    void Renderer::createTexture2D(const char *filepath) {
        VkDevice device = m_Device.getLogicalHandle();
        VkPhysicalDevice physicalDevice = m_Device.getPhysicalHandle();

        ImageData imageData = ImageLoader::load(filepath,device,physicalDevice);
        VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
        VkBuffer stageBuffer = imageData.stageBuffer.getHandle();
        u32 width = imageData.width;
        u32 height = imageData.height;
        u32 mipLevels = imageData.mipLevels;

        ImageInfo imageInfo;
        imageInfo.width = imageData.width;
        imageInfo.height = imageData.height;
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageInfo.mipLevels = mipLevels;
        m_Images.emplace_back(device, physicalDevice, imageInfo);

        VkImage texture2D = m_Images.rbegin()->getHandle();

        m_CommandPool.transitionImageLayout(
                texture2D, format,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                mipLevels
        );

        m_CommandPool.copyBufferImage(stageBuffer, texture2D, width, height);

        if (m_Device.isLinearFilterSupported(format)) {
            m_CommandPool.generateMipmaps(texture2D, width, height, mipLevels);
        } else {
            std::cerr << "Renderer::createTexture2D: Device is not supporting Linear Filtering feature for MipMapping!" << std::endl;
        }

        imageData.stageBuffer.destroy();

        ImageViewInfo imageViewInfo;
        imageViewInfo.format = format;
        imageViewInfo.mipLevels = mipLevels;
        m_ImageViews.emplace_back(device, texture2D, imageViewInfo);

        ImageSamplerInfo samplerInfo;
        samplerInfo.minLod = static_cast<float>(0);
        samplerInfo.maxLod = static_cast<float>(mipLevels);
        m_ImageSamplers.emplace_back(m_Device, samplerInfo);
    }

    static std::vector<ImFont*> uiFonts;

    static void setTheme() {
        IO.Fonts->Clear();
        uiFonts.emplace_back(IO.Fonts->AddFontFromFileTTF("assets/fonts/opensans/OpenSans-Regular.ttf", 16));
        IO.FontDefault = *uiFonts.rbegin();
        // load icon font
        static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;
        icons_config.OversampleH = 2.5;
        icons_config.OversampleV = 2.5;

        IO.Fonts->AddFontFromFileTTF(
                "assets/fonts/font_awesome_4/fontawesome-webfont.ttf",
                18,
                &icons_config, icons_ranges);
        IO.Fonts->Build();

        ImGuiStyle* style = &ImGui::GetStyle();
        ImGui::StyleColorsDark(style);

        style->WindowPadding            = ImVec2(15, 15);
        style->WindowRounding           = 5.0f;
        style->FramePadding             = ImVec2(5, 5);
        style->FrameRounding            = 4.0f;
        style->ItemSpacing              = ImVec2(12, 8);
        style->ItemInnerSpacing         = ImVec2(8, 6);
        style->IndentSpacing            = 25.0f;
        style->ScrollbarSize            = 15.0f;
        style->ScrollbarRounding        = 9.0f;
        style->GrabMinSize              = 5.0f;
        style->GrabRounding             = 3.0f;

        style->Colors[ImGuiCol_Text]                  = ImVec4(0.40f, 0.39f, 0.38f, 1.00f);
        style->Colors[ImGuiCol_TextDisabled]          = ImVec4(0.40f, 0.39f, 0.38f, 0.77f);
        style->Colors[ImGuiCol_WindowBg]              = ImVec4(0.92f, 0.91f, 0.88f, 0.70f);
        style->Colors[ImGuiCol_ChildBg]               = ImVec4(1.00f, 0.98f, 0.95f, 0.58f);
        style->Colors[ImGuiCol_PopupBg]               = ImVec4(0.92f, 0.91f, 0.88f, 0.92f);
        style->Colors[ImGuiCol_Border]                = ImVec4(0.84f, 0.83f, 0.80f, 0.65f);
        style->Colors[ImGuiCol_BorderShadow]          = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
        style->Colors[ImGuiCol_FrameBg]               = ImVec4(1.00f, 0.98f, 0.95f, 1.00f);
        style->Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.25f, 1.00f, 1.00f, 0.78f);
        style->Colors[ImGuiCol_FrameBgActive]         = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        style->Colors[ImGuiCol_TitleBg]               = ImVec4(1.00f, 0.98f, 0.95f, 1.00f);
        style->Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
        style->Colors[ImGuiCol_TitleBgActive]         = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        style->Colors[ImGuiCol_MenuBarBg]             = ImVec4(1.00f, 0.98f, 0.95f, 0.47f);
        style->Colors[ImGuiCol_ScrollbarBg]           = ImVec4(1.00f, 0.98f, 0.95f, 1.00f);
        style->Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.00f, 0.00f, 0.00f, 0.21f);
        style->Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.25, 1.00f, 1.00f, 0.78f);
        style->Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.25f, 1.00f, 1.00f, 1.00f);
        style->Colors[ImGuiCol_CheckMark]             = ImVec4(0.25f, 1.00f, 1.00f, 0.80f);
        style->Colors[ImGuiCol_SliderGrab]            = ImVec4(0.00f, 0.00f, 0.00f, 0.14f);
        style->Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.25f, 1.00f, 1.00f, 1.00f);
        style->Colors[ImGuiCol_Button]                = ImVec4(0.00f, 0.00f, 0.00f, 0.14f);
        style->Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.25f, 1.00f, 1.00f, 0.86f);
        style->Colors[ImGuiCol_ButtonActive]          = ImVec4(0.25f, 1.00f, 1.00f, 1.00f);
        style->Colors[ImGuiCol_Header]                = ImVec4(0.25f, 1.00f, 1.00f, 0.76f);
        style->Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.25f, 1.00f, 1.00f, 0.86f);
        style->Colors[ImGuiCol_HeaderActive]          = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        style->Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.00f, 0.00f, 0.00f, 0.04f);
        style->Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.25f, 1.00f, 1.00f, 0.78f);
        style->Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.25f, 1.00f, 1.00f, 1.00f);
        style->Colors[ImGuiCol_PlotLines]             = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
        style->Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.25f, 1.00f, 1.00f, 1.00f);
        style->Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
        style->Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.25f, 1.00f, 1.00f, 1.00f);
        style->Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.25f, 1.00f, 1.00f, 0.43f);
        style->Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(1.00f, 0.98f, 0.95f, 0.73f);
    }

    static void handleImGuiVkResult(VkResult result) {
        rect_assert(result == VK_SUCCESS, "handleImGuiVkResult assertion failed!")
    }

    void Renderer::createUI() {
#ifdef IMGUI
        VkDevice device = m_Device.getLogicalHandle();
        VkPhysicalDevice physicalDevice = m_Device.getPhysicalHandle();

        VkDescriptorPoolSize pool_sizes[] =
                {
                        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
                };

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000;
        pool_info.poolSizeCount = std::size(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;

        auto imguiPoolStatus = vkCreateDescriptorPool(device, &pool_info, nullptr, &m_ImguiPool);
        rect_assert(imguiPoolStatus == VK_SUCCESS, "Failed to initialize ImGui descriptor pool")

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        IO.IniFilename = "RDK.ini";

        IO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
        IO.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        IO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
        IO.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        if (IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            STYLE.WindowRounding = 0.0f;
            COLORS[ImGuiCol_WindowBg].w = 1.0f;
        }

        IO.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
        IO.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

//        IO.MouseDrawCursor = true;

//        setTheme();

        bool glfwStatus = ImGui_ImplGlfw_InitForVulkan((GLFWwindow*) m_Window->getHandle(), true);
        rect_assert(glfwStatus, "Failed to initialize ImGui GLFW bindings")

        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = m_Handle;
        init_info.PhysicalDevice = physicalDevice;
        init_info.Device = device;
        init_info.QueueFamily = m_Queue.getFamilyIndices().graphicsFamily;
        init_info.Queue = m_Queue.getGraphicsHandle();
        init_info.DescriptorPool = m_ImguiPool;
        init_info.Allocator = nullptr;
        init_info.MinImageCount = 1;
        init_info.ImageCount = 1;
        init_info.CheckVkResultFn = handleImGuiVkResult;
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

        bool vulkanStatus = ImGui_ImplVulkan_Init(&init_info, m_RenderPass->getHandle());
        rect_assert(vulkanStatus, "Failed to initialize ImGui Vulkan bindings")
        // load fonts
        VkCommandBuffer tempCommand = m_CommandPool.beginTempCommand();
        ImGui_ImplVulkan_CreateFontsTexture(tempCommand);
        m_CommandPool.endTempCommand();
#endif
    }

    void Renderer::destroyUI() {
#ifdef IMGUI
        vkDestroyDescriptorPool(m_Device.getLogicalHandle(), m_ImguiPool, nullptr);
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
#endif
    }

}