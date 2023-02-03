#include <Queues.h>
#include <Core.h>

namespace rdk {

    void Queue::create(VkDevice logicalDevice, const QueueFamilyIndices &familyIndices) {
        vkGetDeviceQueue(logicalDevice, familyIndices.graphicsFamily, 0, (VkQueue*) &m_GraphicsHandle);
        vkGetDeviceQueue(logicalDevice, familyIndices.presentationFamily, 0, (VkQueue*) &m_PresentationHandle);
        m_FamilyIndices = familyIndices;
    }

}

