#include <VertexBuffer.h>

namespace rdk {

    static VkVertexInputBindingDescription getBindingDescription(VertexFormat& vertexFormat) {
        VkVertexInputBindingDescription vkDescription {};

        auto description = vertexFormat.getDescriptor();
        vkDescription.binding = description.binding;
        vkDescription.stride = description.stride;
        vkDescription.inputRate = static_cast<VkVertexInputRate>(description.inputRate);

        return vkDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttrs(VertexFormat& vertexFormat) {
        std::vector<VkVertexInputAttributeDescription> attrs;

        for (const auto& attr : vertexFormat.getAttrs()) {
            VkVertexInputAttributeDescription vkAttr {};
            vkAttr.binding = attr.binding;
            vkAttr.format = static_cast<VkFormat>(attr.format);
            vkAttr.offset = attr.offset;
            vkAttr.location = attr.location;
            attrs.emplace_back(vkAttr);
        }

        return attrs;
    }

    void VertexBuffer::create(void *logicalDevice) {
        
    }

    void VertexBuffer::destroy() {

    }

}