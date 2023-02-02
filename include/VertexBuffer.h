#pragma once

#include <VertexFormat.h>

namespace rdk {

    class VertexBuffer final {

    public:
        void create(void* logicalDevice);
        void destroy();

    private:
        void* m_Handle;
        void* m_Memory;
        void* m_LogicalDevice;
        VertexFormat m_VertexFormat;
    };

}