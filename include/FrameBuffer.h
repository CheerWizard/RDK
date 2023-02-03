#pragma once

#include <Core.h>
#include <vector>

namespace rdk {

    class FrameBuffer final {

    public:
        void create(const std::vector<VkImageView>& attachments, VkRenderPass renderPass, const VkExtent2D& extent);
        void destroy();

        inline VkFramebuffer getHandle() {
            return m_Handle;
        }

        inline void setLogicalDevice(VkDevice logicalDevice) {
            m_LogicalDevice = logicalDevice;
        }

    private:
        VkFramebuffer m_Handle;
        VkDevice m_LogicalDevice;
    };

}
