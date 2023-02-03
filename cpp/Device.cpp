#include <Device.h>
#include <Core.h>
#include <SwapChain.h>

#include <set>
#include <string>

namespace rdk {

    void Device::create(VkInstance client, VkSurfaceKHR surface) {
        setClient(client);
        m_PhysicalHandle = VK_NULL_HANDLE;
        // eval devices count
        u32 deviceCount = 0;
        vkEnumeratePhysicalDevices(m_Client, &deviceCount, nullptr);
        rect_assert(deviceCount != 0, "Failed to setup Vulkan physical device")
        // eval devices handles
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_Client, &deviceCount, devices.data());
        // find suitable device
        for (const auto& device: devices) {
            if (isSuitable(device, surface)) {
                m_PhysicalHandle = device;
                break;
            }
        }
        rect_assert(m_PhysicalHandle != VK_NULL_HANDLE, "Failed to find a suitable GPU")
        // setup queue commands
        QueueFamilyIndices indices = findQueueFamily(surface);
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<int> uniqueQueueFamilies = {
                indices.graphicsFamily,
                indices.presentationFamily
        };
        float queuePriority = 1.0f;
        for (int queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }
        // setup device features
        VkPhysicalDeviceFeatures deviceFeatures{};
        // setup logical device
        VkDeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceCreateInfo.queueCreateInfoCount = queueCreateInfos.size();
        deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
#ifdef VALIDATION_LAYERS
        deviceCreateInfo.enabledLayerCount = static_cast<u32>(m_ValidationLayers.size());
        deviceCreateInfo.ppEnabledLayerNames = m_ValidationLayers.data();
#else
        deviceCreateInfo.enabledLayerCount = 0;
#endif
        deviceCreateInfo.enabledExtensionCount = static_cast<u32>(m_Extensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = m_Extensions.data();
        // create and assert logical device
        auto logicalDeviceStatus = vkCreateDevice(
                m_PhysicalHandle,
                &deviceCreateInfo,
                nullptr,
                &m_LogicalHandle
        );
        rect_assert(logicalDeviceStatus == VK_SUCCESS, "Failed to create Vulkan logical device")
    }

    uint32_t Device::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_PhysicalHandle, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        rect_assert(false, "Failed to find Vulkan suitable memory type")
        return 0;
    }

    void Device::destroy() {
        vkDestroyDevice(m_LogicalHandle, nullptr);
    }

    void Device::waitIdle() {
        vkDeviceWaitIdle(m_LogicalHandle);
    }

    bool Device::isSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
        bool extensionSupport = isExtensionSupported(physicalDevice);
        bool swapChainSupport = false;
        if (extensionSupport) {
            SwapChainSupportDetails swapChainSupportDetails = SwapChain::querySwapChainSupport(physicalDevice, surface);
            swapChainSupport = !swapChainSupportDetails.formats.empty() && !swapChainSupportDetails.presentModes.empty();
        }
        return findQueueFamily(physicalDevice, surface).completed() && extensionSupport && swapChainSupport;
    }

    bool Device::isExtensionSupported(VkPhysicalDevice physicalDevice) {
        u32 extensionCount;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(m_Extensions.begin(), m_Extensions.end());

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    QueueFamilyIndices Device::findQueueFamily(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            // check for graphics support
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                indices.graphicsFamily = i;
            // check for presentation support
            VkBool32 presentationSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentationSupport);
            if (presentationSupport)
                indices.presentationFamily = i;

            if (indices.completed())
                break;

            i++;
        }

        return indices;
    }

    QueueFamilyIndices Device::findQueueFamily(VkSurfaceKHR surface) {
        return findQueueFamily(m_PhysicalHandle, surface);
    }

    bool Device::isLayerValidationSupported() {
        m_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };

        u32 layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char *layerName: m_ValidationLayers) {
            bool layerFound = false;

            for (const auto &layerProperties: availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }

}