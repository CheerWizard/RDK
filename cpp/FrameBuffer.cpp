#include <FrameBuffer.h>

namespace rdk {

    FrameBuffer::FrameBuffer(
            VkDevice device,
            const VkImageView* attachments,
            size_t attachmentCount,
            VkRenderPass renderPass,
            const VkExtent2D &extent
    ) {
        VkFramebufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = renderPass;
        info.attachmentCount = attachmentCount;
        info.pAttachments = attachments;
        info.width = extent.width;
        info.height = extent.height;
        info.layers = 1;
        m_Device = device;
        auto status = vkCreateFramebuffer(device, &info, nullptr, &m_Handle);
        rect_assert(status == VK_SUCCESS, "Failed to create Vulkan framebuffer")
    }

    FrameBuffer::~FrameBuffer() {
        vkDestroyFramebuffer(m_Device, m_Handle, nullptr);
    }

}
