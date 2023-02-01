#pragma once

#include <Core.h>

namespace rdk {

    struct Extent2D final {
        u32 width, height;
    };

    class FrameBuffer final {

    public:
        void create(void* imageView, void* renderPass, const Extent2D& extent);
        void destroy();

        inline void* getHandle() {
            return m_Handle;
        }

        inline void setLogicalDevice(void* logicalDevice) {
            m_LogicalDevice = logicalDevice;
        }

    private:
        void* m_Handle;
        void* m_LogicalDevice;
    };

}
