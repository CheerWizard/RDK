#include <VertexFormat.h>

namespace rdk {

    VertexInput::VertexInput(
            const VkVertexInputBindingDescription &bindDesc,
            const std::vector<VkVertexInputAttributeDescription> &attrs
    ) {
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        info.vertexBindingDescriptionCount = 1;
        info.pVertexBindingDescriptions = &bindDesc;
        info.vertexAttributeDescriptionCount = static_cast<u32>(attrs.size());
        info.pVertexAttributeDescriptions = data(attrs);
    }

}