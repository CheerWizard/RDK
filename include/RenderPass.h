#pragma once

namespace rdk {

    class RenderPass final {

    public:
        void create();
        void destroy();

        inline void* getHandle() {
            return m_Handle;
        }

        inline void setLogicalDevice(void* logicalDevice) {
            m_LogicalDevice = logicalDevice;
        }

        inline void setFormat(int format) {
            m_Format = format;
        }

    private:
        void* m_Handle;
        void* m_LogicalDevice;
        int m_Format;
    };

}