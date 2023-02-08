#pragma once

#include <Core.h>

namespace rdk {

    class Debugger final {

    public:
        void create(VkInstance client);
        void destroy();

    public:
        static void setMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    private:
        VkDebugUtilsMessengerEXT m_Handle;
        VkInstance m_Client;
    };

}