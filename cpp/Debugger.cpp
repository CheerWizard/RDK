#include <Debugger.h>
#include <Core.h>

namespace rdk {

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
            void *pUserData
    ) {
        printf("Vulkan DEBUG callback: %s\n", pCallbackData->pMessage);
        if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            breakpoint();
        }
        return VK_FALSE;
    }

    static VkResult createDebugUtilsMessenger(
            VkInstance instance,
            const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
            const VkAllocationCallbacks *pAllocator,
            VkDebugUtilsMessengerEXT *pDebugMessenger
    ) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
                                                                               "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    static void destroyDebugUtilsMessenger(
            VkInstance instance,
            VkDebugUtilsMessengerEXT debugMessenger,
            const VkAllocationCallbacks *pAllocator
    ) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
                                                                                "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }

    void Debugger::create(void* client) {
        setClient(client);
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        setMessengerCreateInfo(createInfo);
        VkResult debuggerStatus = createDebugUtilsMessenger(
                (VkInstance) m_Client,
                &createInfo, nullptr,
                (VkDebugUtilsMessengerEXT*) &m_Handle
        );
        rect_assert(debuggerStatus == VK_SUCCESS, "Failed to setup Vulkan debugger")
    }

    void Debugger::destroy() {
        destroyDebugUtilsMessenger((VkInstance) m_Client, (VkDebugUtilsMessengerEXT) m_Handle, nullptr);
    }

    void Debugger::setMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                                     | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                     | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                                 | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                 | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

}