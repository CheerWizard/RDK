#include <FrameBuffer.h>

namespace rdk {

    void FrameBuffer::create(const std::vector<VkImageView>& attachments, VkRenderPass renderPass, const VkExtent2D& extent) {
        VkFramebufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = renderPass;
        info.attachmentCount = attachments.size();
        info.pAttachments = attachments.data();
        info.width = extent.width;
        info.height = extent.height;
        info.layers = 1;
        auto status = vkCreateFramebuffer(m_LogicalDevice, &info, nullptr, &m_Handle);
        rect_assert(status == VK_SUCCESS, "Failed to create Vulkan framebuffer")
    }

    void FrameBuffer::destroy() {
        vkDestroyFramebuffer(m_LogicalDevice, m_Handle, nullptr);
    }

}
