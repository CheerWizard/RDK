#pragma once

#include <Core.h>
#include <vector>


namespace rdk {

    struct VertexData final {
        size_t size;
        void* data;
    };

    struct IndexData final {
        size_t size;
        u32* data;
    };

    struct VertexInput final {
        VkPipelineVertexInputStateCreateInfo info{};

        VertexInput() = default;

        VertexInput(
                const VkVertexInputBindingDescription& bindDesc,
                const std::vector<VkVertexInputAttributeDescription>& attrs
        );
    };

}
