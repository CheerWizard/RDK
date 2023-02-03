#pragma once

#include <Core.h>
#include <vector>


namespace rdk {

    struct DrawData final {
        size_t vertexCount = 0;
        void* vertices;
        size_t indexCount = 0;
        u32* indices;
    };

    struct VertexFormat final {
        VkVertexInputBindingDescription bindDesc{};
        std::vector<VkVertexInputAttributeDescription> attrs;
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};

        VertexFormat() = default;

        VertexFormat(
                const VkVertexInputBindingDescription& bindDesc,
                const std::vector<VkVertexInputAttributeDescription>& attrs
        );
    };

}
