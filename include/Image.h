#pragma once

#include <Core.h>
#include <Buffer.h>

namespace rdk {

    struct ImageData final {
        u32 width;
        u32 height;
        int channels;
        u32 mipLevels;
        Buffer stageBuffer;
    };

    class ImageLoader final {

    public:
        static ImageData load(const char* filepath, VkDevice device, VkPhysicalDevice physicalDevice);
    };

    struct ImageInfo final {
        u32 width;
        u32 height;
        VkFormat format;
        VkImageTiling tiling;
        VkImageUsageFlags usage;
        VkMemoryPropertyFlags properties;
        u32 mipLevels = 1;
    };

    class Image final {

    public:
        Image() = default;

        Image(
            VkDevice device,
            VkPhysicalDevice physicalDevice,
            const ImageInfo& info
        );

        ~Image();

    public:
        [[nodiscard]] inline VkImage getHandle() const { return m_Handle; }
        [[nodiscard]] inline VkDevice getDevice() const { return m_Device; }

    private:
        void freeMemory();

    private:
        VkImage m_Handle;
        VkDeviceMemory m_Memory;
        VkDevice m_Device;
    };

    struct ImageViewInfo final {
        VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
        VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        u32 baseMipLevel = 0;
        u32 baseArrayLayer = 0;
        u32 mipLevels = 1;
        u32 layerCount = 1;
    };

    class ImageView final {

    public:
        ImageView() = default;

        ImageView(
                VkDevice device,
                VkImage image,
                const ImageViewInfo& info = {}
        );

        ~ImageView();

    public:
        [[nodiscard]] inline VkImageView getHandle() const { return m_Handle; }

    private:
        VkImageView m_Handle;
        VkDevice m_Device;
    };

    struct ImageSamplerInfo final {
        VkFilter minFilter = VK_FILTER_LINEAR;
        VkFilter magFilter = VK_FILTER_LINEAR;
        VkSamplerAddressMode modeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode modeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode modeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkBorderColor borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        VkBool32 normalized = VK_TRUE;
        VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        float mipLodBias = 0;
        float minLod = 0;
        float maxLod = 0;
        VkBool32 compareEnable = VK_FALSE;
        VkCompareOp compareOp = VK_COMPARE_OP_ALWAYS;
    };

    class ImageSampler final {

    public:
        ImageSampler() = default;

        ImageSampler(const Device& device, const ImageSamplerInfo& info = {});

        ~ImageSampler();

    public:
        [[nodiscard]] inline VkSampler getHandle() const { return m_Handle; }

    private:
        VkSampler m_Handle;
        VkDevice m_Device;
    };

}