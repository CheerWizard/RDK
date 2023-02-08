#pragma once

#include <Pipeline.h>
#include <Queues.h>
#include <Device.h>
#include <DescriptorPool.h>
#include <Window.h>

#ifdef IMGUI
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <GLFW/glfw3.h>
#include <backends/imgui_impl_vulkan.h>
#endif

namespace rdk {

    class CommandBuffer final {

    public:
        void create(VkCommandPool commandPool, u32 count = 1);
        void destroy(VkCommandPool commandPool, u32 count = 1);

        inline VkCommandBuffer getHandle() {
            return m_Handle;
        }

        inline void setHandle(VkCommandBuffer handle) {
            m_Handle = handle;
        }

        inline void setLogicalDevice(VkDevice logicalDevice) {
            m_LogicalDevice = logicalDevice;
        }

        void begin();
        void end();
        void reset();

    private:
        VkCommandBuffer m_Handle;
        VkDevice m_LogicalDevice;
    };

    class CommandPool final {

    public:
        CommandPool() = default;
        CommandPool(
                VkInstance instance,
                Window* window,
                VkSurfaceKHR surface,
                Device* device,
                DescriptorPool* descriptorPool,
                Queue* queue,
                Pipeline* pipeline
        ) : m_Instance(instance), m_Window(window), m_Surface(surface), m_Device(device),
        m_DescriptorPool(descriptorPool), m_Queue(queue), m_Pipeline(pipeline) {}

    public:
        inline void setMaxFramesInFlight(u32 maxFramesInFlight) {
            m_MaxFramesInFlight = maxFramesInFlight;
        }

        inline void setFrameBufferResized(bool resized) {
            m_FrameBufferResized = resized;
        }

        [[nodiscard]] inline VkCommandBuffer getCurrentBuffer() {
            return m_Buffers[m_CurrentFrame].getHandle();
        }

        [[nodiscard]] inline u32 getMaxFramesInFlight() const {
            return m_MaxFramesInFlight;
        }

        [[nodiscard]] inline u32 getCurrentFrame() const {
            return m_CurrentFrame;
        }

        void create();
        void destroy();

        void beginFrame();
        void endFrame();

        void drawVertices(u32 vertexCount, u32 instanceCount);
        void drawIndices(u32 indexCount, u32 instanceCount);

        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, u32 mipLevels = 1);
        void copyBufferImage(VkBuffer srcBuffer, VkImage dstImage, u32 width, u32 height);
        void CommandPool::generateMipmaps(VkImage image, int width, int height, u32 mipLevels);

        VkCommandBuffer& beginTempCommand();
        void endTempCommand();

        void beginUI();

    private:
        void createBuffers();
        void destroyBuffers();
        void createSyncObjects();
        void destroySyncObjects();

        void renderUIDrawData(ImDrawData* drawData = ImGui::GetDrawData());

        void recreateSwapChain();

    private:
        VkInstance m_Instance;
        VkCommandPool m_Handle;
        Device* m_Device;
        Window* m_Window;
        VkSurfaceKHR m_Surface;
        std::vector<CommandBuffer> m_Buffers;

        Pipeline* m_Pipeline = nullptr;

        // sync objects
        u32 m_MaxFramesInFlight = 2;
        u32 m_CurrentFrame = 0;
        bool m_FrameBufferResized = false;
        std::vector<VkSemaphore> m_ImageAvailableSemaphore;
        std::vector<VkSemaphore> m_RenderFinishedSemaphore;
        std::vector<VkFence> m_FlightFence;
        Queue* m_Queue;

        u32 currentImageIndex;

        DescriptorPool* m_DescriptorPool = nullptr;

        VkCommandBuffer m_TempCommand;

#ifdef IMGUI
        ImGui_ImplVulkanH_Window m_ImGuiData;
#endif
    };

}