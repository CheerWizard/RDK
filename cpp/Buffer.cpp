#include <Buffer.h>

namespace rdk {

    void Buffer::create(
            VkDeviceSize size,
            VkDevice device,
            VkPhysicalDevice physicalDevice,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags props
    ) {
        m_LogicalDevice = device;

        VkBufferCreateInfo info {};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.size = size;
        info.usage = usage;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        auto state = vkCreateBuffer(m_LogicalDevice, &info, nullptr, &m_Handle);
        rect_assert(state == VK_SUCCESS, "Failed to create Vulkan buffer object")

        // allocate memory and associate it with current buffer
        allocateMemory(physicalDevice, props);
        bindMemory();
    }

    void Buffer::destroy() {
        vkDestroyBuffer(m_LogicalDevice, m_Handle, nullptr);
        freeMemory();
    }

    void Buffer::bindMemory() {
        vkBindBufferMemory(m_LogicalDevice, m_Handle, m_Memory, 0);
    }

    void Buffer::allocateMemory(VkPhysicalDevice physicalDevice, VkMemoryPropertyFlags props) {
        VkDevice device = m_LogicalDevice;

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, m_Handle, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, props);

        auto status = vkAllocateMemory(device, &allocInfo, nullptr, &m_Memory);
        rect_assert(status == VK_SUCCESS, "Failed to allocate memory for Vulkan vertex buffer")
    }

    void Buffer::freeMemory() {
        vkFreeMemory(m_LogicalDevice, m_Memory, nullptr);
    }

    void* Buffer::mapMemory(VkDeviceSize size) {
        void* block;
        vkMapMemory(m_LogicalDevice, m_Memory, 0, size, 0, &block);
        return block;
    }

    void Buffer::unmapMemory() {
        vkUnmapMemory(m_LogicalDevice, m_Memory);
    }

    void Buffer::bindVertex(VkCommandBuffer commandBuffer) {
        VkBuffer vertexBuffers[] = { m_Handle };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    }

    void Buffer::bindIndex(VkCommandBuffer commandBuffer) {
        vkCmdBindIndexBuffer(
                commandBuffer,
                m_Handle,
                0,
                VK_INDEX_TYPE_UINT32      //todo for now we require only u32 indices
        );
    }

    u32 Buffer::findMemoryType(VkPhysicalDevice physicalDevice, u32 typeFilter, VkMemoryPropertyFlags props) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
        u32 memoryTypeCount = memProperties.memoryTypeCount;

        for (u32 i = 0; i < memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & props) == props) {
                return i;
            }
        }

        rect_assert(false, "Failed to find Vulkan suitable memory type")
        return 0;
    }
}