#include <Image.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <stdexcept>
#include <cmath>

namespace rdk {

    Image::Image(
            VkDevice device,
            VkPhysicalDevice physicalDevice,
            const ImageInfo& info
    ) {
        // create image
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = static_cast<uint32_t>(info.width);
        imageInfo.extent.height = static_cast<uint32_t>(info.height);
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = info.mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = info.format;
        imageInfo.tiling = info.tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = info.usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags = 0; // Optional
        auto status = vkCreateImage(device, &imageInfo, nullptr, &m_Handle);
        rect_assert(status == VK_SUCCESS, "Failed to create a Vulkan image")

        // allocate memory
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, m_Handle, &memRequirements);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = Buffer::findMemoryType(
                physicalDevice,
                memRequirements.memoryTypeBits,
                info.properties
        );

        auto memoryStatus = vkAllocateMemory(device, &allocInfo, nullptr, &m_Memory);
        rect_assert(memoryStatus == VK_SUCCESS, "Failed to allocate Vulkan image memory")

        vkBindImageMemory(device, m_Handle, m_Memory, 0);

        m_Device = device;
    }

    void Image::freeMemory() {
        vkFreeMemory(m_Device, m_Memory, nullptr);
    }

    Image::~Image() {
        vkDestroyImage(m_Device, m_Handle, nullptr);
        freeMemory();
    }

    ImageData ImageLoader::load(const char *filepath, VkDevice device, VkPhysicalDevice physicalDevice) {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(filepath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;
        u32 mipLevels = std::floor(std::log2(std::max(texWidth, texHeight))) + 1;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        // create stage buffer
        Buffer stageBuffer{};
        stageBuffer.create(
                imageSize,
                device, physicalDevice,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        // copy data into stage buffer
        void* block = stageBuffer.mapMemory(imageSize);
        memcpy(block, pixels, imageSize);
        stageBuffer.unmapMemory();

        // free image pixels
        stbi_image_free(pixels);

        return {
            static_cast<u32>(texWidth),
            static_cast<u32>(texHeight),
            texChannels,
            mipLevels,
            stageBuffer
        };
    }

    ImageView::ImageView(VkDevice device, VkImage image, const ImageViewInfo& info) {
        m_Device = device;

        VkImageSubresourceRange subresourceRange {};
        subresourceRange.aspectMask = info.aspectMask;
        subresourceRange.baseMipLevel = info.baseMipLevel;
        subresourceRange.baseArrayLayer = info.baseArrayLayer;
        subresourceRange.levelCount = info.mipLevels;
        subresourceRange.layerCount = info.layerCount;

        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = image;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = info.format;
        createInfo.subresourceRange = subresourceRange;

        auto status = vkCreateImageView(m_Device, &createInfo, nullptr, &m_Handle);
        rect_assert(status == VK_SUCCESS, "Failed to create Vulkan image view")
    }

    ImageView::~ImageView() {
        vkDestroyImageView(m_Device, m_Handle, nullptr);
    }

    ImageSampler::ImageSampler(
            const Device& device,
            const ImageSamplerInfo& info
    ) {
        m_Device = device.getLogicalHandle();

        VkSamplerCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

        createInfo.magFilter = info.magFilter;
        createInfo.minFilter = info.minFilter;

        createInfo.addressModeU = info.modeU;
        createInfo.addressModeV = info.modeV;
        createInfo.addressModeW = info.modeW;

        createInfo.borderColor = info.borderColor;

        createInfo.unnormalizedCoordinates = !info.normalized;

        createInfo.compareEnable = info.compareEnable;
        createInfo.compareOp = info.compareOp;

        createInfo.mipmapMode = info.mipmapMode;
        createInfo.mipLodBias = info.mipLodBias;
        createInfo.minLod = info.minLod;
        createInfo.maxLod = info.maxLod;

        VkPhysicalDeviceFeatures features = device.getFeatures();
        VkPhysicalDeviceProperties props = device.getProperties();

        createInfo.anisotropyEnable = features.samplerAnisotropy;
        createInfo.maxAnisotropy = createInfo.anisotropyEnable ? props.limits.maxSamplerAnisotropy : 1;

        auto status = vkCreateSampler(m_Device, &createInfo, nullptr, &m_Handle);
        rect_assert(status == VK_SUCCESS, "Failed to create a Vulkan sampler")
    }

    ImageSampler::~ImageSampler() {
        vkDestroySampler(m_Device, m_Handle, nullptr);
    }
}