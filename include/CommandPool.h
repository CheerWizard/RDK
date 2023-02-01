#pragma once

#include <Pipeline.h>
#include <Queues.h>
#include <Device.h>

namespace rdk {

    class CommandBuffer final {

    public:
        void create(void* commandPool);
        void destroy(void* commandPool);

        inline void* getHandle() {
            return m_Handle;
        }

        void setHandle(void* handle);

        inline void setLogicalDevice(void* logicalDevice) {
            m_LogicalDevice = logicalDevice;
        }

        void begin();
        void end();
        void reset();

    private:
        void* m_Handle;
        void* m_LogicalDevice;
    };

    class CommandPool final {

    public:
        CommandPool() = default;
        CommandPool(void* window, void* surface, const Device& device);

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
        void drawFrame(u32 vertexCount, u32 instanceCount);

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
        void* m_Handle;
        Device m_Device;
        void* m_Window;
        void* m_Surface;
        QueueFamilyIndices m_FamilyIndices;
        std::vector<CommandBuffer> m_Buffers;
        Pipeline m_Pipeline;
        // sync objects
        u32 m_MaxFramesInFlight = 2;
        u32 m_CurrentFrame = 0;
        bool m_FrameBufferResized = false;
        std::vector<void*> m_ImageAvailableSemaphore;
        std::vector<void*> m_RenderFinishedSemaphore;
        std::vector<void*> m_FlightFence;
        Queue m_Queue;
    };

}