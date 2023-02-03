#pragma once

#include <VertexFormat.h>
#include <Device.h>

namespace rdk {

    class VertexBuffer final {

    public:
        void create(const Device& device, const VertexFormat& vertexFormat, u32 vertexCount = 100);
        void destroy();

        [[nodiscard]] inline const VertexFormat& getVertexFormat() { return m_VertexFormat; }

        void upload(const DrawData& drawData);

        void bind(VkCommandBuffer commandBuffer);

    private:
        void allocateMemory();
        void bindMemory();
        void freeMemory();

    private:
        VkBuffer m_Handle;
        VkBufferCreateInfo m_Info{};
        Device m_Device;
        VertexFormat m_VertexFormat;
        VkDeviceMemory m_Memory;
    };

}