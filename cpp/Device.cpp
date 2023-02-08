#include <Device.h>
#include <Core.h>
#include <SwapChain.h>

#include <set>
#include <stdexcept>
#include <iostream>

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

        vkGetPhysicalDeviceProperties(m_PhysicalHandle, &m_Props);
        vkGetPhysicalDeviceFeatures(m_PhysicalHandle, &m_Features);
    }

    VkFormat Device::findSupportedFormat(
            const std::vector<VkFormat>& candidates,
            VkImageTiling tiling,
            VkFormatFeatureFlags features
    ) {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(m_PhysicalHandle, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        std::cerr << "Device::findSupportedFormat: No format has found!" << std::endl;
        throw std::runtime_error("Failed to find supported image format");
    }

    void Device::destroy() {
        vkDestroyDevice(m_LogicalHandle, nullptr);
    }

    void Device::waitIdle() {
        vkDeviceWaitIdle(m_LogicalHandle);
    }

    bool Device::isSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
        bool suitable = findQueueFamily(physicalDevice, surface).completed();

        bool extensionSupport = isExtensionSupported(physicalDevice);
        suitable = suitable && extensionSupport;

        bool swapChainSupport = false;
        if (extensionSupport) {
            SwapChainSupportDetails swapChainSupportDetails = SwapChain::querySwapChainSupport(physicalDevice, surface);
            swapChainSupport = !swapChainSupportDetails.formats.empty() && !swapChainSupportDetails.presentModes.empty();
        }
        suitable = suitable && swapChainSupport;

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

        suitable = suitable && supportedFeatures.samplerAnisotropy;

        return suitable;
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

    VkFormat Device::findDepthFormat() {
        return findSupportedFormat(
                { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
                VK_IMAGE_TILING_OPTIMAL,
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    bool Device::isLinearFilterSupported(VkFormat format) {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(m_PhysicalHandle, format, &formatProperties);
        return formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
    }

    VkPhysicalDeviceFeatures Device::queryFeatures() const {
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(m_PhysicalHandle, &features);
        return features;
    }

    VkPhysicalDeviceProperties Device::queryProps() const {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(m_PhysicalHandle, &props);
        return props;
    }

}