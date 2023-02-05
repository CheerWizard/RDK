#pragma once

#include <RenderPass.h>
#include <FrameBuffer.h>
#include <Queues.h>
#include <Image.h>

#include <vector>

namespace rdk {

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    class SwapChain final {

    public:
        void create(void* window, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, const QueueFamilyIndices& indices);
        void destroy();
        void queryImages(u32 imageCount);

        inline VkSwapchainKHR getHandle() {
            return m_Handle;
        }

        inline void setLogicalDevice(VkDevice logicalDevice) {
            m_Device = logicalDevice;
        }

        inline const VkExtent2D& getExtent() {
            return m_Extent;
        }

        inline VkFormat getImageFormat() const {
            return m_ImageFormat;
        }

        inline void setRenderPass(const RenderPass& renderPass) {
            m_RenderPass = renderPass;
        }

        inline RenderPass& getRenderPass() {
            return m_RenderPass;
        }

        inline VkFramebuffer getFrameBuffer(u32 imageIndex) {
            return m_FrameBuffers[imageIndex].getHandle();
        }

        void createImageViews();
        void createFrameBuffers();
        void recreate(void* window, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, const QueueFamilyIndices& indices);

    public:
        static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

    private:
        static VkSurfaceFormatKHR selectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        static VkPresentModeKHR selectSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        static VkExtent2D selectSwapExtent(void* window, const VkSurfaceCapabilitiesKHR& capabilities);

    private:
        VkSwapchainKHR m_Handle;
        VkDevice m_Device;
        std::vector<VkImage> m_Images;
        std::vector<ImageView> m_ImageViews;
        VkFormat m_ImageFormat;
        VkExtent2D m_Extent;
        RenderPass m_RenderPass;
        std::vector<FrameBuffer> m_FrameBuffers;
    };

}