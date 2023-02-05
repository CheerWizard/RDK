#include <Image.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <stdexcept>

namespace rdk {

    Image::Image(
            VkDevice device,
            VkPhysicalDevice physicalDevice,
            u32 width, u32 height,
            VkFormat format,
            VkImageTiling tiling,
            VkImageUsageFlags usage,
            VkMemoryPropertyFlags properties
    ) {
        // create image
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = static_cast<uint32_t>(width);
        imageInfo.extent.height = static_cast<uint32_t>(height);
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
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
                properties
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
            stageBuffer
        };
    }

    ImageView::ImageView(VkDevice device, VkImage image, VkFormat format) {
        VkImageSubresourceRange subresourceRange{};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = 1;
        m_Device = device;
        create(image, format, subresourceRange);
    }

    void ImageView::create(VkImage image, VkFormat format, const VkImageSubresourceRange &subresourceRange) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = image;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = format;
        createInfo.subresourceRange = subresourceRange;

        auto status = vkCreateImageView(m_Device, &createInfo, nullptr, &m_Handle);
        rect_assert(status == VK_SUCCESS, "Failed to create Vulkan image view")
    }

    ImageView::~ImageView() {
        vkDestroyImageView(m_Device, m_Handle, nullptr);
    }

    ImageSampler::ImageSampler(
            const Device& device,
            VkFilter minFilter, VkFilter magFilter,
            VkSamplerAddressMode modeU,
            VkSamplerAddressMode modeV,
            VkSamplerAddressMode modeW,
            VkBorderColor borderColor,
            VkBool32 normalized,
            VkSamplerMipmapMode mipmapMode,
            float mipLodBias,
            float minLod,
            float maxLod
    ) {
        m_Device = device.getLogicalHandle();

        VkSamplerCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        createInfo.magFilter = magFilter;
        createInfo.minFilter = minFilter;
        createInfo.addressModeU = modeU;
        createInfo.addressModeV = modeV;
        createInfo.addressModeW = modeW;
        createInfo.borderColor = borderColor;
        createInfo.unnormalizedCoordinates = !normalized;
        createInfo.compareEnable = VK_FALSE;
        createInfo.compareOp = VK_COMPARE_OP_ALWAYS;

        createInfo.mipmapMode = mipmapMode;
        createInfo.mipLodBias = mipLodBias;
        createInfo.minLod = minLod;
        createInfo.maxLod = maxLod;

//        createInfo.anisotropyEnable = device.getFeatures().samplerAnisotropy;
//        createInfo.maxAnisotropy = device.getProperties().limits.maxSamplerAnisotropy;

        createInfo.anisotropyEnable = VK_FALSE;
        createInfo.maxAnisotropy = 1;

        auto status = vkCreateSampler(m_Device, &createInfo, nullptr, &m_Handle);
        rect_assert(status == VK_SUCCESS, "Failed to create a Vulkan sampler")
    }

    ImageSampler::~ImageSampler() {
        vkDestroySampler(m_Device, m_Handle, nullptr);
    }
}