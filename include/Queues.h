#pragma once

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
        void create(void* logicalDevice, const QueueFamilyIndices& familyIndices);

        inline void* getGraphicsHandle() {
            return m_GraphicsHandle;
        }

        inline void* getPresentationHandle() {
            return m_PresentationHandle;
        }

    private:
        void* m_GraphicsHandle;
        void* m_PresentationHandle;
        QueueFamilyIndices m_FamilyIndices;
    };

}