#include <VertexFormat.h>

namespace rdk {

    VertexFormat::VertexFormat(
            const VkVertexInputBindingDescription &bindDesc,
            const std::vector<VkVertexInputAttributeDescription> &attrs
    ) : bindDesc(bindDesc), attrs(attrs) {
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindDesc;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<u32>(attrs.size());
        vertexInputInfo.pVertexAttributeDescriptions = data(attrs);
    }

}