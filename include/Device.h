#pragma once

#include <Queues.h>

#include <vector>

namespace rdk {

    class Device final {

    public:
        void create(void* client, void* surface);
        void destroy();

        void waitIdle();

        bool isExtensionSupported(void* physicalDevice);
        bool isSuitable(void* physicalDevice, void* surface);
        QueueFamilyIndices findQueueFamily(void* physicalDevice, void* surface);
        QueueFamilyIndices findQueueFamily(void* surface);

        inline void setClient(void* client) {
            m_Client = client;
        }

        inline void* getPhysicalHandle() {
            return m_PhysicalHandle;
        }

        inline void* getLogicalHandle() {
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
        void* m_Client;
        void* m_PhysicalHandle;
        void* m_LogicalHandle;
        std::vector<const char*> m_Extensions;
        std::vector<const char*> m_ValidationLayers;
    };

}