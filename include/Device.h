#pragma once

#include <Queues.h>

#include <vector>

namespace rdk {

    class Device final {

    public:
        void create(VkInstance client, VkSurfaceKHR surface);
        void destroy();

        void waitIdle();

        uint32_t Device::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

        bool isExtensionSupported(VkPhysicalDevice physicalDevice);
        bool isSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
        QueueFamilyIndices findQueueFamily(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
        QueueFamilyIndices findQueueFamily(VkSurfaceKHR surface);

        inline void setClient(VkInstance client) {
            m_Client = client;
        }

        inline VkPhysicalDevice getPhysicalHandle() {
            return m_PhysicalHandle;
        }

        inline VkDevice getLogicalHandle() {
            return m_LogicalHandle;
        }

        inline std::vector<const char*>& getExtensions() {
            return m_Extensions;
        }

        inline void setExtensions(const std::initializer_list<const char*>& extensions) {
            m_Extensions = extensions;
        }

        inline std::vector<const char*>& getValidationLayers() {
            return m_ValidationLayers;
        }

        bool isLayerValidationSupported();

    private:
        VkInstance m_Client;
        VkPhysicalDevice m_PhysicalHandle;
        VkDevice m_LogicalHandle;
        std::vector<const char*> m_Extensions;
        std::vector<const char*> m_ValidationLayers;
    };

}