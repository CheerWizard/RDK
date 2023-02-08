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
        SwapChain(
                void* window,
                Device* device,
                VkSurfaceKHR surface,
                VkFormat depthFormat
        );
        ~SwapChain();

    public:
        [[nodiscard]] inline VkSwapchainKHR getHandle() {
            return m_Handle;
        }

        [[nodiscard]] inline const VkExtent2D& getExtent() {
            return m_Extent;
        }

        [[nodiscard]] inline RenderPass& getRenderPass() {
            return *m_RenderPass;
        }

        [[nodiscard]] inline VkFramebuffer getFrameBuffer(u32 imageIndex) {
            return m_FrameBuffers[imageIndex].getHandle();
        }

        [[nodiscard]] inline VkImage getDepthImage() const {
            return m_DepthImage->getHandle();
        }

        [[nodiscard]] inline VkFormat getDepthFormat() const {
            return m_DepthFormat;
        }

        void recreate(void* window, VkSurfaceKHR surface);
        void recreate(void* window, VkSurfaceKHR surface, const QueueFamilyIndices& familyIndices);

    public:
        static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

    private:
        static VkSurfaceFormatKHR selectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        static VkPresentModeKHR selectSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        static VkExtent2D selectSwapExtent(void* window, const VkSurfaceCapabilitiesKHR& capabilities);

        void create(void* window, VkSurfaceKHR surface);
        void createColorImages();
        void createDepthImage();
        void createFrameBuffers();

    private:
        VkSwapchainKHR m_Handle;
        Device* m_Device;
        VkExtent2D m_Extent;
        // color images
        VkFormat m_ColorFormat;
        std::vector<VkImage> m_Images;
        std::vector<ImageView> m_ImageViews;
        // depth image
        VkFormat m_DepthFormat;
        Image* m_DepthImage;
        ImageView* m_DepthImageView;
        // render pass and frame buffers
        RenderPass* m_RenderPass;
        std::vector<FrameBuffer> m_FrameBuffers;
    };
}