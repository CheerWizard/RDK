#include <DescriptorPool.h>

namespace rdk {

    void DescriptorPool::create(VkDevice device, const VkDescriptorPoolSize& poolSize, u32 maxSets) {
        m_Device = device;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = maxSets;

        auto status = vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_Handle);
        rect_assert(status == VK_SUCCESS, "Failed to create Vulkan descriptor pool")
    }

    void DescriptorPool::destroy() {
        vkDestroyDescriptorPool(m_Device, m_Handle, nullptr);
    }

    void DescriptorPool::createSets(u32 count, const VkDescriptorSetLayout& layout) {
        std::vector<VkDescriptorSetLayout> layouts(count, layout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_Handle;
        allocInfo.descriptorSetCount = count;
        allocInfo.pSetLayouts = layouts.data();

        m_Sets.resize(count);
        auto status = vkAllocateDescriptorSets(m_Device, &allocInfo, m_Sets.data());
        rect_assert(status == VK_SUCCESS, "Failed to create Vulkan descriptor sets")
    }

}