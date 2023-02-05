#pragma once

#include <Core.h>
#include <vector>

namespace rdk {

    class FrameBuffer final {

    public:
        FrameBuffer() = default;
        FrameBuffer(
                VkDevice device,
                const VkImageView* attachments,
                size_t attachmentCount,
                VkRenderPass renderPass,
                const VkExtent2D& extent);
        ~FrameBuffer();

    public:
        inline VkFramebuffer getHandle() {
            return m_Handle;
        }

    private:
        VkFramebuffer m_Handle;
        VkDevice m_Device;
    };

}
