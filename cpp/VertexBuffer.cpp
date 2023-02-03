#include <VertexBuffer.h>

namespace rdk {

    void VertexBuffer::create(const Device& device, const VertexFormat& vertexFormat, u32 vertexCount) {
        m_Device = device;
        m_VertexFormat = vertexFormat;
        u32 stride = vertexFormat.bindDesc.stride;

        m_Info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        m_Info.size = stride * vertexCount;
        m_Info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        m_Info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        auto state = vkCreateBuffer(m_Device.getLogicalHandle(), &m_Info, nullptr, &m_Handle);
        rect_assert(state == VK_SUCCESS, "Failed to create Vulkan vertex buffer")

        // allocate memory and associate it with current buffer
        allocateMemory();
        bindMemory();
    }

    void VertexBuffer::destroy() {
        vkDestroyBuffer(m_Device.getLogicalHandle(), m_Handle, nullptr);
        freeMemory();
    }

    void VertexBuffer::allocateMemory() {
        VkDevice device = m_Device.getLogicalHandle();

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, m_Handle, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = m_Device.findMemoryType(
                memRequirements.memoryTypeBits,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        auto status = vkAllocateMemory(device, &allocInfo, nullptr, &m_Memory);
        rect_assert(status == VK_SUCCESS, "Failed to allocate memory for Vulkan vertex buffer")
    }

    void VertexBuffer::bindMemory() {
        vkBindBufferMemory(m_Device.getLogicalHandle(), m_Handle, m_Memory, 0);
    }

    void VertexBuffer::freeMemory() {
        vkFreeMemory(m_Device.getLogicalHandle(), m_Memory, nullptr);
    }

    void VertexBuffer::upload(const DrawData& drawData) {
        VkDevice device = m_Device.getLogicalHandle();
        VkDeviceMemory memory = m_Memory;
        size_t size = drawData.vertexCount * m_VertexFormat.bindDesc.stride;
        void* dstBlock;

        vkMapMemory(device, memory, 0, size, 0, &dstBlock);
        memcpy(dstBlock, drawData.vertices, size);
        vkUnmapMemory(device, memory);
    }

    void VertexBuffer::bind(VkCommandBuffer commandBuffer) {
        VkBuffer vertexBuffers[] = { m_Handle };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    }
}