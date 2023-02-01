#pragma once

#include <RenderPass.h>
#include <FrameBuffer.h>
#include <Queues.h>

#ifdef VULKAN
#include <vulkan/vulkan.h>
#endif

#include <vector>

namespace rdk {

#ifdef VULKAN

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

#endif

    class SwapChain final {

    public:
        void create(void* window, void* physicalDevice, void* surface, const QueueFamilyIndices& indices);
        void destroy();
        void queryImages(u32 imageCount);

        inline void* getHandle() {
            return m_Handle;
        }

        inline void setLogicalDevice(void* logicalDevice) {
            m_LogicalDevice = logicalDevice;
        }

        inline const Extent2D& getExtent() {
            return m_Extent;
        }

        inline int getImageFormat() const {
            return m_ImageFormat;
        }

        inline void setRenderPass(const RenderPass& renderPass) {
            m_RenderPass = renderPass;
        }

        inline RenderPass& getRenderPass() {
            return m_RenderPass;
        }

        inline void* getFrameBuffer(u32 imageIndex) {
            return m_FrameBuffers[imageIndex].getHandle();
        }

        void createImageViews();
        void createFrameBuffers();
        void recreate(void* window, void* physicalDevice, void* surface, const QueueFamilyIndices& indices);

    public:
        static SwapChainSupportDetails querySwapChainSupport(void* physicalDevice, void* surfaceHandle);

    private:

#ifdef VULKAN

        static VkSurfaceFormatKHR selectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        static VkPresentModeKHR selectSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        static VkExtent2D selectSwapExtent(void* window, const VkSurfaceCapabilitiesKHR& capabilities);

#endif

    private:
        void* m_Handle;
        void* m_LogicalDevice;
        std::vector<void*> m_Images;
        std::vector<void*> m_ImageViews;
        int m_ImageFormat;
        Extent2D m_Extent;
        RenderPass m_RenderPass;
        std::vector<FrameBuffer> m_FrameBuffers;
    };

}