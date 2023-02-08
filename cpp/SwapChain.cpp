#include <SwapChain.h>
#include <math/math.h>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32

namespace rdk {

    SwapChain::SwapChain(void* window, Device* device, VkSurfaceKHR surface, VkFormat depthFormat) {
        m_Device = device;
        m_DepthFormat = depthFormat;
        m_DepthImage = new Image();
        m_DepthImageView = new ImageView();

        create(window, surface);
        createColorImages();
        createDepthImage();

        m_RenderPass = new RenderPass(m_Device->getLogicalHandle(), m_ColorFormat, m_DepthFormat);

        createFrameBuffers();
    }

    void SwapChain::create(void *window, VkSurfaceKHR surface) {
        VkDevice device = m_Device->getLogicalHandle();
        VkPhysicalDevice physicalDevice = m_Device->getPhysicalHandle();

        QueueFamilyIndices indices = m_Device->findQueueFamily(surface);
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
        auto swapChainStatus = vkCreateSwapchainKHR(device, &createInfo, nullptr, &m_Handle);
        rect_assert(swapChainStatus == VK_SUCCESS, "Failed to create Vulkan swap chain")

        vkGetSwapchainImagesKHR(device, m_Handle, &imageCount, nullptr);
        m_Images.resize(imageCount);
        vkGetSwapchainImagesKHR(device, m_Handle, &imageCount, m_Images.data());
        m_ColorFormat = surfaceFormat.format;
        m_Extent.width = extent.width;
        m_Extent.height = extent.height;
    }

    void SwapChain::createColorImages() {
        m_ImageViews.clear();
        m_ImageViews.reserve(m_Images.size());

        ImageViewInfo imageViewInfo;
        imageViewInfo.format = m_ColorFormat;
        for (const auto& image : m_Images) {
            m_ImageViews.emplace_back(m_Device->getLogicalHandle(), image, imageViewInfo);
        }
    }

    void SwapChain::createFrameBuffers() {
        m_FrameBuffers.clear();
        m_FrameBuffers.reserve(m_ImageViews.size());
        VkRenderPass renderPass = m_RenderPass->getHandle();
        VkExtent2D extent = m_Extent;
        VkDevice device = m_Device->getLogicalHandle();
        VkImageView depthImageView = m_DepthImageView->getHandle();

        for (const auto& imageView : m_ImageViews) {
            VkImageView attachments[] = { imageView.getHandle(), depthImageView };
            int attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
            m_FrameBuffers.emplace_back(
                    device,
                    attachments,
                    attachmentCount,
                    renderPass,
                    extent
            );
        }
    }

    void SwapChain::recreate(void *window, VkSurfaceKHR surface) {
        recreate(window, surface, m_Device->findQueueFamily(surface));
    }

    void SwapChain::recreate(void* window, VkSurfaceKHR surface, const QueueFamilyIndices& familyIndices) {
        vkDestroySwapchainKHR(m_Device->getLogicalHandle(), m_Handle, nullptr);
        m_DepthImageView->~ImageView();
        m_DepthImage->~Image();
        // create again
        create(window, surface);
        createColorImages();
        createDepthImage();
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

    SwapChain::~SwapChain() {
        delete m_RenderPass;

        m_FrameBuffers.clear();

        delete m_DepthImageView;
        delete m_DepthImage;

        m_ImageViews.clear();
        m_Images.clear();

        vkDestroySwapchainKHR(m_Device->getLogicalHandle(), m_Handle, nullptr);
    }

    void SwapChain::createDepthImage() {
        VkExtent2D extent = m_Extent;
        VkFormat depthFormat = m_DepthFormat;
        VkDevice device = m_Device->getLogicalHandle();
        VkPhysicalDevice physicalDevice = m_Device->getPhysicalHandle();

        ImageInfo imageInfo;
        imageInfo.width = extent.width;
        imageInfo.height = extent.height;
        imageInfo.format = depthFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        ImageViewInfo imageViewInfo;
        imageViewInfo.format = depthFormat;
        imageViewInfo.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        new (m_DepthImage) Image(device, physicalDevice, imageInfo);
        new (m_DepthImageView) ImageView(device, m_DepthImage->getHandle(), imageViewInfo);
    }

}