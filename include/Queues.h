#pragma once

#include <Core.h>

namespace rdk {

    struct QueueFamilyIndices final {
        static const int NONE_FAMILY = -1;

        int graphicsFamily = NONE_FAMILY;
        int presentationFamily = NONE_FAMILY;

        inline bool completed() const {
            return graphicsFamily != NONE_FAMILY && presentationFamily != NONE_FAMILY;
        }
    };

    class Queue final {

    public:
        void create(VkDevice logicalDevice, const QueueFamilyIndices& familyIndices);

        inline VkQueue getGraphicsHandle() {
            return m_GraphicsHandle;
        }

        inline VkQueue getPresentationHandle() {
            return m_PresentationHandle;
        }

        inline QueueFamilyIndices& getFamilyIndices() {
            return m_FamilyIndices;
        }

    private:
        VkQueue m_GraphicsHandle;
        VkQueue m_PresentationHandle;
        QueueFamilyIndices m_FamilyIndices;
    };

}