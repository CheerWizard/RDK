#pragma once

#include <Pipeline.h>
#include <Queues.h>
#include <Device.h>

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
        CommandPool(void* window, VkSurfaceKHR surface, const Device& device);

    public:
        void create();

        inline void setQueue(const Queue& queue) {
            m_Queue = queue;
        }

        inline void setPipeline(const Pipeline& pipeline) {
            m_Pipeline = pipeline;
        }

        void destroy();
        void addCommandBuffer(const CommandBuffer& commandBuffer);
        void drawFrame(const DrawData& drawData, u32 instanceCount);

        inline void setMaxFramesInFlight(u32 maxFramesInFlight) {
            m_MaxFramesInFlight = maxFramesInFlight;
        }

        inline void setFrameBufferResized(bool resized) {
            m_FrameBufferResized = resized;
        }

    private:
        void createBuffers();
        void destroyBuffers();
        void createSyncObjects();
        void destroySyncObjects();

    private:
        VkCommandPool m_Handle;
        Device m_Device;
        void* m_Window;
        VkSurfaceKHR m_Surface;
        std::vector<CommandBuffer> m_Buffers;
        Pipeline m_Pipeline;
        // sync objects
        u32 m_MaxFramesInFlight = 2;
        u32 m_CurrentFrame = 0;
        bool m_FrameBufferResized = false;
        std::vector<VkSemaphore> m_ImageAvailableSemaphore;
        std::vector<VkSemaphore> m_RenderFinishedSemaphore;
        std::vector<VkFence> m_FlightFence;
        Queue m_Queue;
    };

}