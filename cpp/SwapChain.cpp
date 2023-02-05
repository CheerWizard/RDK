#include <SwapChain.h>
#include <math/math.h>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32

namespace rdk {

    void SwapChain::create(void* window, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, const QueueFamilyIndices& indices) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface);
        // select swap chain format
        VkSurfaceFormatKHR surfaceFormat = selectSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = selectSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = selectSwapExtent((GLFWwindow*) window, swapChainSupport.capabilities);
        // eval image count
        auto* capabilities = &swapChainSupport.capabilities;
        u32 imageCount = capabilities->minImageCount + 1;
        if (capabilities->maxImageCount > 0 && imageCount > capabilities->maxImageCount) {
            imageCount = capabilities->maxImageCount;
        }
        // setup swap chain info
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        // setup queue families
        int queueFamilyIndices[] = { indices.graphicsFamily, indices.presentationFamily };

        if (indices.graphicsFamily != indices.presentationFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = (u32*) &queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }
        // setup images transform
        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        // setup window blending
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        // setup presentation mode
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        // setup swap chain lifecycle
        createInfo.oldSwapchain = VK_NULL_HANDLE;
        // create swap chain
        auto swapChainStatus = vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_Handle);
        rect_assert(swapChainStatus == VK_SUCCESS, "Failed to create Vulkan swap chain")
        queryImages(imageCount);
        m_ImageFormat = surfaceFormat.format;
        m_Extent.width = extent.width;
        m_Extent.height = extent.height;
    }

    void SwapChain::destroy() {
        m_RenderPass.destroy();
        m_FrameBuffers.clear();
        m_ImageViews.clear();
        vkDestroySwapchainKHR(m_Device, m_Handle, nullptr);
        m_Images.clear();
    }

    void SwapChain::queryImages(u32 imageCount) {
        vkGetSwapchainImagesKHR(m_Device, m_Handle, &imageCount, nullptr);
        m_Images.resize(imageCount);
        vkGetSwapchainImagesKHR(m_Device, m_Handle, &imageCount, m_Images.data());
    }

    void SwapChain::createFrameBuffers() {
        m_FrameBuffers.clear();
        m_FrameBuffers.reserve(m_ImageViews.size());
        for (const auto& imageView : m_ImageViews) {
            VkImageView attachments[] = { imageView.getHandle() };
            int attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
            m_FrameBuffers.emplace_back(
                    m_Device,
                    attachments,
                    attachmentCount,
                    m_RenderPass.getHandle(),
                    m_Extent
            );
        }
    }

    void SwapChain::createImageViews() {
        m_ImageViews.clear();
        m_ImageViews.reserve(m_Images.size());
        for (const auto& image : m_Images) {
            m_ImageViews.emplace_back(m_Device, image, m_ImageFormat);
        }
    }

    void SwapChain::recreate(void* window, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, const QueueFamilyIndices& indices) {
        // handling minimization
        int width = 0, height = 0;
        glfwGetFramebufferSize((GLFWwindow*) window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize((GLFWwindow*) window, &width, &height);
            glfwWaitEvents();
        }
        vkDeviceWaitIdle(m_Device);
        // clean up
        vkDestroySwapchainKHR(m_Device, m_Handle, nullptr);
        // create again
        create(window, physicalDevice, surface, indices);
        createImageViews();
        createFrameBuffers();
    }

    SwapChainSupportDetails SwapChain::querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
        SwapChainSupportDetails details;
        // query base surface capabilities
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);
        // query supported surface format
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
        }
        // query presentation modes
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    VkSurfaceFormatKHR SwapChain::selectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }
        return availableFormats[0];
    }

    VkPresentModeKHR SwapChain::selectSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D SwapChain::selectSwapExtent(void *window, const VkSurfaceCapabilitiesKHR &capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<u32>::max()) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize((GLFWwindow*) window, &width, &height);

            VkExtent2D actualExtent = {
                    static_cast<u32>(width),
                    static_cast<u32>(height)
            };

            actualExtent.width = clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }
}