#pragma once

#include <Core.h>

namespace rdk {

    class RenderPass final {

    public:
        void create();
        void destroy();

        inline VkRenderPass getHandle() {
            return m_Handle;
        }

        inline void setLogicalDevice(VkDevice logicalDevice) {
            m_LogicalDevice = logicalDevice;
        }

        inline void setFormat(VkFormat format) {
            m_Format = format;
        }

    private:
        VkRenderPass m_Handle;
        VkDevice m_LogicalDevice;
        VkFormat m_Format;
    };

}