#include <SwapChain.h>
#include <math/math.h>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32

namespace rdk {

    void SwapChain::create(void* window, void* physicalDevice, void* surface, const QueueFamilyIndices& indices) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface);
        // select swap chain format
        VkSurfaceFormatKHR surfaceFormat = selectSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = selectSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = selectSwapExtent((GLFWwindow*) window, swapChainSupport.capabilities);
        // eval image count
        auto* capabilities = (VkSurfaceCapabilitiesKHR*) &swapChainSupport.capabilities;
        u32 imageCount = capabilities->minImageCount + 1;
        if (capabilities->maxImageCount > 0 && imageCount > capabilities->maxImageCount) {
            imageCount = capabilities->maxImageCount;
        }
        // setup swap chain info
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = (VkSurfaceKHR) surface;
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
        auto swapChainStatus = vkCreateSwapchainKHR((VkDevice) m_LogicalDevice, &createInfo, nullptr, (VkSwapchainKHR*) &m_Handle);
        rect_assert(swapChainStatus == VK_SUCCESS, "Failed to create Vulkan swap chain");
        queryImages(imageCount);
        m_ImageFormat = surfaceFormat.format;
        m_Extent.width = extent.width;
        m_Extent.height = extent.height;
    }

    void SwapChain::destroy() {
        m_RenderPass.destroy();
        for (const auto& imageView : m_ImageViews) {
            vkDestroyImageView((VkDevice) m_LogicalDevice, (VkImageView) imageView, nullptr);
        }
        m_ImageViews.clear();
        for (auto& framebuffer : m_FrameBuffers) {
            framebuffer.destroy();
        }
        m_FrameBuffers.clear();
        vkDestroySwapchainKHR((VkDevice) m_LogicalDevice, (VkSwapchainKHR) m_Handle, nullptr);
        m_Images.clear();
    }

    void SwapChain::queryImages(u32 imageCount) {
        std::vector<VkImage> swapChainImages;
        vkGetSwapchainImagesKHR((VkDevice) m_LogicalDevice, (VkSwapchainKHR) m_Handle, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR((VkDevice) m_LogicalDevice, (VkSwapchainKHR) m_Handle, &imageCount, swapChainImages.data());
        m_Images.clear();
        for (const auto& image : swapChainImages) {
            m_Images.emplace_back(image);
        }
    }

    void SwapChain::createFrameBuffers() {
        m_FrameBuffers.resize(m_ImageViews.size());
        for (size_t i = 0; i < m_FrameBuffers.size(); i++) {
            auto& frameBuffer = m_FrameBuffers[i];
            frameBuffer.setLogicalDevice(m_LogicalDevice);
            frameBuffer.create(m_ImageViews[i], m_RenderPass.getHandle(), m_Extent);
        }
    }

    void SwapChain::createImageViews() {
        m_ImageViews.resize(m_Images.size());
        for (u32 i = 0 ; i < m_ImageViews.size() ; i++) {
            // setup image view info
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = (VkImage) m_Images[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = (VkFormat) m_ImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;
            // create image view
            auto imageViewStatus = vkCreateImageView((VkDevice) m_LogicalDevice, &createInfo, nullptr, (VkImageView*) &m_ImageViews[i]);
            rect_assert(imageViewStatus == VK_SUCCESS, "Failed to create Vulkan image view")
        }
    }

    void SwapChain::recreate(void* window, void* physicalDevice, void* surface, const QueueFamilyIndices& indices) {
        // handling minimization
        int width = 0, height = 0;
        glfwGetFramebufferSize((GLFWwindow*) window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize((GLFWwindow*) window, &width, &height);
            glfwWaitEvents();
        }
        vkDeviceWaitIdle((VkDevice) m_LogicalDevice);
        // clean up
        for (const auto& imageView : m_ImageViews) {
            vkDestroyImageView((VkDevice) m_LogicalDevice, (VkImageView) imageView, nullptr);
        }
        m_ImageViews.clear();
        for (auto& framebuffer : m_FrameBuffers) {
            framebuffer.destroy();
        }
        m_FrameBuffers.clear();
        vkDestroySwapchainKHR((VkDevice) m_LogicalDevice, (VkSwapchainKHR) m_Handle, nullptr);
        // create again
        create(window, physicalDevice, surface, indices);
        createImageViews();
        createFrameBuffers();
    }

    SwapChainSupportDetails SwapChain::querySwapChainSupport(void *physicalDevice, void *surfaceHandle) {
        SwapChainSupportDetails details;
        auto device = (VkPhysicalDevice) physicalDevice;
        auto surface = (VkSurfaceKHR) surfaceHandle;
        auto* capabilities = (VkSurfaceCapabilitiesKHR*) &details.capabilities;
        // query base surface capabilities
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, capabilities);
        // query supported surface format
        uint32_t formatCount;
        ;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, (VkSurfaceFormatKHR*) details.formats.data());
        }
        // query presentation modes
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, (VkPresentModeKHR*) details.presentModes.data());
        }

        return details;
    }

#ifdef VULKAN

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

#endif

}