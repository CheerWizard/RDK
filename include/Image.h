#pragma once

#include <Core.h>
#include <Buffer.h>

namespace rdk {

    struct ImageData final {
        u32 width;
        u32 height;
        int channels;
        Buffer stageBuffer;
    };

    class ImageLoader final {

    public:
        static ImageData load(const char* filepath, VkDevice device, VkPhysicalDevice physicalDevice);
    };

    class Image final {

    public:
        Image() = default;

        Image(VkDevice device,
              VkPhysicalDevice physicalDevice,
              u32 width, u32 height,
              VkFormat format,
              VkImageTiling tiling,
              VkImageUsageFlags usage,
              VkMemoryPropertyFlags properties);

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

    class ImageView final {

    public:
        ImageView() = default;

        ImageView(VkDevice device, VkImage image, VkFormat format);

        ~ImageView();

    public:
        [[nodiscard]] inline VkImageView getHandle() const { return m_Handle; }

    private:
        void create(VkImage image, VkFormat format, const VkImageSubresourceRange& subresourceRange);

    private:
        VkImageView m_Handle;
        VkDevice m_Device;
    };

    class ImageSampler final {

    public:
        ImageSampler() = default;

        ImageSampler(
                const Device& device,
                VkFilter minFilter = VK_FILTER_LINEAR,
                VkFilter magFilter = VK_FILTER_LINEAR,
                VkSamplerAddressMode modeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                VkSamplerAddressMode modeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                VkSamplerAddressMode modeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                VkBorderColor borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
                VkBool32 normalized = VK_TRUE,
                VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
                float mipLodBias = 0,
                float minLod = 0,
                float maxLod = 0
        );

        ~ImageSampler();

    public:
        [[nodiscard]] inline VkSampler getHandle() const { return m_Handle; }

    private:
        VkSampler m_Handle;
        VkDevice m_Device;
    };

}