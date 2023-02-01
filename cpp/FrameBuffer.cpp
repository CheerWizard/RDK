#include <FrameBuffer.h>

namespace rdk {

    void FrameBuffer::create(void* imageView, void* renderPass, const Extent2D& extent) {
        VkImageView attachments[] = { (VkImageView) imageView };
        VkFramebufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = (VkRenderPass) renderPass;
        info.attachmentCount = 1;
        info.pAttachments = attachments;
        info.width = extent.width;
        info.height = extent.height;
        info.layers = 1;
        auto status = vkCreateFramebuffer((VkDevice) m_LogicalDevice, &info, nullptr, (VkFramebuffer*) &m_Handle);
        rect_assert(status == VK_SUCCESS, "Failed to create Vulkan framebuffer")
    }

    void FrameBuffer::destroy() {
        vkDestroyFramebuffer((VkDevice) m_LogicalDevice, (VkFramebuffer) m_Handle, nullptr);
    }

}
