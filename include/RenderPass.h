#pragma once

#include <Core.h>

namespace rdk {

    class RenderPass final {

    public:
        RenderPass() = default;

        RenderPass(VkDevice device, VkFormat colorFormat, VkFormat depthFormat);

        ~RenderPass();

    public:
        [[nodiscard]] inline VkRenderPass getHandle() const {
            return m_Handle;
        }

    private:
        VkRenderPass m_Handle;
        VkDevice m_Device;
    };

}