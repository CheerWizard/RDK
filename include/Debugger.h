#pragma once

#include <Core.h>

namespace rdk {

    class Debugger final {

    public:
        void create(void* client);
        void destroy();

        inline void setClient(void* client) {
            m_Client = client;
        }

    public:

#ifdef VULKAN

        static void setMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

#endif

    private:
        void* m_Handle;
        void* m_Client;
    };

}