#pragma once

#include <VertexFormat.h>
#include <Device.h>

namespace rdk {

    class Buffer final {

    public:
        [[nodiscard]] inline VkBuffer getHandle() { return m_Handle; }

        void create(
                VkDeviceSize size,
                VkDevice device,
                VkPhysicalDevice physicalDevice,
                VkBufferUsageFlags usage,
                VkMemoryPropertyFlags props
        );
        void destroy();

        void* mapMemory(VkDeviceSize size);
        void unmapMemory();

        void bindVertex(VkCommandBuffer commandBuffer);
        void bindIndex(VkCommandBuffer commandBuffer);

        void bindMemory();

        static u32 findMemoryType(VkPhysicalDevice physicalDevice, u32 typeFilter, VkMemoryPropertyFlags props);

    private:
        void allocateMemory(VkPhysicalDevice physicalDevice, VkMemoryPropertyFlags props);
        void freeMemory();

    private:
        VkBuffer m_Handle;
        VkDevice m_LogicalDevice;
        VkDeviceMemory m_Memory;
    };

}