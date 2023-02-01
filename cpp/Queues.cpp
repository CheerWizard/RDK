#include <Queues.h>
#include <Core.h>

namespace rdk {

    void Queue::create(void *logicalDevice, const QueueFamilyIndices &familyIndices) {
        VkQueue graphicsQueue;
        VkQueue presentationQueue;
        vkGetDeviceQueue((VkDevice) logicalDevice, familyIndices.graphicsFamily, 0, (VkQueue*) &graphicsQueue);
        vkGetDeviceQueue((VkDevice) logicalDevice, familyIndices.presentationFamily, 0, (VkQueue*) &presentationQueue);
        m_GraphicsHandle = graphicsQueue;
        m_PresentationHandle = presentationQueue;
        m_FamilyIndices = familyIndices;
    }

}

