#pragma once

#include <Queues.h>

#include <vector>

namespace rdk {

    class Device final {

    public:
        void create(VkInstance client, VkSurfaceKHR surface);
        void destroy();

        void waitIdle();

        QueueFamilyIndices findQueueFamily(VkSurfaceKHR surface);

        inline void setClient(VkInstance client) {
            m_Client = client;
        }

        inline VkPhysicalDevice getPhysicalHandle() const {
            return m_PhysicalHandle;
        }

        inline VkDevice getLogicalHandle() const {
            return m_LogicalHandle;
        }

        inline const std::vector<const char*>& getExtensions() const {
            return m_Extensions;
        }

        inline void setExtensions(const std::initializer_list<const char*>& extensions) {
            m_Extensions = extensions;
        }

        inline const std::vector<const char*>& getValidationLayers() const {
            return m_ValidationLayers;
        }

        inline const VkPhysicalDeviceProperties& getProperties() const {
            return m_Props;
        }

        inline const VkPhysicalDeviceFeatures& getFeatures() const {
            return m_Features;
        }

        bool isLayerValidationSupported();

        VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
        VkFormat findDepthFormat();

        bool isLinearFilterSupported(VkFormat format);

        VkPhysicalDeviceFeatures queryFeatures() const;
        VkPhysicalDeviceProperties queryProps() const;

    private:
        bool isExtensionSupported(VkPhysicalDevice physicalDevice);
        bool isSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
        QueueFamilyIndices findQueueFamily(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

    private:
        VkInstance m_Client;

        VkPhysicalDevice m_PhysicalHandle;
        VkDevice m_LogicalHandle;

        std::vector<const char*> m_Extensions;
        std::vector<const char*> m_ValidationLayers;

        VkPhysicalDeviceProperties m_Props;
        VkPhysicalDeviceFeatures m_Features;
    };

}